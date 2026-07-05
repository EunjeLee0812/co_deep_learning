// PluginChain.cpp — 동적 이펙터 체인 구현
// (Bela Makefile 은 하위 폴더 .cpp 에 CPPFLAGS 를 안 넘기므로 상대경로 include)
#include "PluginChain.h"
#include "ChannelStrip/Equalizer.h"
#include "ChannelStrip/Compressor.h"
#include "Distortion/Distortion.h"
#include "Modulation/Chorus.h"
#include "Modulation/Flanger.h"
#include "Modulation/Phaser.h"
#include "Spaces/Reverb.h"
#include "Spaces/Delay.h"

namespace fx {

// ───────────────────────── 팩토리 ─────────────────────────
// ★ 편집 스레드(AuxiliaryTask)에서만 호출된다. 오디오 스레드 할당 금지 원칙 준수.
Effect* PluginChain::createEffect(EffectType type) {
    Effect* e = nullptr;
    switch (type) {
        case EffectType::EQ:      e = new Equalizer();  break;
        case EffectType::Comp:    e = new Compressor(); break;
        case EffectType::Dist:    e = new Distortion(); break;
        case EffectType::Chorus:  e = new Chorus();     break;
        case EffectType::Flanger: e = new Flanger();    break;
        case EffectType::Phaser:  e = new Phaser();     break;
        case EffectType::Reverb:  e = new Reverb();     break;
        case EffectType::Delay:   e = new Delay();      break;
        default: return nullptr;
    }
    if (!e->setup(sampleRate_, maxBlock_)) { delete e; return nullptr; }
    e->setBypass(false);   // 사용자가 명시적으로 추가한 이펙트는 바로 켠다
    e->reset();
    return e;
}

// ───────────────────────── 수명 관리 ─────────────────────────
bool PluginChain::setup(float sampleRate, unsigned int maxBlockSize) {
    sampleRate_ = sampleRate;
    maxBlock_   = maxBlockSize;
    editCount_  = 0;
    retiredCount_ = 0;
    snap_[0].count = snap_[1].count = 0;
    activeSnap_.store(0, std::memory_order_release);
    return true;
}

void PluginChain::cleanup() {
    // 오디오 스레드가 이미 멈춘 뒤(Bela cleanup 단계) 호출된다고 가정.
    for (int i = 0; i < editCount_; ++i) { editFx_[i]->cleanup(); delete editFx_[i]; }
    editCount_ = 0;
    for (int i = 0; i < retiredCount_; ++i) { retired_[i].fx->cleanup(); delete retired_[i].fx; }
    retiredCount_ = 0;
    snap_[0].count = snap_[1].count = 0;
}

// ───────────────────────── 오디오 스레드 ─────────────────────────
void PluginChain::process(float* left, float* right, unsigned int numFrames) {
    // 스냅샷은 발행 이후 내용이 바뀌지 않으므로(불변), 인덱스만 읽으면 안전.
    const Snapshot& s = snap_[activeSnap_.load(std::memory_order_acquire)];
    for (int i = 0; i < s.count; ++i)
        s.fx[i]->process(left, right, numFrames);           // 직렬 in-place
    renderTick_.fetch_add(1, std::memory_order_release);    // 해제 안전 판정용
}

// ───────────────────────── 발행 (RCU) ─────────────────────────
void PluginChain::publish() {
    const int cur  = activeSnap_.load(std::memory_order_relaxed);
    const int next = cur ^ 1;
    Snapshot& s = snap_[next];
    s.count = editCount_;
    for (int i = 0; i < editCount_; ++i) s.fx[i] = editFx_[i];
    activeSnap_.store(next, std::memory_order_release);
}

// ───────────────────────── 편집 연산 ─────────────────────────
int PluginChain::addEffect(EffectType type) {
    if (editCount_ >= kMaxSlots) return -1;
    Effect* e = createEffect(type);
    if (!e) return -1;
    editFx_[editCount_]   = e;
    editType_[editCount_] = type;
    ++editCount_;
    publish();
    return editCount_ - 1;
}

bool PluginChain::removeEffect(int slot) {
    if (slot < 0 || slot >= editCount_) return false;
    Effect* victim = editFx_[slot];
    for (int i = slot; i < editCount_ - 1; ++i) {           // 뒤쪽을 한 칸씩 당김
        editFx_[i]   = editFx_[i + 1];
        editType_[i] = editType_[i + 1];
    }
    --editCount_;
    publish();                                              // 먼저 새 체인 발행
    // 오디오 스레드가 아직 옛 스냅샷을 도는 중일 수 있으니 즉시 delete 금지.
    if (retiredCount_ < (int)(sizeof(retired_) / sizeof(retired_[0]))) {
        retired_[retiredCount_++] = { victim, renderTick_.load(std::memory_order_acquire) };
    } else {
        // 은퇴 목록이 가득이면(비정상적으로 빠른 연타) 이전 것부터 강제 정리
        collectGarbage();
        retired_[retiredCount_++] = { victim, renderTick_.load(std::memory_order_acquire) };
    }
    return true;
}

bool PluginChain::moveEffect(int from, int to) {
    if (from < 0 || from >= editCount_ || to < 0 || to >= editCount_ || from == to)
        return false;
    Effect*    e = editFx_[from];
    EffectType t = editType_[from];
    if (from < to) {
        for (int i = from; i < to; ++i) { editFx_[i] = editFx_[i+1]; editType_[i] = editType_[i+1]; }
    } else {
        for (int i = from; i > to; --i) { editFx_[i] = editFx_[i-1]; editType_[i] = editType_[i-1]; }
    }
    editFx_[to] = e; editType_[to] = t;
    publish();
    return true;
}

void PluginChain::setSlotBypass(int slot, bool bypass) {
    if (slot < 0 || slot >= editCount_) return;
    editFx_[slot]->setBypass(bypass);   // bool 대입 — 스레드간 벤ign
}

bool PluginChain::slotBypassed(int slot) const {
    if (slot < 0 || slot >= editCount_) return false;
    return editFx_[slot]->isBypassed();
}

void PluginChain::setSlotParameter(int slot, int paramId, float value) {
    if (slot < 0 || slot >= editCount_) return;
    editFx_[slot]->setParameter(paramId, value);
}

void PluginChain::setSlotTempo(int slot, float bpm) {
    if (slot < 0 || slot >= editCount_) return;
    // setTempo 는 Effect 베이스에 없어서 종류별로 다운캐스트.
    switch (editType_[slot]) {
        case EffectType::Chorus:  static_cast<Chorus*>(editFx_[slot])->setTempo(bpm);  break;
        case EffectType::Flanger: static_cast<Flanger*>(editFx_[slot])->setTempo(bpm); break;
        case EffectType::Phaser:  static_cast<Phaser*>(editFx_[slot])->setTempo(bpm);  break;
        case EffectType::Delay:   editFx_[slot]->setParameter((int)Delay::Param::TempoBpm, bpm); break;
        default: break;
    }
}

void PluginChain::collectGarbage() {
    const uint32_t now = renderTick_.load(std::memory_order_acquire);
    int w = 0;
    for (int i = 0; i < retiredCount_; ++i) {
        // 발행 시점 이후 오디오 블록이 2번 이상 지나갔으면 옛 스냅샷 참조는 끝났다.
        if ((uint32_t)(now - retired_[i].retiredAt) >= 2) {
            retired_[i].fx->cleanup();
            delete retired_[i].fx;
        } else {
            retired_[w++] = retired_[i];   // 아직 이르면 유지
        }
    }
    retiredCount_ = w;
}

} // namespace fx

// PluginChain.h — "동적" 이펙터 체인 (DAW 플러그인 방식)
// ───────────────────────────────────────────────────────────────────────────
// 기존 EffectChain(슬롯 5개 고정)과 달리, 이펙트 인스턴스를
//   - 원하는 종류로, 같은 종류 여러 개도 추가 (addEffect)
//   - 아무 위치나 삭제 (removeEffect)
//   - 순서 자유 변경 (moveEffect)
// 할 수 있다. 각 인스턴스는 완전히 독립적인 상태(딜레이라인/필터/스무더)를 가진다.
//
// ★ 최대 개수 kMaxSlots = 8 로 지정했다. 근거:
//   - 가장 무거운 이펙트는 Reverb(콤브 8 + 올패스 4, 스테레오)와 Delay(최대 4초
//     스테레오 딜레이라인)이다. PocketBeagle 2 의 A53 단일 코어는 기존 Bela 대비
//     2~6배 빠르고(공식 벤치마크), 기존 Bela 에서도 Freeverb 계열 리버브 1개는
//     한 자릿수 % CPU 수준이었다. 8슬롯 전부 리버브로 채우는 극단적인 경우에도
//     신스 4보이스와 합쳐 단일 코어에서 충분한 여유가 남는다.
//   - 메모리: Delay 1개가 최대 4초*2ch*4B ≈ 1.4MB 로 가장 크다. 8개여도 ~12MB,
//     PB2 의 512MB RAM 에 전혀 문제 없다.
//   - UI 관점에서도 800px 폭 디스플레이에 아이콘 8칸(96px 간격)이 딱 맞는다.
//
// ── 스레딩 규칙 (매우 중요) ─────────────────────────────────────────────────
//   process()                       : 오디오 스레드 전용. 절대 할당/해제 없음.
//   addEffect / removeEffect /
//   moveEffect / setSlotBypass /
//   setSlotParameter / collectGarbage : "편집 스레드"(디스플레이 AuxiliaryTask) 전용.
//                                       한 스레드에서만 호출할 것.
//
//   구조 변경(추가/삭제/이동)은 RCU 방식으로 처리한다:
//     1) 편집 스레드가 비활성 스냅샷 버퍼에 새 포인터 배열을 만든다
//     2) atomic 인덱스 교체로 발행 → 오디오 스레드는 다음 블록부터 새 체인 사용
//     3) 삭제된 인스턴스는 즉시 delete 하지 않고 "은퇴 목록"에 넣고,
//        오디오 스레드가 최소 2블록 지나간 것을 renderTick 으로 확인한 뒤 해제
//   → 오디오 스레드는 언제나 유효한 포인터만 보고, 락도 없다.
//
//   파라미터 변경(setSlotParameter)은 이 프로젝트의 기존 관례
//   (디스플레이 태스크 → effect->setParameter 직접 호출, float 대입은
//    ARM64 에서 원자적)를 그대로 따른다.
// ───────────────────────────────────────────────────────────────────────────
#pragma once
#include <atomic>
#include <cstdint>
#include "Effect.h"

namespace fx {

// 추가 가능한 이펙트 종류. 디스플레이/프로토콜과 공유하는 ID.
enum class EffectType : int {
    EQ       = 0,  // ChannelStrip 카테고리
    Comp     = 1,
    Dist     = 2,
    Chorus   = 3,  // Modulation 카테고리
    Flanger  = 4,
    Phaser   = 5,
    Reverb   = 6,  // Spaces 카테고리
    Delay    = 7,
    Count
};

class PluginChain {
public:
    static constexpr int kMaxSlots = 8;   // ★ 성능/메모리/UI 근거는 파일 상단 주석

    bool setup(float sampleRate, unsigned int maxBlockSize);
    void cleanup();                        // 모든 인스턴스 + 은퇴 목록 해제

    // ── 오디오 스레드 (render) ──
    void process(float* left, float* right, unsigned int numFrames);

    // ── 편집 스레드 (디스플레이 AuxiliaryTask) ──
    // 성공 시 새 슬롯 인덱스(체인 끝), 꽉 찼거나 실패 시 -1.
    int  addEffect(EffectType type);
    bool removeEffect(int slot);
    bool moveEffect(int from, int to);     // from 슬롯을 to 위치로 (사이에 낀 것들 밀림)
    void setSlotBypass(int slot, bool bypass);
    void setSlotParameter(int slot, int paramId, float value); // 각 이펙트 Param enum 값
    // 모듈레이션 계열(Chorus/Flanger/Phaser)의 BPM 은 Param 에 없어서 별도 통로.
    void setSlotTempo(int slot, float bpm);
    void collectGarbage();                 // 은퇴 인스턴스 중 해제 가능한 것 정리. 주기 호출 권장.

    // ── 상태 조회 (편집 스레드에서 UI 그리기용) ──
    int        numSlots() const { return editCount_; }
    EffectType slotType(int slot) const { return editType_[slot]; }
    bool       slotBypassed(int slot) const;

private:
    // 오디오 스레드가 읽는 스냅샷. 2개를 번갈아 쓴다(RCU).
    struct Snapshot {
        Effect* fx[kMaxSlots];
        int     count = 0;
    };
    Snapshot            snap_[2];
    std::atomic<int>    activeSnap_{0};    // 현재 발행된 스냅샷 인덱스
    std::atomic<uint32_t> renderTick_{0};  // process() 마다 +1 (해제 안전 판정용)

    // 편집 스레드 전용 마스터 상태
    Effect*     editFx_[kMaxSlots]   = {nullptr};
    EffectType  editType_[kMaxSlots] = {EffectType::EQ};
    int         editCount_ = 0;

    // 은퇴(삭제 대기) 목록: renderTick 이 retiredAt+2 를 넘으면 delete 안전
    struct Retired { Effect* fx; uint32_t retiredAt; };
    Retired     retired_[kMaxSlots * 2];
    int         retiredCount_ = 0;

    float        sampleRate_ = 44100.0f;
    unsigned int maxBlock_   = 16;

    Effect* createEffect(EffectType type);  // new (편집 스레드에서만!)
    void    publish();                       // editFx_ → 비활성 스냅샷 → atomic 교체
};

} // namespace fx

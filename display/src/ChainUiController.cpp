// ChainUiController.cpp — Nextion ↔ PluginChain 컨트롤러 구현
#include "ChainUiController.h"
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cmath>

namespace disp {

// Nextion 메인 페이지 컴포넌트 이름 규약 (Nextion Editor 에서 동일하게 만들 것):
//   슬롯 아이콘   : p0..p7   (Picture)
//   슬롯 이름     : t0..t7   (Text, 아이콘 아래)
//   파라미터 라벨 : lp0..lp3 (Text)
//   파라미터 슬라이더 : h0..h3 (Slider, minval=0 maxval=1000)
//   파라미터 값 표시  : vp0..vp3 (Text)
//   선택 이펙트 헤더  : thdr (Text)  / 페이지 표시 : tpg (Text)
//   바이패스 버튼     : bbyp (Dual-state Button, bbyp.val)
// 좌표 규약: 슬롯 i 의 x = 8 + i*98 (아이콘 88px). NEXTION_GUIDE.md 참고.

bool ChainUiController::setup(fx::PluginChain* chain, const char* device, int baud) {
    chain_ = chain;
    if (!serial_.open(device, baud)) return false;
    rxLen_ = 0;
    selected_ = -1;
    paramPage_ = 0;
    // 부팅 직후 Nextion 이 아직 안 깨어 있어도, Nextion 쪽 page0 진입 이벤트가
    // SYNC(0xA5 0x08) 를 보내므로 그때 redrawAll() 이 실행된다.
    return true;
}

void ChainUiController::cleanup() { serial_.close(); }

void ChainUiController::update() {
    pumpSerial();
    chain_->collectGarbage();   // 삭제된 인스턴스 지연 해제 (여기가 편집 스레드)
}

// ───────────────────────── 수신 ─────────────────────────
int ChainUiController::payloadLen(unsigned char cmd) {
    switch (cmd) {
        case 0x01: return 4;   // ADD    type
        case 0x02: return 4;   // REMOVE slot
        case 0x03: return 8;   // MOVE   from,to
        case 0x04: return 4;   // SELECT slot
        case 0x05: return 4;   // PAGE   page
        case 0x06: return 8;   // PARAM  row,raw
        case 0x07: return 4;   // BYPASS on
        case 0x08: return 0;   // SYNC
        default:   return -1;  // 모르는 명령
    }
}

void ChainUiController::pumpSerial() {
    unsigned char buf[64];
    int n;
    while ((n = serial_.readBytes(buf, sizeof(buf))) > 0) {
        for (int i = 0; i < n; ++i) {
            const unsigned char b = buf[i];
            if (rxLen_ == 0) {                    // 헤더 대기
                if (b == 0xA5) rx_[rxLen_++] = b; // 그 외 바이트(Nextion 부팅 메시지 등)는 무시
                continue;
            }
            rx_[rxLen_++] = b;
            if (rxLen_ >= 2) {
                const int pl = payloadLen(rx_[1]);
                if (pl < 0) { rxLen_ = 0; continue; }        // 미지 명령 → 리셋
                if (rxLen_ == 2 + pl) {                       // 프레임 완성
                    handleFrame(rx_[1], rx_ + 2);
                    rxLen_ = 0;
                }
            }
            if (rxLen_ >= (int)sizeof(rx_)) rxLen_ = 0;       // 안전장치
        }
    }
}

void ChainUiController::handleFrame(unsigned char cmd, const unsigned char* pl) {
    switch (cmd) {
        case 0x01: doAdd((int)i32(pl)); break;
        case 0x02: doRemove((int)i32(pl)); break;
        case 0x03: doMove((int)i32(pl), (int)i32(pl + 4)); break;
        case 0x04: doSelect((int)i32(pl)); break;
        case 0x05: doPage((int)i32(pl)); break;
        case 0x06: doParam((int)i32(pl), (int)i32(pl + 4)); break;
        case 0x07: doBypass(i32(pl) != 0); break;
        case 0x08: redrawAll(); break;
        default: break;
    }
}

// ───────────────────────── 명령 처리 ─────────────────────────
void ChainUiController::initSlotCache(int slot) {
    const EffectDef& def = kEffects[(int)chain_->slotType(slot)];
    for (int p = 0; p < def.numParams && p < 16; ++p) {
        float v = def.params[p].def;
        paramCache_[slot][p] = v;
        // Compressor Ratio 특수 매핑 (인덱스→실제 비율값)
        if (chain_->slotType(slot) == fx::EffectType::Comp && p == 2)
            v = compRatioFromIndex((int)v);
        chain_->setSlotParameter(slot, p, v);
    }
}

void ChainUiController::doAdd(int type) {
    if (type < 0 || type >= (int)fx::EffectType::Count) return;
    const int slot = chain_->addEffect((fx::EffectType)type);
    if (slot < 0) {
        nxSend("thdr.txt=\"CHAIN FULL (8)\"");   // 꽉 참 안내
        return;
    }
    initSlotCache(slot);
    selected_  = slot;    // 방금 추가한 이펙트를 바로 선택
    paramPage_ = 0;
    redrawAll();
}

void ChainUiController::doRemove(int slot) {
    if (slot == -1) slot = selected_;      // -1 = "현재 선택 슬롯 삭제" (Nextion 딜리트 버튼)
    if (slot < 0) return;
    if (!chain_->removeEffect(slot)) return;
    // 캐시도 한 칸 당김
    for (int i = slot; i < chain_->numSlots(); ++i)
        memcpy(paramCache_[i], paramCache_[i + 1], sizeof(paramCache_[i]));
    if (selected_ == slot) selected_ = -1;
    else if (selected_ > slot) --selected_;
    paramPage_ = 0;
    redrawAll();
}

void ChainUiController::doMove(int from, int to) {
    const int n = chain_->numSlots();
    if (to < 0) to = 0;
    if (to > n - 1) to = n - 1;            // 화면 밖/빈칸에 떨어뜨려도 끝으로 클램프
    if (from == to) { redrawAll(); return; }   // 제자리 → 드래그 잔상만 지우게 다시 그림
    if (!chain_->moveEffect(from, to)) { redrawAll(); return; }
    // 캐시 배열도 동일하게 회전
    float tmp[16];
    memcpy(tmp, paramCache_[from], sizeof(tmp));
    if (from < to)
        for (int i = from; i < to; ++i) memcpy(paramCache_[i], paramCache_[i+1], sizeof(tmp));
    else
        for (int i = from; i > to; --i) memcpy(paramCache_[i], paramCache_[i-1], sizeof(tmp));
    memcpy(paramCache_[to], tmp, sizeof(tmp));
    // 선택 추적
    if (selected_ == from) selected_ = to;
    else if (from < to && selected_ > from && selected_ <= to) --selected_;
    else if (to < from && selected_ >= to && selected_ < from) ++selected_;
    redrawAll();
}

void ChainUiController::doSelect(int slot) {
    if (slot < 0 || slot >= chain_->numSlots()) return;
    selected_  = slot;
    paramPage_ = 0;
    redrawChain();
    redrawParams();
}

void ChainUiController::doPage(int page) {
    if (selected_ < 0) return;
    const EffectDef& def = kEffects[(int)chain_->slotType(selected_)];
    const int maxPage = (def.numParams + 3) / 4 - 1;
    if (page < 0) page = 0;
    if (page > maxPage) page = maxPage;
    paramPage_ = page;
    redrawParams();
}

void ChainUiController::doParam(int row, int raw) {
    if (selected_ < 0 || row < 0 || row > 3) return;
    const fx::EffectType type = chain_->slotType(selected_);
    const EffectDef& def = kEffects[(int)type];
    const int pIdx = paramPage_ * 4 + row;
    if (pIdx >= def.numParams) return;
    const ParamDef& p = def.params[pIdx];

    float real = rawToReal(p, raw);
    paramCache_[selected_][pIdx] = real;

    // Compressor Ratio: 인덱스(0/1/2) → 실제 비율(3/5/10) 로 바꿔 전송
    float toSend = real;
    if (type == fx::EffectType::Comp && pIdx == 2)
        toSend = compRatioFromIndex((int)real);

    chain_->setSlotParameter(selected_, pIdx, toSend);
    redrawValueText(row, p, real);   // 값 텍스트만 가볍게 갱신 (슬라이더는 이미 움직임)
}

void ChainUiController::doBypass(bool on) {
    if (selected_ < 0) return;
    chain_->setSlotBypass(selected_, on);
}

// ───────────────────────── 송신 ─────────────────────────
void ChainUiController::nxSend(const char* cmd) {
    serial_.writeBytes((const unsigned char*)cmd, strlen(cmd));
    static const unsigned char term[3] = { 0xFF, 0xFF, 0xFF };
    serial_.writeBytes(term, 3);
}

void ChainUiController::nxSendf(const char* fmt, ...) {
    char buf[128];
    va_list ap; va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    nxSend(buf);
}

void ChainUiController::redrawAll() { redrawChain(); redrawParams(); }

void ChainUiController::redrawChain() {
    const int n = chain_->numSlots();
    nxSendf("va_sel.val=%d", selected_);   // Nextion 쪽 선택 변수도 동기화
    for (int i = 0; i < fx::PluginChain::kMaxSlots; ++i) {
        if (i < n) {
            const int t = (int)chain_->slotType(i);
            nxSendf("vis p%d,1", i);
            nxSendf("vis t%d,1", i);
            nxSendf("p%d.pic=%d", i, (i == selected_) ? kPicSelected[t] : kPicNormal[t]);
            nxSendf("t%d.txt=\"%s\"", i, kEffects[t].name);
        } else {
            nxSendf("vis p%d,0", i);   // 빈 슬롯은 숨김
            nxSendf("vis t%d,0", i);
        }
    }
}

void ChainUiController::redrawValueText(int row, const ParamDef& p, float real) {
    // 이산 파라미터는 정수로, 연속은 소수 1자리로 표시 (범위 커서 큰 값은 정수)
    if (p.steps >= 2 || fabsf(p.max) >= 100.0f)
        nxSendf("vp%d.txt=\"%d\"", row, (int)lroundf(real));
    else
        nxSendf("vp%d.txt=\"%.1f\"", row, real);
}

void ChainUiController::redrawParams() {
    if (selected_ < 0 || selected_ >= chain_->numSlots()) {
        nxSend("thdr.txt=\"-- select an effect --\"");
        nxSend("tpg.txt=\"\"");
        for (int r = 0; r < 4; ++r) {
            nxSendf("vis lp%d,0", r); nxSendf("vis h%d,0", r); nxSendf("vis vp%d,0", r);
        }
        nxSend("bbyp.val=0");
        return;
    }
    const fx::EffectType type = chain_->slotType(selected_);
    const EffectDef& def = kEffects[(int)type];
    const int maxPage = (def.numParams + 3) / 4;

    nxSendf("va_pg.val=%d", paramPage_);   // Nextion 쪽 페이지 변수 동기화 (클램프 반영)
    nxSendf("thdr.txt=\"%s  [slot %d]\"", def.name, selected_ + 1);
    nxSendf("tpg.txt=\"%d/%d\"", paramPage_ + 1, maxPage);
    nxSendf("bbyp.val=%d", chain_->slotBypassed(selected_) ? 1 : 0);

    for (int r = 0; r < 4; ++r) {
        const int pIdx = paramPage_ * 4 + r;
        if (pIdx < def.numParams) {
            const ParamDef& p = def.params[pIdx];
            const float real  = paramCache_[selected_][pIdx];
            nxSendf("vis lp%d,1", r); nxSendf("vis h%d,1", r); nxSendf("vis vp%d,1", r);
            nxSendf("lp%d.txt=\"%s\"", r, p.name);
            nxSendf("h%d.val=%d", r, realToRaw(p, real));
            redrawValueText(r, p, real);
        } else {
            nxSendf("vis lp%d,0", r); nxSendf("vis h%d,0", r); nxSendf("vis vp%d,0", r);
        }
    }
}

} // namespace disp

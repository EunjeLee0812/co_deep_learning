#include "DisplayController.h"
#include <cmath>
#include <cstdio>

namespace disp {

void DisplayController::setup(NextionLink* link, EffectParamSink* sink) {
    link_ = link;
    sink_ = sink;

    // Nextion 이벤트가 오면 onNextionEvent 로 들어오게 연결
    if (link_)
        link_->setEventHandler([this](const NextionEvent& ev){ onNextionEvent(ev); });

    // 초기 상태: 0번 이펙트 선택 + 화면 동기화
    selected_ = 0;
    focusParam_ = 0;
    selectEffect(0, /*notifySink=*/true);
}

void DisplayController::update() {
    if (link_) link_->poll();  // 들어온 터치/숫자 메시지 처리(핸들러로 디스패치됨)
    // TODO: 보낼 게 큐에 쌓이는 구조라면 여기서 flush
}

// ---------------------------------------------------------------------------
// Nextion → 여기로 들어오는 이벤트
void DisplayController::onNextionEvent(const NextionEvent& ev) {
    if (ev.type == NextionEvent::Type::Touch) {
        // 누름(press)만 처리, 뗌(release)은 무시
        if (ev.touchEvent != 0x01) return;

        // 1) 육각형(이펙트) 터치?
        int idx = menu_.effectIndexForComponent(ev.component);
        if (idx >= 0) {
            selectEffect(idx, /*notifySink=*/true);
            return;
        }
        // 2) 하단 슬라이더/버튼 터치는 컴포넌트 번호로 처리하거나(아래 Number 방식 권장)
        //    Editor 의 슬라이더 Touch Move 이벤트에서 값을 보내도록 설정하는 게 깔끔.
    }
    else if (ev.type == NextionEvent::Type::Number) {
        // 슬라이더를 드래그하면 Editor 설정으로 "현재 슬롯 + 값"을 보낼 수 있음.
        // 가장 단순한 약속: 상위 8비트=슬롯(0~3), 하위 8비트=퍼센트(0~100).
        // (Editor 슬라이더 Touch Move 에서  get  로 조합해 보내도록 구성)
        int slot    = (ev.number >> 8) & 0xFF;
        int percent = ev.number & 0xFF;
        if (slot >= 0 && slot < 4) setParamPercent(slot, percent);
    }
}

// ---------------------------------------------------------------------------
// 이펙트 선택 (흰 글로우 전환 + 하단 파라미터 로드)
void DisplayController::selectEffect(int index, bool notifySink) {
    if (index < 0) index = 0;
    if (index >= menu_.numEffects()) index = menu_.numEffects() - 1;
    selected_ = index;
    focusParam_ = 0;

    refreshHexHighlights();  // 모든 육각형: 선택만 글로우, 나머지 일반
    refreshParamRow();       // 하단 4칸 갱신

    if (notifySink && sink_)
        sink_->onEffectSelected(menu_.effect(selected_).target);
}

// 하단 슬롯 값 설정 → 엔진 송출 + 화면 갱신
void DisplayController::setParamPercent(int slot, int percent) {
    if (slot < 0 || slot >= menu_.effect(selected_).numParams) return;
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    paramPercent_[selected_][slot] = percent;
    const ParamDef& p = menu_.effect(selected_).params[slot];

    // 1) 외부(엔진)로 실제값 송출
    if (sink_)
        sink_->setEffectParameter(p.target, p.paramId, percentToReal(p, percent));

    // 2) 화면 숫자/프로그레스바 갱신 (노브로 바꾼 경우 디스플레이도 따라오게)
    if (link_) {
        link_->setNumber(p.valueComp, percent);
        link_->setProgress(p.progComp, percent);
    }
}

// ---------------------------------------------------------------------------
// 노브 입력
void DisplayController::selectKnobDelta(int delta) {
    int n = menu_.numEffects();
    int next = (selected_ + delta % n + n) % n;  // 순환
    selectEffect(next, true);
}

void DisplayController::selectKnobFromPot(float norm01) {
    if (norm01 < 0) norm01 = 0; if (norm01 > 1) norm01 = 1;
    int idx = (int)(norm01 * menu_.numEffects());
    if (idx >= menu_.numEffects()) idx = menu_.numEffects() - 1;
    if (idx != selected_) selectEffect(idx, true);
}

void DisplayController::adjustKnobFromPot(float norm01) {
    if (norm01 < 0) norm01 = 0; if (norm01 > 1) norm01 = 1;
    setParamPercent(focusParam_, (int)std::lround(norm01 * 100.0f));
}

void DisplayController::focusNextParam() {
    int n = menu_.effect(selected_).numParams;
    if (n > 0) focusParam_ = (focusParam_ + 1) % n;
    // TODO: 화면에 "현재 포커스 슬롯" 표시(테두리 강조 등) 갱신
}

// ---------------------------------------------------------------------------
// 화면 갱신 헬퍼
void DisplayController::refreshHexHighlights() {
    // 선택된 것만 글로우 그림, 나머지는 일반 그림으로.
    for (int i = 0; i < menu_.numEffects(); ++i) {
        const EffectDef& e = menu_.effect(i);
        if (link_)
            link_->setPicture(e.hexComp, (i == selected_) ? e.picGlow : e.picNormal);
    }
}

void DisplayController::refreshParamRow() {
    const EffectDef& e = menu_.effect(selected_);
    for (int s = 0; s < 4; ++s) {
        if (s < e.numParams) {
            const ParamDef& p = e.params[s];
            int percent = paramPercent_[selected_][s];
            if (link_) {
                link_->setText(p.labelComp, p.label);
                link_->setNumber(p.valueComp, percent);
                link_->setProgress(p.progComp, percent);
            }
        } else {
            // 빈 슬롯: 라벨 비우기
            if (link_) link_->setText((e.params[0].labelComp), ""); // TODO: 슬롯별 빈처리
        }
    }
}

// ---------------------------------------------------------------------------
// 0~100 퍼센트 ↔ 실제값 변환 (디스플레이는 정수만 다루므로 퍼센트 기준)
float DisplayController::percentToReal(const ParamDef& p, int percent) {
    float t = percent / 100.0f;
    return p.min + t * (p.max - p.min);   // 선형. (지수 필요 시 여기 확장)
}

int DisplayController::realToPercent(const ParamDef& p, float real) {
    if (p.max == p.min) return 0;
    float t = (real - p.min) / (p.max - p.min);
    if (t < 0) t = 0; if (t > 1) t = 1;
    return (int)std::lround(t * 100.0f);
}

} // namespace disp

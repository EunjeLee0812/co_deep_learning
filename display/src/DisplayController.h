// DisplayController.h — 디스플레이 앱 로직(상태 머신)
//   - Nextion 터치 이벤트 처리: 육각형 터치 → 이펙트 선택(흰 글로우 전환),
//     하단 슬라이더 조작 → 해당 파라미터 변경
//   - 노브 2개: selectKnob(이펙트 선택), adjustKnob(현재 파라미터 조절)
//   - 변경분을 EffectParamSink 로 외부(엔진)에 송출
//   - 화면을 항상 현재 상태와 동기화 (선택 글로우, 값, 프로그레스바)
//
//  ※ update() 는 보조 태스크(AuxiliaryTask)에서 주기적으로 호출. 오디오 스레드 아님.
#pragma once
#include "NextionLink.h"
#include "EffectMenu.h"
#include "EffectParamSink.h"

namespace disp {

class DisplayController {
public:
    // link: 통신, sink: 외부 송출 대상(엔진). 둘 다 외부 소유.
    void setup(NextionLink* link, EffectParamSink* sink);

    // 보조 태스크에서 주기적으로 호출(예: 5~10ms). 시리얼 폴링 + 대기 작업 처리.
    void update();

    // ---- 물리 노브 입력 (하드웨어 파트가 호출) ----
    void selectKnobDelta(int delta);          // 로터리 인코더: +1/-1 로 이펙트 이동
    void selectKnobFromPot(float norm01);     // 포텐쇼미터: 0~1 → 8개 중 선택
    void adjustKnobFromPot(float norm01);     // 0~1 → 현재 포커스 파라미터 값

    // 현재 포커스(하단 4칸 중 어느 슬롯을 노브로 조절할지) 이동
    void focusNextParam();

    int  selectedEffect() const { return selected_; }

private:
    NextionLink*     link_ = nullptr;
    EffectParamSink* sink_ = nullptr;
    EffectMenu       menu_;

    int   selected_ = 0;          // 현재 선택된 이펙트(0~7)
    int   focusParam_ = 0;        // 현재 노브가 조절하는 하단 슬롯(0~3)
    int   paramPercent_[EffectMenu::kNumEffects][4] = {{0}}; // 각 값 0~100 캐시

    // 내부 동작
    void onNextionEvent(const NextionEvent& ev);
    void selectEffect(int index, bool notifySink);
    void setParamPercent(int slot, int percent);   // 슬롯 값 설정 → 송출 + 화면갱신
    void refreshHexHighlights();                   // 전체 육각형 글로우 갱신
    void refreshParamRow();                        // 하단 4칸 라벨/값/바 갱신

    static float percentToReal(const ParamDef& p, int percent);
    static int   realToPercent(const ParamDef& p, float real);
};

} // namespace disp

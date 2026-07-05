// ChainUiController.h — Nextion 디스플레이 ↔ PluginChain 을 잇는 컨트롤러
// ───────────────────────────────────────────────────────────────────────────
// 역할:
//   1) Nextion → Bela : 바이너리 프레임 파싱 (아래 프로토콜)
//      → PluginChain 편집 연산 호출 (추가/삭제/이동/파라미터/바이패스)
//   2) Bela → Nextion : 체인 그림/선택 하이라이트/파라미터 4칸 갱신
//      (표준 Nextion ASCII 명령 + 0xFF 0xFF 0xFF 종료)
//
// "진실의 원천"은 항상 Bela 쪽(PluginChain)이다. 디스플레이는 터치를 보내고,
// 화면은 Bela 가 다시 그려준다. → 재부팅/재연결 시 SYNC 한 번이면 화면 복구.
//
// ── Nextion → Bela 프로토콜 (모든 프레임: 0xA5 + cmd 1B + 페이로드) ──
//   페이로드의 정수는 Nextion `print 변수.val` 이 보내는 4바이트 리틀엔디언.
//   0x01 ADD    [type:i32]              카테고리→이펙트 선택 완료
//   0x02 REMOVE [slot:i32]              선택된 슬롯 삭제
//   0x03 MOVE   [from:i32][to:i32]      드래그 완료
//   0x04 SELECT [slot:i32]              슬롯 탭 (파라미터 페이지 이 슬롯으로)
//   0x05 PAGE   [page:i32]              파라미터 페이지 넘김 (0..)
//   0x06 PARAM  [row:i32][raw:i32]      현재 페이지의 row(0..3) 슬라이더 = raw(0..1000)
//   0x07 BYPASS [on:i32]                선택 슬롯 바이패스 토글 (1=바이패스)
//   0x08 SYNC   (페이로드 없음)          부팅/페이지 진입 시 전체 화면 재전송 요청
//
// update() 는 AuxiliaryTask 에서 5~10ms 주기로 호출 (오디오 스레드 아님!).
// ───────────────────────────────────────────────────────────────────────────
#pragma once
#include "SerialPort.h"
#include "EffectDefs.h"
#include "../../effects/PluginChain.h"

namespace disp {

class ChainUiController {
public:
    // device 예: "/dev/ttyS4" (연결 가이드 참고), baud 는 Nextion 과 일치 (기본 115200)
    bool setup(fx::PluginChain* chain, const char* device, int baud = 115200);
    void cleanup();

    // 보조 태스크에서 주기 호출: 수신 파싱 + 체인 반영 + 필요 시 화면 갱신 + GC
    void update();

private:
    fx::PluginChain* chain_ = nullptr;
    SerialPort       serial_;

    // UI 상태 (Bela 가 관리하는 진실)
    int selected_  = -1;   // 선택된 슬롯 (-1 = 없음)
    int paramPage_ = 0;    // 파라미터 페이지 (4개씩)

    // 각 슬롯의 파라미터 "현재값" 캐시 (화면 복원/슬라이더 초기화용)
    float paramCache_[fx::PluginChain::kMaxSlots][16];
    void  initSlotCache(int slot);        // 새 슬롯 → 기본값 채우고 이펙트에도 적용

    // ── 수신 파서 ──
    unsigned char rx_[64];
    int  rxLen_ = 0;
    void pumpSerial();                    // 읽고 프레임 완성되면 handleFrame
    static int payloadLen(unsigned char cmd);
    void handleFrame(unsigned char cmd, const unsigned char* pl);
    static int32_t i32(const unsigned char* p) {
        return (int32_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8) |
                         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24));
    }

    // ── 명령 처리 ──
    void doAdd(int type);
    void doRemove(int slot);
    void doMove(int from, int to);
    void doSelect(int slot);
    void doPage(int page);
    void doParam(int row, int raw);
    void doBypass(bool on);

    // ── 송신 (Nextion 그리기) ──
    void nxSend(const char* cmd);                     // 명령 + FF FF FF
    void nxSendf(const char* fmt, ...);               // printf 스타일
    void redrawAll();                                 // 체인 + 선택 + 파라미터 전부
    void redrawChain();                               // 아이콘 8칸 + 하이라이트
    void redrawParams();                              // 라벨/슬라이더/값 4칸 + 헤더
    void redrawValueText(int row, const ParamDef& p, float real);
};

} // namespace disp

// NextionLink.h — Nextion 디스플레이와의 통신 계층
//  보내기: setText/setNumber/setPicture/sendCommand (자동으로 0xFF x3 부착)
//  받기  : poll() 로 시리얼을 읽어 완성된 메시지를 NextionEvent 로 파싱 → 콜백
//  ※ 시리얼이 블로킹될 수 있으니 오디오 스레드가 아닌 보조 태스크에서 호출할 것.
#pragma once
#include "SerialPort.h"
#include <functional>
#include <cstdint>

namespace disp {

// 파싱된 한 개의 디스플레이 이벤트
struct NextionEvent {
    enum class Type { None, Touch, Number, String, Other };
    Type type = Type::None;
    // Touch 일 때
    uint8_t page = 0;
    uint8_t component = 0;   // 컴포넌트 번호(.id) — Editor 와 일치해야 함
    uint8_t touchEvent = 0;  // 0=release, 1=press
    // Number 일 때 (get 결과)
    int32_t number = 0;
    // String 일 때
    char    text[64] = {0};
    // 원본 코드(첫 바이트)
    uint8_t rawCode = 0;
};

class NextionLink {
public:
    bool setup(const char* device, int baud);
    void cleanup();

    // ---- 보내기 (화면 갱신) ----
    void sendCommand(const char* cmd);                 // 임의 명령 + 종료자
    void setText(const char* comp, const char* value); // 예: t0.txt="REVERB"
    void setNumber(const char* comp, int value);       // 예: n0.val=50
    void setProgress(const char* comp, int value0to100);// 예: j0.val=50 (프로그레스바)
    void setPicture(const char* comp, int picId);      // 예: b0.pic=3 (선택 글로우 전환)

    // ---- 받기 ----
    // 매 주기 호출: 시리얼 버퍼를 읽어 완성된 메시지마다 핸들러 호출.
    void poll();
    void setEventHandler(std::function<void(const NextionEvent&)> h) { handler_ = std::move(h); }

private:
    SerialPort serial_;
    std::function<void(const NextionEvent&)> handler_;

    // 수신 누적 버퍼 (메시지는 0xFF 0xFF 0xFF 로 끝남)
    unsigned char rx_[256];
    int rxLen_ = 0;

    void dispatchMessage(const unsigned char* payload, int len); // FF 제외한 본문
};

} // namespace disp

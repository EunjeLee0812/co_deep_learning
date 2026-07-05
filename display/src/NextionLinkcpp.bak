#include "NextionLink.h"
#include "NextionProtocol.h"
#include <cstring>
#include <cstdio>

namespace disp {

bool NextionLink::setup(const char* device, int baud) {
    rxLen_ = 0;
    if (!serial_.open(device, baud)) return false;
    // 시작 시 권장: 디스플레이 재초기화/페이지 진입 등을 여기서 보낼 수 있음
    sendCommand("bkcmd=1");   // 명령 성공/실패 응답 수준 (디버깅 시 3)
    return true;
}

void NextionLink::cleanup() { serial_.close(); }

// ---- 보내기 ----
void NextionLink::sendCommand(const char* cmd) {
    if (!serial_.isOpen()) return;
    serial_.writeBytes(reinterpret_cast<const unsigned char*>(cmd), std::strlen(cmd));
    const unsigned char term[3] = { nx::kTerminator, nx::kTerminator, nx::kTerminator };
    serial_.writeBytes(term, 3);
}

void NextionLink::setText(const char* comp, const char* value) {
    char buf[96];
    std::snprintf(buf, sizeof(buf), "%s.txt=\"%s\"", comp, value);
    sendCommand(buf);
}

void NextionLink::setNumber(const char* comp, int value) {
    char buf[48];
    std::snprintf(buf, sizeof(buf), "%s.val=%d", comp, value);
    sendCommand(buf);
}

void NextionLink::setProgress(const char* comp, int value0to100) {
    if (value0to100 < 0) value0to100 = 0;
    if (value0to100 > 100) value0to100 = 100;
    setNumber(comp, value0to100); // 프로그레스바(j)도 .val 사용
}

void NextionLink::setPicture(const char* comp, int picId) {
    char buf[48];
    std::snprintf(buf, sizeof(buf), "%s.pic=%d", comp, picId);
    sendCommand(buf);
}

// ---- 받기 ----
void NextionLink::poll() {
    if (!serial_.isOpen()) return;

    unsigned char chunk[128];
    int n = serial_.readBytes(chunk, sizeof(chunk));
    if (n <= 0) return;

    for (int i = 0; i < n; ++i) {
        if (rxLen_ < (int)sizeof(rx_))
            rx_[rxLen_++] = chunk[i];
        else
            rxLen_ = 0; // 오버플로 방지: 버리고 재동기화

        // 메시지 종료: 마지막 3바이트가 모두 0xFF
        if (rxLen_ >= 3 &&
            rx_[rxLen_-1] == nx::kTerminator &&
            rx_[rxLen_-2] == nx::kTerminator &&
            rx_[rxLen_-3] == nx::kTerminator) {
            dispatchMessage(rx_, rxLen_ - 3); // 종료자 제외한 본문 전달
            rxLen_ = 0;
        }
    }
}

void NextionLink::dispatchMessage(const unsigned char* payload, int len) {
    if (len <= 0 || !handler_) return;

    NextionEvent ev;
    ev.rawCode = payload[0];

    switch (payload[0]) {
        case nx::kRetTouchEvent: // 0x65 page comp event
            if (len >= 4) {
                ev.type = NextionEvent::Type::Touch;
                ev.page       = payload[1];
                ev.component  = payload[2];
                ev.touchEvent = payload[3];
            }
            break;
        case nx::kRetNumberData: // 0x71 + int32 little-endian
            if (len >= 5) {
                ev.type = NextionEvent::Type::Number;
                ev.number = (int32_t)( payload[1]
                          | (payload[2] << 8)
                          | (payload[3] << 16)
                          | (payload[4] << 24));
            }
            break;
        case nx::kRetStringData: // 0x70 + chars
            {
                ev.type = NextionEvent::Type::String;
                int copy = len - 1;
                if (copy > (int)sizeof(ev.text) - 1) copy = sizeof(ev.text) - 1;
                std::memcpy(ev.text, payload + 1, copy);
                ev.text[copy] = '\0';
            }
            break;
        default:
            ev.type = NextionEvent::Type::Other;
            break;
    }
    handler_(ev);
}

} // namespace disp

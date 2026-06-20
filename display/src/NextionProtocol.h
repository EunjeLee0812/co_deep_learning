// NextionProtocol.h — Nextion 시리얼 프로토콜 정의
// 모든 명령/반환 데이터는 0xFF 0xFF 0xFF 세 바이트로 끝난다.
#pragma once
#include <cstdint>

namespace disp {
namespace nx {

// 명령 종료 바이트 (3개)
static constexpr unsigned char kTerminator = 0xFF;

// ---- Nextion → 호스트 반환 코드 (Return Data) ----
// "Send Component ID" 가 켜진 컴포넌트 터치 시: 0x65 page comp event FF FF FF
static constexpr unsigned char kRetTouchEvent   = 0x65; // 터치(누름/뗌)
static constexpr unsigned char kRetCurrentPage  = 0x66; // sendme 결과
static constexpr unsigned char kRetNumberData   = 0x71; // get 숫자 결과(4바이트 LE)
static constexpr unsigned char kRetStringData   = 0x70; // get 문자열 결과
static constexpr unsigned char kRetTouchCoord   = 0x67; // 터치 좌표
static constexpr unsigned char kRetSleepAuto    = 0x86; // 자동 슬립 진입
static constexpr unsigned char kRetWakeAuto     = 0x87; // 자동 웨이크

// 터치 이벤트 종류
static constexpr unsigned char kTouchRelease = 0x00;
static constexpr unsigned char kTouchPress   = 0x01;

} // namespace nx
} // namespace disp

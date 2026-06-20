// SerialPort.h — 리눅스 UART 포트 래퍼 (termios). Nextion 과의 시리얼 통신용.
// Bela 는 리눅스 위에서 돌기 때문에 표준 termios 로 시리얼 장치를 연다.
//   - Bela 보드의 UART 핀을 쓰거나(장치명은 보드마다 다름)
//   - USB-시리얼 어댑터(/dev/ttyUSB0)를 쓰면 가장 간단하다.
// 주의: 이 클래스의 read/write 는 블로킹될 수 있으므로 오디오 스레드가 아니라
//       별도 태스크(AuxiliaryTask)에서 호출해야 한다.
#pragma once
#include <cstddef>

namespace disp {

class SerialPort {
public:
    ~SerialPort();
    // device 예: "/dev/ttyUSB0", "/dev/ttyS4". baud 예: 9600, 115200, 921600.
    bool open(const char* device, int baud);
    void close();
    bool isOpen() const { return fd_ >= 0; }

    // 논블로킹 읽기. 읽은 바이트 수 반환(0이면 데이터 없음, <0이면 에러).
    int  readBytes(unsigned char* buf, size_t maxLen);
    // 쓰기. 보낸 바이트 수 반환.
    int  writeBytes(const unsigned char* buf, size_t len);

private:
    int fd_ = -1;
};

} // namespace disp

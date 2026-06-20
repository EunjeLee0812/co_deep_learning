#include "SerialPort.h"
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <cstdio>

namespace disp {

// 정수 baud → termios speed_t 매핑
static speed_t toSpeed(int baud) {
    switch (baud) {
        case 9600:   return B9600;
        case 19200:  return B19200;
        case 38400:  return B38400;
        case 57600:  return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
        case 460800: return B460800;
        case 921600: return B921600;
        default:     return B9600; // 안전 기본값
    }
}

SerialPort::~SerialPort() { close(); }

bool SerialPort::open(const char* device, int baud) {
    fd_ = ::open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) {
        printf("[SerialPort] '%s' 열기 실패\n", device);
        return false;
    }
    termios tty{};
    if (tcgetattr(fd_, &tty) != 0) { close(); return false; }

    cfsetospeed(&tty, toSpeed(baud));
    cfsetispeed(&tty, toSpeed(baud));

    // 8N1, 로컬 라인, 수신 허용
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag &= ~PARENB;          // no parity
    tty.c_cflag &= ~CSTOPB;          // 1 stop bit
    tty.c_cflag &= ~CRTSCTS;         // no HW flow control
    tty.c_cflag |= (CLOCAL | CREAD);

    // raw 모드 (가공 없이 바이트 그대로)
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | INLCR | ISTRIP);
    tty.c_oflag &= ~OPOST;

    // 논블로킹: 즉시 반환
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(fd_, TCSANOW, &tty) != 0) { close(); return false; }
    tcflush(fd_, TCIOFLUSH);
    return true;
}

void SerialPort::close() {
    if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
}

int SerialPort::readBytes(unsigned char* buf, size_t maxLen) {
    if (fd_ < 0) return -1;
    return (int)::read(fd_, buf, maxLen);
}

int SerialPort::writeBytes(const unsigned char* buf, size_t len) {
    if (fd_ < 0) return -1;
    return (int)::write(fd_, buf, len);
}

} // namespace disp

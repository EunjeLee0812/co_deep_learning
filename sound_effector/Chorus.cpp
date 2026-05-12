#include "Chorus.h"
#include <cmath>
#include <algorithm>

Chorus::Chorus() : gWritePtr(0), gLfoPhase(0.0f), gRate(0.5f), gDepth(0.002f), gMix(0.5f) {}

void Chorus::setup(float sampleRate) {
    gSampleRate = sampleRate;
    // 코러스는 보통 30ms 이내의 짧은 지연을 사용하므로 0.1초 정도의 버퍼면 충분합니다.
    gBuffer.assign((unsigned int)(sampleRate * 0.1f), 0.0f);
}

void Chorus::setRate(float hz) {
    gRate = hz;
}

void Chorus::setDepth(float ms) {
    // 밀리초를 초 단위로 변환 (보통 1~5ms 정도가 적당함)
    gDepth = ms / 1000.0f;
}

void Chorus::setMix(float m) {
    if (m < 0.0f) gMix = 0.0f;
    else if (m > 1.0f) gMix = 1.0f;
    else gMix = m;
}

float Chorus::process(float input) {
    // 1. LFO 위상 전진
    gLfoPhase += 2.0f * M_PI * gRate / gSampleRate;
    if (gLfoPhase >= 2.0f * M_PI) gLfoPhase -= 2.0f * M_PI;

    // 2. 변조된 딜레이 시간 계산 (Base Delay 20ms + LFO 변조)
    float baseDelay = 0.020f; 
    float modDelay = baseDelay + (sinf(gLfoPhase) * gDepth);
    float delaySamples = modDelay * gSampleRate;

    // 3. 읽기 포인터 계산 및 선형 보간 (Linear Interpolation)
    float readPtr = (float)gWritePtr - delaySamples;
    while (readPtr < 0) readPtr += gBuffer.size();

    int iPart = (int)readPtr;              // 정수 부분
    float fPart = readPtr - iPart;         // 소수 부분
    int iPartNext = (iPart + 1) % gBuffer.size();

    // 인접한 두 샘플을 섞어 부드러운 소리를 만듭니다.
    float wet = gBuffer[iPart] * (1.0f - fPart) + gBuffer[iPartNext] * fPart;

    // 4. 버퍼 업데이트
    gBuffer[gWritePtr] = input;
    gWritePtr = (gWritePtr + 1) % gBuffer.size();

    // 5. Dry/Wet Mix 출력
    return (input * (1.0f - gMix)) + (wet * gMix);
}
// Dsp.h — 모든 이펙트가 공유하는 실시간 안전 DSP 빌딩블록 모음.
// ───────────────────────────────────────────────────────────────────────────
// 설계 원칙
//  - 렌더 스레드(process)에서는 절대 heap 할당 금지. 버퍼 할당은 setup()에서만.
//  - 계수(coeff) 재계산은 "파라미터가 바뀔 때만" 한다(매 샘플 X). 각 필터의 set*()가 그 역할.
//  - 모든 클래스는 헤더 inline 구현. (Bela 는 전 .cpp 를 모아 빌드하므로 헤더 inline 이 안전)
//
// 들어있는 것:
//   dB <-> linear 변환,  OnePole(LP/HP),  Biquad(RBJ: LP/HP/BP/peak/shelf),
//   StateVariableFilter(TPT, LP/BP/HP 동시 출력 → 디스토션 모핑 EQ용),
//   DelayLine(분수지연 선형보간),  Lfo(사인/삼각, 위상오프셋).
// ───────────────────────────────────────────────────────────────────────────
#pragma once
#include <vector>
#include <cmath>
#include <cstdint>

namespace fx {
namespace dsp {

constexpr float kPi  = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;

// dB(데시벨) → 선형 진폭.  0dB→1.0,  -6dB→0.5,  +6dB→2.0
inline float dbToLin(float db) { return std::pow(10.0f, db * 0.05f); }
inline float linToDb(float lin) { return 20.0f * std::log10(lin > 1e-9f ? lin : 1e-9f); }

inline float clampf(float x, float lo, float hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}

// ───────────────────────────────────────────────────────────────────────────
// OnePole — 1차 필터. low/high cut(컷오프 부드러운 6dB/oct) 용도.
//   y += a*(x - y)  형태. a 는 컷오프로 결정.
// ───────────────────────────────────────────────────────────────────────────
class OnePole {
public:
    void setSampleRate(float fs) { fs_ = fs; updateCoeff(); }
    void setCutoff(float hz) {
        cutoff_ = clampf(hz, 1.0f, fs_ * 0.49f);
        updateCoeff();
    }
    // 저역통과 출력
    inline float lp(float x) {
        z_ += a_ * (x - z_);
        return z_;
    }
    // 고역통과 출력 = 입력 - 저역통과
    inline float hp(float x) {
        z_ += a_ * (x - z_);
        return x - z_;
    }
    void reset() { z_ = 0.0f; }

private:
    void updateCoeff() {
        // a = 1 - exp(-2π fc / fs)
        a_ = 1.0f - std::exp(-kTwoPi * cutoff_ / fs_);
    }
    float fs_ = 44100.0f;
    float cutoff_ = 1000.0f;
    float a_ = 0.1f;
    float z_ = 0.0f;
};

// ───────────────────────────────────────────────────────────────────────────
// Biquad — RBJ cookbook 2차 필터. (transposed direct form II)
//   set*() 로 계수 갱신(파라미터 변경 시에만 호출). process()는 매 샘플.
// ───────────────────────────────────────────────────────────────────────────
class Biquad {
public:
    void setSampleRate(float fs) { fs_ = fs; }
    void reset() { z1_ = z2_ = 0.0f; }

    inline float process(float x) {
        float y = b0_ * x + z1_;
        z1_ = b1_ * x - a1_ * y + z2_;
        z2_ = b2_ * x - a2_ * y;
        return y;
    }

    void setLowpass(float hz, float Q) {
        float w0, c, s, alpha; basics(hz, Q, w0, c, s, alpha);
        float b0 = (1 - c) * 0.5f, b1 = 1 - c, b2 = (1 - c) * 0.5f;
        float a0 = 1 + alpha, a1 = -2 * c, a2 = 1 - alpha;
        normalize(b0, b1, b2, a0, a1, a2);
    }
    void setHighpass(float hz, float Q) {
        float w0, c, s, alpha; basics(hz, Q, w0, c, s, alpha);
        float b0 = (1 + c) * 0.5f, b1 = -(1 + c), b2 = (1 + c) * 0.5f;
        float a0 = 1 + alpha, a1 = -2 * c, a2 = 1 - alpha;
        normalize(b0, b1, b2, a0, a1, a2);
    }
    void setBandpass(float hz, float Q) {  // 0dB 피크(constant skirt) 형태
        float w0, c, s, alpha; basics(hz, Q, w0, c, s, alpha);
        float b0 = alpha, b1 = 0.0f, b2 = -alpha;
        float a0 = 1 + alpha, a1 = -2 * c, a2 = 1 - alpha;
        normalize(b0, b1, b2, a0, a1, a2);
    }
    void setPeak(float hz, float Q, float gainDb) {
        float A = std::pow(10.0f, gainDb / 40.0f);
        float w0, c, s, alpha; basics(hz, Q, w0, c, s, alpha);
        float b0 = 1 + alpha * A, b1 = -2 * c, b2 = 1 - alpha * A;
        float a0 = 1 + alpha / A, a1 = -2 * c, a2 = 1 - alpha / A;
        normalize(b0, b1, b2, a0, a1, a2);
    }
    void setLowShelf(float hz, float gainDb, float S = 1.0f) {
        float A = std::pow(10.0f, gainDb / 40.0f);
        float w0 = kTwoPi * hz / fs_, c = std::cos(w0), s = std::sin(w0);
        float alpha = s * 0.5f * std::sqrt((A + 1 / A) * (1 / S - 1) + 2);
        float ta = 2 * std::sqrt(A) * alpha;
        float b0 =    A * ((A + 1) - (A - 1) * c + ta);
        float b1 = 2 * A * ((A - 1) - (A + 1) * c);
        float b2 =    A * ((A + 1) - (A - 1) * c - ta);
        float a0 =        (A + 1) + (A - 1) * c + ta;
        float a1 =   -2 * ((A - 1) + (A + 1) * c);
        float a2 =        (A + 1) + (A - 1) * c - ta;
        normalize(b0, b1, b2, a0, a1, a2);
    }
    void setHighShelf(float hz, float gainDb, float S = 1.0f) {
        float A = std::pow(10.0f, gainDb / 40.0f);
        float w0 = kTwoPi * hz / fs_, c = std::cos(w0), s = std::sin(w0);
        float alpha = s * 0.5f * std::sqrt((A + 1 / A) * (1 / S - 1) + 2);
        float ta = 2 * std::sqrt(A) * alpha;
        float b0 =    A * ((A + 1) + (A - 1) * c + ta);
        float b1 = -2 * A * ((A - 1) + (A + 1) * c);
        float b2 =    A * ((A + 1) + (A - 1) * c - ta);
        float a0 =        (A + 1) - (A - 1) * c + ta;
        float a1 =    2 * ((A - 1) - (A + 1) * c);
        float a2 =        (A + 1) - (A - 1) * c - ta;
        normalize(b0, b1, b2, a0, a1, a2);
    }
    // 계수를 통과(bypass, 1.0 게인)로 — 밴드 off 시
    void setPassthrough() {
        b0_ = 1.0f; b1_ = b2_ = a1_ = a2_ = 0.0f;
    }

private:
    void basics(float hz, float Q, float& w0, float& c, float& s, float& alpha) {
        hz = clampf(hz, 10.0f, fs_ * 0.49f);
        Q  = Q < 0.05f ? 0.05f : Q;
        w0 = kTwoPi * hz / fs_;
        c  = std::cos(w0);
        s  = std::sin(w0);
        alpha = s / (2.0f * Q);
    }
    void normalize(float b0, float b1, float b2, float a0, float a1, float a2) {
        float inv = 1.0f / a0;
        b0_ = b0 * inv; b1_ = b1 * inv; b2_ = b2 * inv;
        a1_ = a1 * inv; a2_ = a2 * inv;
    }
    float fs_ = 44100.0f;
    float b0_ = 1, b1_ = 0, b2_ = 0, a1_ = 0, a2_ = 0;
    float z1_ = 0, z2_ = 0;
};

// ───────────────────────────────────────────────────────────────────────────
// StateVariableFilter — TPT(Topology Preserving Transform) SVF.
//   한 번 process 로 LP/BP/HP 를 동시에 얻는다. 디스토션의 "모핑 EQ"처럼
//   하나의 주파수/레조넌스로 LP↔BP↔HP 를 연속 블렌딩할 때 핵심.
//   (Andrew Simper / Cytomic 공식)
// ───────────────────────────────────────────────────────────────────────────
class StateVariableFilter {
public:
    void setSampleRate(float fs) { fs_ = fs; update(); }
    void setCutoff(float hz)     { cutoff_ = clampf(hz, 10.0f, fs_ * 0.49f); update(); }
    void setResonance(float r)   { // 0..1 → Q 약 0.5..~20
        float Q = 0.5f + r * 19.5f;
        k_ = 1.0f / Q;
        update();
    }
    void reset() { ic1_ = ic2_ = 0.0f; }

    // 한 샘플 처리. lp/bp/hp 에 세 출력이 채워진다.
    inline void process(float x, float& lp, float& bp, float& hp) {
        float v3 = x - ic2_;
        float v1 = a1_ * ic1_ + a2_ * v3;
        float v2 = ic2_ + a2_ * ic1_ + a3_ * v3;
        ic1_ = 2.0f * v1 - ic1_;
        ic2_ = 2.0f * v2 - ic2_;
        bp = v1;
        lp = v2;
        hp = x - k_ * v1 - v2;
    }

private:
    void update() {
        float g = std::tan(kPi * cutoff_ / fs_);
        a1_ = 1.0f / (1.0f + g * (g + k_));
        a2_ = g * a1_;
        a3_ = g * a2_;
    }
    float fs_ = 44100.0f, cutoff_ = 1000.0f, k_ = 1.0f;
    float a1_ = 0, a2_ = 0, a3_ = 0;
    float ic1_ = 0, ic2_ = 0;
};

// ───────────────────────────────────────────────────────────────────────────
// DelayLine — 원형 버퍼 + 선형보간 분수지연. 딜레이/코러스/플랜저/리버브 공용.
//   사용법(피드백 딜레이): 매 샘플  float w = read(d);  ...;  write(in + w*fb);
//   read(d) 는 "가장 최근 write 로부터 d 샘플 이전" 값을 보간 반환.
// ───────────────────────────────────────────────────────────────────────────
class DelayLine {
public:
    // maxDelaySamples 이상 저장 가능하도록 2의 거듭제곱으로 할당(마스크 wrap).
    void setup(int maxDelaySamples) {
        int need = maxDelaySamples + 4;
        int sz = 1;
        while (sz < need) sz <<= 1;
        buf_.assign((size_t)sz, 0.0f);
        size_ = sz;
        mask_ = sz - 1;
        w_ = 0;
    }
    void reset() {
        std::fill(buf_.begin(), buf_.end(), 0.0f);
        w_ = 0;
    }
    inline void write(float x) {
        buf_[(size_t)w_] = x;
        w_ = (w_ + 1) & mask_;
    }
    // d = 지연 샘플수(실수 가능). d=0 이면 직전 write 한 값.
    inline float read(float d) const {
        if (d < 0.0f) d = 0.0f;
        float rp = (float)w_ - 1.0f - d;
        while (rp < 0.0f) rp += (float)size_;
        int i0 = (int)rp;
        float frac = rp - (float)i0;
        int i1 = (i0 + 1) & mask_;
        i0 &= mask_;
        return buf_[(size_t)i0] + frac * (buf_[(size_t)i1] - buf_[(size_t)i0]);
    }
    int maxDelay() const { return size_ - 4; }

private:
    std::vector<float> buf_;
    int size_ = 0, mask_ = 0, w_ = 0;
};

// ───────────────────────────────────────────────────────────────────────────
// Lfo — 저주파 오실레이터. 사인/삼각. 위상오프셋으로 스테레오 분산.
// ───────────────────────────────────────────────────────────────────────────
class Lfo {
public:
    enum class Shape { Sine, Triangle };

    void setSampleRate(float fs) { fs_ = fs; }
    void setRate(float hz)       { inc_ = hz / fs_; }
    void setShape(Shape s)       { shape_ = s; }
    void reset(float phase01 = 0.0f) { phase_ = phase01; }

    // 한 샘플 진행. -1..+1 반환. phaseOffset01 로 같은 LFO 의 다른 위상 탭을 뽑을 수 있음.
    inline float next() {
        float v = shapeAt(phase_);
        phase_ += inc_;
        if (phase_ >= 1.0f) phase_ -= 1.0f;
        return v;
    }
    // 진행 없이 현재 위상에서 offset 만큼 떨어진 값(스테레오 우채널 등)
    inline float tap(float phaseOffset01) const {
        float p = phase_ + phaseOffset01;
        p -= std::floor(p);
        return shapeAt(p);
    }

private:
    inline float shapeAt(float p) const {
        if (shape_ == Shape::Sine) return std::sin(kTwoPi * p);
        // 삼각: -1..+1
        float t = p < 0.5f ? p * 2.0f : (1.0f - p) * 2.0f; // 0..1..0
        return t * 2.0f - 1.0f;
    }
    float fs_ = 44100.0f;
    float phase_ = 0.0f, inc_ = 0.0f;
    Shape shape_ = Shape::Sine;
};

// ───────────────────────────────────────────────────────────────────────────
// 노트분할 → 1사이클 길이(초). BPM sync 용. (딜레이/모듈레이션 공용)
//   div: 0=1/1, 1=1/2, 2=1/4, 3=1/8, 4=1/16, 5=점8분(dotted), 6=3연8분(triplet)
// ───────────────────────────────────────────────────────────────────────────
inline float noteDivSeconds(int div, float bpm) {
    float quarter = 60.0f / (bpm > 1.0f ? bpm : 1.0f); // 4분음표 길이(초)
    switch (div) {
        case 0: return quarter * 4.0f;   // 온음표
        case 1: return quarter * 2.0f;   // 2분
        case 2: return quarter;          // 4분
        case 3: return quarter * 0.5f;   // 8분
        case 4: return quarter * 0.25f;  // 16분
        case 5: return quarter * 0.75f;  // 점8분
        case 6: return quarter / 3.0f;   // 3연8분
        default: return quarter;
    }
}

} // namespace dsp
} // namespace fx

// CommHandler.h — 디스플레이 -> Bela 통신 (수신측)
// 디스플레이에서 보낸 설정/이펙터 파라미터 메시지를 받아 엔진에 적용
#pragma once
class SynthEngine;

class CommHandler {
public:
    bool setup();                       // 통신 채널 초기화 (OSC/UDP/시리얼 등 — 추후 확정)
    void poll(SynthEngine& engine);     // 들어온 메시지 파싱 -> engine.setParam(...)
    void cleanup();
};

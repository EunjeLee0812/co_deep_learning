// render.cpp — Bela 진입점 (setup / render / cleanup)
// 실시간 오디오 콜백. 여기서 각 파트를 조립한다.
#include <Bela.h>
#include "synthesis/SynthEngine.h"
#include "hardware/HardwareInput.h"
#include "comm/CommHandler.h"

static SynthEngine    gSynth;
static HardwareInput  gHardware;
static CommHandler    gComm;   // 디스플레이로부터 파라미터 수신

bool setup(BelaContext* context, void* userData) {
    if (!gSynth.setup(context->audioSampleRate)) return false;
    if (!gHardware.setup(context)) return false;
    if (!gComm.setup()) return false;
    return true;
}

void render(BelaContext* context, void* userData) {
    // 1) 디스플레이에서 들어온 파라미터 변경 반영
    gComm.poll(gSynth);
    // 2) 하드웨어(센서/버튼 등) 입력 반영
    gHardware.poll(context, gSynth);
    // 3) 오디오 블록 생성
    for (unsigned int n = 0; n < context->audioFrames; n++) {
        float out = gSynth.process();          // instruments -> effects -> mix
        for (unsigned int ch = 0; ch < context->audioOutChannels; ch++)
            audioWrite(context, n, ch, out);
    }
}

void cleanup(BelaContext* context, void* userData) {
    gComm.cleanup();
    gHardware.cleanup();
    gSynth.cleanup();
}

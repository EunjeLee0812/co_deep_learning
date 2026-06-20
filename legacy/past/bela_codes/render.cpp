#include <Bela.h>
#include <cmath>
#include <Watcher.h>
#include <libraries/Trill/Trill.h> // Trill Bar 센서를 사용하기 위한 헤더

// 1. 파이썬으로 보낼 Watcher 변수 선언 (파이썬에서도 이 이름으로 받습니다)
Watcher<float> touchLocation("touch_location");
Watcher<float> touchSize("touch_size");

// Trill 객체 생성
Trill trillBar;

// 오디오 관련 변수
float gFrequency = 440.0;
float gPhase;
float gInverseSampleRate;

bool setup(BelaContext *context, void *userData)
{
    // Watcher 통신 서버 초기화
    Bela_getDefaultWatcherManager()->setup(context->audioSampleRate);
    Bela_getDefaultWatcherManager()->getGui().setup(context->projectName);
    
    // Trill Bar 초기화 (I2C 주소 1번, BAR 타입)
    if(trillBar.setup(1, Trill::BAR) != 0) {
        rt_printf("Trill Bar 연결 실패! 배선을 확인하세요.\n");
        return false;
    }

    gInverseSampleRate = 1.0 / context->audioSampleRate;
    gPhase = 0.0;
    return true;
}

void render(BelaContext *context, void *userData)
{
    // 1. 센서 값 읽기 (블록당 1번)
    trillBar.readI2C();
    
    float currentLoc = 0.0;
    float currentSize = 0.0;

    if(trillBar.getNumTouches() > 0) {
        currentLoc = trillBar.touchLocation(0);
        currentSize = trillBar.touchSize(0);    
    }

    // 2. 파이썬과 시간 동기화 및 데이터 전송 (루프 밖에서 블록당 1번!)
    // 루프 밖이므로 n 대신 true를 넣어 새로운 블록이 시작되었음을 알립니다.
    Bela_getDefaultWatcherManager()->tick(context->audioFramesElapsed, true);
    touchLocation = currentLoc;
    touchSize = currentSize;

    // -----------------------------------------------------
    // 💡 파이썬에서 받은 값을 읽어 gFrequency를 업데이트하는 코드도 루프 밖에 두는 것이 좋습니다.
    // -----------------------------------------------------

    // 3. 오디오 샘플 처리 루프 (오디오 합성만 담당)
    for(unsigned int n = 0; n < context->audioFrames; n++) {
        
        // 기본 소리 합성 (삐~ 소리)
        float out = 0.8 * sinf(gPhase);
        gPhase += 2.0 * (float)M_PI * gFrequency * gInverseSampleRate;
        if(gPhase > 2.0 * (float)M_PI) gPhase -= 2.0 * (float)M_PI;

        for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
            audioWrite(context, n, channel, out);
        }
    }
}

void cleanup(BelaContext *context, void *userData)
{
}

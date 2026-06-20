#include <Bela.h>
#include <cmath>
#include <libraries/Trill/Trill.h>
#include <Watcher.h>

// 1. 파이썬으로 보낼 Watcher 변수 선언 (Input: 내부 값, Output: 네트워크 패킷)
// 직관성을 위해 변수명을 바꿨습니다. 나중에 파이썬에서 variables=["touchLoc", "touchSize"]로 받으시면 됩니다.
Watcher<float> touchLoc("touchLoc");
Watcher<float> touchSize("touchSize");

// 2. Trill 객체 및 백그라운드 데이터 저장용 전역 변수
Trill touchSensor;
float gTouchLocation = 0.0;
float gTouchSize = 0.0;
unsigned int gNumActiveTouches = 0;
unsigned int gTaskSleepTime = 12000; // 12ms 대기

// 3. Auxiliary Task (생산자 스레드): I2C 통신으로 센서 값을 읽어 전역 변수에 저장
void loop(void*)
{
    while(!Bela_stopRequested())
    {
        touchSensor.readI2C(); // 센서 업데이트 (블로킹 함수)
        gNumActiveTouches = touchSensor.getNumTouches();
        
        if(gNumActiveTouches > 0) {
            // 첫 번째 손가락(Index 0)의 위치와 크기만 가져옵니다.
            gTouchLocation = touchSensor.touchLocation(0);
            gTouchSize = touchSensor.touchSize(0);
        } else {
            // 손을 떼면 -0.1로 초기화하여 파이썬 그래프에서 터치 안 됨을 명확히 표시
            gTouchLocation = -0.1; 
            gTouchSize = 0.0;
        }
        usleep(gTaskSleepTime); // 12ms 대기
    }
}

// 4. 초기화 함수
bool setup(BelaContext *context, void *userData)
{
    // Watcher 초기화 (파이썬 통신 준비)
    Bela_getDefaultWatcherManager()->getGui().setup(context->projectName);
    Bela_getDefaultWatcherManager()->setup(context->audioSampleRate);

    // Trill 센서 초기화 (I2C 버스 1번, BAR 타입)
    if(touchSensor.setup(1, Trill::BAR) != 0) {
        fprintf(stderr, "Unable to initialise Trill Bar\n");
        return false;
    }
    touchSensor.setMinimumTouchSize(0.1); // 노이즈 제거
    usleep(10000);

    // I2C 데이터를 읽는 백그라운드 스레드 실행
    Bela_runAuxiliaryTask(loop);

    return true;
}

// 5. 오디오 실시간 루프 (소비자 스레드): 전역 변수 값을 Watcher에 담아 파이썬으로 전송
void render(BelaContext *context, void *userData)
{
    // 매 오디오 프레임마다 파이썬으로 보내면 데이터가 너무 많아 버퍼가 터집니다.
    // 64 프레임(약 1.4ms)에 한 번씩만 Watcher에 데이터를 업데이트합니다.
    int streamRate = 512; 

    for(unsigned int n = 0; n < context->audioFrames; n++) {
        
        if((n % streamRate) == 0) {
            // 타임스탬프 기록
            uint64_t frames = context->audioFramesElapsed + n;
            Bela_getDefaultWatcherManager()->tick(frames); 
            
            // 전역 변수 값을 Watcher에 대입하는 순간 파이썬으로 데이터가 전송됩니다.
            touchLoc = gTouchLocation;
            touchSize = gTouchSize;
        }
    }
}

void cleanup(BelaContext *context, void *userData)
{
}
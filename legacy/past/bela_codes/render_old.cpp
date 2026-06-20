// #include <Bela.h>
// #include <cmath>
// #include <Watcher.h>
// #include <libraries/Trill/Trill.h>

// Watcher<float> touchLocation("touch_location");
// Watcher<float> touchSize("touch_size");

// Trill trillBar;
// AuxiliaryTask i2cTask; // ✅ 추가: 백그라운드 작업을 위한 변수

// float gFrequency = 440.0;
// float gPhase;
// float gInverseSampleRate;

// // ✅ 추가: 오디오 스레드와 별개로 센서 값을 읽어올 백그라운드 함수
// void readSensorBackground(void*) {
//     trillBar.readI2C();
// }

// bool setup(BelaContext *context, void *userData)
// {
//     Bela_getDefaultWatcherManager()->setup(context->audioSampleRate);
//     Bela_getDefaultWatcherManager()->getGui().setup(context->projectName);
    
//     if(trillBar.setup(1, Trill::BAR) != 0) {
//         rt_printf("Trill Bar 연결 실패! 배선을 확인하세요.\n");
//         return false;
//     }

//     // ✅ 추가: 백그라운드 태스크 생성 (함수, 우선순위, 이름)
//     i2cTask = Bela_createAuxiliaryTask(readSensorBackground, 50, "trill-i2c-read");

//     gInverseSampleRate = 1.0 / context->audioSampleRate;
//     gPhase = 0.0;
//     return true;
// }

// void render(BelaContext *context, void *userData)
// {
//     Bela_scheduleAuxiliaryTask(i2cTask);
    
//     float currentLoc = 0.0;
//     float currentSize = 0.0;

//     // 터치가 감지되었을 때
//     if(trillBar.getNumTouches() > 0) {
//         currentLoc = trillBar.touchLocation(0); 
//         currentSize = trillBar.touchSize(0);    

//         // 💡 1. 센서 위치(0.0 ~ 1.0)를 음높이(220Hz ~ 1100Hz)로 매핑합니다.
//         gFrequency = 220.0 + (currentLoc * 880.0); 
//     } else {
//         // 💡 2. 터치하지 않았을 때는 볼륨(currentSize)을 0으로 만들어 소리를 끕니다.
//         currentSize = 0.0; 
//     }
    
// 	Bela_getDefaultWatcherManager()->tick(context->audioFramesElapsed, true);
//     touchLocation = currentLoc;
//     touchSize = currentSize;

//     for(unsigned int n = 0; n < context->audioFrames; n++) {
//         // 이 안에는 소리(오디오)를 만드는 코드만 남깁니다.
//         float out = currentSize * sinf(gPhase); 
        
//         gPhase += 2.0 * (float)M_PI * gFrequency * gInverseSampleRate;
//         if(gPhase > 2.0 * (float)M_PI) gPhase -= 2.0 * (float)M_PI;

//         for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
//             audioWrite(context, n, channel, out);
//         }
//     }
// }

// void cleanup(BelaContext *context, void *userData)
// {
// }

#include <Bela.h>
#include <vector>
#include "KarplusStrong.h"
#include "Overdrive.h" 
#include "Delay.h"
#include "Chorus.h"
KarplusStrong gString;
Overdrive gDrive; 
Delay gDelay;
Chorus gChorus;
int gSampleCount = 0; 

#define C4 263.63f
#define D4 293.66f
#define E4 329.63f
#define F4 349.23f
#define G4 392.00f
#define A4 440.00f
#define B4 493.88f
#define C5 523.25f

int for_test = 0;

// 음계 주파수 벡터와 인덱스 선언
std::vector<float> gScale;
int gNoteIndex = 0;

bool setup(BelaContext *context, void *userData) {
    gScale = {E4, D4, C4, D4, E4, E4, E4,D4,D4,D4,E4,E4,E4,E4,D4,C4,D4,E4,E4};
    
    // 초기 설정
    gString.setup(context->audioSampleRate, gScale[0]); 
    
    // 3. 오버드라이브 초기 설정
    gDrive.setGain(20.0f);    // 게인 설정 (1.0~200.0)
    gDrive.setMix(0.8f);     // 원음과 왜곡음 비율 (0.0~1.0)
    gDrive.setMode(OverdriveMode::kHard); // 하드 클리핑 모드 설정

	gChorus.setup(context->audioSampleRate);
    gChorus.setRate(1.5f);   // 1.5Hz 속도로 흔들림
    gChorus.setDepth(3.0f);  // 3ms 깊이로 흔들림
    gChorus.setMix(0.5f);
	
	gDelay.setup(context->audioSampleRate);
    gDelay.setDelayTime(0.2f);
    gDelay.setFeedback(0.6f);
    gDelay.setMix(0.4f);
    
    return true;
}

void render(BelaContext *context, void *userData) {
    float volume = 0.2f; // 오버드라이브로 인해 소리가 커질 수 있으니 조절

    for(unsigned int n = 0; n < context->audioFrames; n++) {
        
        // 0.5초마다 다음 음 연주
        if (gSampleCount >= context->audioSampleRate * 0.5f) {
            gString.setFrequency(context->audioSampleRate, gScale[gNoteIndex]);
            gString.pluck(); 

			for_test++; //for debug
            
            //rt_printf("Note Index: %d, Freq: %.2f Hz\n", gNoteIndex, gScale[gNoteIndex]); 

            gNoteIndex = (gNoteIndex + 1) % gScale.size();
            gSampleCount = 0;
        }
        
        gSampleCount++;

        //먼저 현악기 원음을 생성
        float stringOut = gString.process();
		float finalOut;
        
        
		if (for_test % 8 < 0){ //일단 이거 안뜨게함
        	finalOut = gDrive.process(stringOut);
		}
		else{
			stringOut = gDrive.process(stringOut);
			stringOut = gChorus.process(stringOut);
			finalOut = gDelay.process(stringOut);
		}

        for(unsigned int ch = 0; ch < context->audioOutChannels; ch++) {
            // (3) 최종 결과를 출력합니다.
            audioWrite(context, n, ch, finalOut * volume);
        }
    }
}

void cleanup(BelaContext *context, void *userData) {
    // 필요한 정리 작업이 있다면 여기에 작성
}
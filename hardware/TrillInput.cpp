#include "TrillInput.h"
#include <Bela.h>
#include <libraries/Trill/Trill.h>
#include <algorithm>
#include <cmath>

// 사용법: 센서 on/off를 원할시, 각 센서마다 두가지를 시행
// ㄱ. bool TrillInput::setup()에서  3. 각 센서 초기화 에 if문과 그 아래의 g.setMode를 활성화
// ㄴ. void TrillInput::poll()에서 .Active = false; 줄을 주석처리 하고, 아래 긴 코드들은 주석( /* ~ */ ) 해제하여 활성화

namespace hw {

// 1. 실제 센서 객체 및 백그라운드 태스크 선언
static Trill gRing;
static Trill gQuality;
static Trill gComplexity;
static Trill gVoicing;
static AuxiliaryTask gReadTask;

namespace {
    // 터치 면적을 0.0 ~ 1.0 벨로시티로 정규화할 때 쓸 기준값
    constexpr float kStrengthFullScale = 0.05f; // 손가락 면적에 맞게 나중에 조절하세요.

    inline float clamp01(float v) { return std::min(1.0f, std::max(0.0f, v)); }

    // 2. 백그라운드에서 무한히 센서를 읽어올 루프 함수
    // (I2C 통신은 느려서 오디오 스레드에서 직접 읽으면 소리가 끊깁니다)
    void readLoop(void* arg) {
        TrillInput* input = static_cast<TrillInput*>(arg);
        while(!Bela_stopRequested()) {
            input->poll(); // 실제 센서 읽기 호출
            usleep(10000);  // 10ms 대기 (초당 약 100회 읽기, CPU 부담 완화)
        }
    }
} // namespace

bool TrillInput::setup(BelaContext* /*context*/) {
    // 3. 각 센서 초기화 (I2C 버스 1번 사용)
    if (gRing.setup(1, Trill::RING, 0x38) != 0) { rt_printf("Ring 센서 연결 실패!\n"); return false; }
    //if (gQuality.setup(1, Trill::BAR,  0x20) != 0) { rt_printf("Quality 바 연결 실패!\n"); return false; }
    if (gComplexity.setup(1, Trill::BAR, 0x21) != 0) { rt_printf("Complexity 바 연결 실패!\n"); return false; }
    //if (gVoicing.setup(1, Trill::BAR, 0x22) != 0) { rt_printf("Voicing 바 연결 실패!\n"); return false; }

    // 센서 모드를 위치와 면적을 둘 다 읽는 CENTROID 모드로 설정
    gRing.setMode(Trill::CENTROID);
    //gQuality.setMode(Trill::CENTROID);
    gComplexity.setMode(Trill::CENTROID);
    //gVoicing.setMode(Trill::CENTROID);

    // 4. 백그라운드 태스크 생성 및 실행
    gReadTask = Bela_createAuxiliaryTask(readLoop, 50, "trill-read", this);
    Bela_scheduleAuxiliaryTask(gReadTask);

    frame_ = TrillFrame{};
    rt_printf("✅ Trill 터치 센서 4개 초기화 완료!\n");
    return true;
}

void TrillInput::poll() {
    // 5. 각 센서의 상태를 읽어서 프레임(frame_)에 0.0 ~ 1.0 값으로 예쁘게 담습니다. << 각 센서마다, 주석( /* ~ */ ) 이거 지우고, Active = false;이 줄은 주석처리 하면 동작합니다.
    // [Ring] 5도권 베이스
	//frame_.ringActive = false;
	///*
    gRing.readI2C();
    if (gRing.getNumTouches() > 0) {
        frame_.ringActive = true;
        frame_.ringPos = gRing.touchLocation(0);
    } else {
        frame_.ringActive = false;
    }
	//*/

    // [Quality Bar] M/m/aug/dim
	frame_.qualityActive = false;
	/*
    gQuality.readI2C();
    if (gQuality.getNumTouches() > 0) {
        frame_.qualityActive = true;
        frame_.qualityPos = clamp01(gQuality.touchLocation(0));
    } else {
        frame_.qualityActive = false;
    }
	*/

    // [Complexity Bar] 텐션 (power -> dom7)
	frame_.complexityActive = false;
	//*
    gComplexity.readI2C();
    if (gComplexity.getNumTouches() > 0) {
        frame_.complexityActive = true;
        frame_.complexityPos = clamp01(gComplexity.touchLocation(0));
        // 만약 센서를 거꾸로 달아서 위아래가 반대라면 아래 줄을 쓰세요.
        // frame_.complexityPos = 1.0f - clamp01(gComplexity.touchLocation(0));
    } else {
        frame_.complexityActive = false;
    }
	//*/

    // [Voicing Bar] 폭 + 세기 (발음 트리거)
	frame_.voicingActive = false;
    frame_.voicingStrength = 0.0f;
	/*
    gVoicing.readI2C();
    if (gVoicing.getNumTouches() > 0) {
        frame_.voicingActive = true;
        frame_.voicingPos = clamp01(gVoicing.touchLocation(0));
        frame_.voicingStrength = clamp01(gVoicing.touchSize(0) / kStrengthFullScale);
    } else {
        frame_.voicingActive = false;
        frame_.voicingStrength = 0.0f;
    }
	*/
}

void TrillInput::cleanup() {
    // 프로그램 종료 시 특별히 처리할 것은 없습니다. < 비워뒀습니다. 뭘 해야하는 곳인지 이해를 못해서..
}

} // namespace hw

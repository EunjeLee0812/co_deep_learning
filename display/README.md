# display — 디스플레이 파트 (Nextion Intelligent NX8048P050-011C, 5.0")

## 먼저 알아야 할 것: Nextion 은 "화면을 코드로 그리지 않는다"
Nextion 은 자체 MCU(200MHz)/플래시를 가진 HMI 라서, **UI 는 PC의 Nextion Editor 로
디자인해서 디스플레이에 업로드**한다. 터치 반응·흰색 글로우·애니메이션은 디스플레이가
스스로 처리한다. Bela 는 **UART 시리얼**로 명령을 주고(`b0.pic=2` 등) 터치 이벤트를 받는다.

그래서 이 폴더의 코드 = **Bela에서 도는 "Nextion 드라이버 + 화면 제어 로직"** 이다.
(실제 그래픽은 Editor 프로젝트 `.HMI` → `.tft` 로 따로 만든다)

## 전체 작업 순서
1. **Nextion Editor** 로 화면 디자인 (아래 컴포넌트 규약대로)
2. 네가 만든 육각형 PNG 를 버튼의 일반/선택 그림으로 등록 (`assets/` 에 원본 보관)
3. 터치 컴포넌트의 **"Send Component ID"** 체크 → 터치 시 0x65 이벤트 전송
4. `.tft` 빌드 후 SD카드/시리얼로 디스플레이에 업로드
5. Bela 에 이 폴더 코드 올리고 시리얼 연결 → 동작

## 배선 (XH2.54 4핀: +5V, TX, RX, GND)
| Nextion | 연결 |
|---|---|
| +5V | 5V 전원 (전류 여유 있게 — 별도 5V 권장) |
| GND | 공통 GND |
| TX  | Bela UART RX |
| RX  | Bela UART TX |

- Nextion 데이터선은 **TTL 3.3V 로직**. Bela 도 3.3V 라 보통 직결 가능하지만,
  네 보드 UART 전압을 확인하고 다르면 레벨 시프터를 써. **5V 를 Bela 핀에 직접 넣지 말 것.**
- 가장 쉬운 방법: **USB-시리얼 어댑터**(`/dev/ttyUSB0`)를 Bela USB 에 꽂고 거기에 Nextion 연결.
- 기본 baud 9600. 갱신이 많으면 Editor 에서 115200 등으로 올리고 코드의 baud 도 맞춰.

## Nextion Editor 컴포넌트 규약 (코드와 일치 필수)
`EffectMenu.cpp` 가 이 이름/번호를 기대한다. Editor 에서 동일하게 만들 것:

- 육각형 8개: 이름 `b0`~`b7`, 컴포넌트 `.id` 는 `EffectMenu.cpp` 의 `hexId` 와 일치.
  각 버튼에 **일반 그림(picNormal)** 과 **선택 글로우 그림(picGlow)** 두 장 등록.
- 하단 파라미터 4칸: 라벨 텍스트 `t0`~`t3`, 숫자 `n0`~`n3`, 프로그레스바 `j0`~`j3`.
- 슬라이더를 터치로 조절하려면, 슬라이더의 **Touch Move 이벤트**에서 "슬롯<<8 | 퍼센트"
  형태의 숫자를 보내도록 구성(코드의 Number 파싱과 약속). 또는 간단히 슬라이더 컴포넌트의
  값을 `get` 해서 보내고 코드를 거기에 맞춰도 됨.

> 그림 id, 컴포넌트 번호는 Editor 환경마다 다르니 `EffectMenu.cpp` 의 숫자만 맞추면 된다.

## 노브 2개 (선택용 / 조절용)
노브는 Nextion 이 아니라 **Bela 가 읽는다**(hardware 파트). 읽은 값을 DisplayController 에 전달:
- **선택 노브**: 로터리 인코더면 `selectKnobDelta(+1/-1)`, 포텐쇼미터면 `selectKnobFromPot(0~1)`
- **조절 노브**: `adjustKnobFromPot(0~1)` → 현재 포커스된 하단 파라미터를 조절
- `focusNextParam()` 으로 4칸 중 조절 대상 이동

## 파일 구조
| 파일 | 역할 |
|---|---|
| `SerialPort.*` | 리눅스 UART(termios) 래퍼 |
| `NextionProtocol.h` | 프로토콜 상수/반환코드(0x65 터치 등) |
| `NextionLink.*` | 명령 송신(setPicture/setNumber/setText) + 수신 파서 |
| `EffectParamSink.h` | **파라미터를 외부(엔진)로 내보내는 API** |
| `EffectMenu.*` | 8개 이펙트 ↔ 화면 컴포넌트 ↔ 엔진 paramId 매핑 테이블 |
| `DisplayController.*` | 앱 로직: 터치/노브 처리, 글로우 전환, 값 송출, 화면 동기화 |

## 스레딩 (중요)
시리얼 read/write 는 블로킹될 수 있으니 **오디오 스레드(render)에서 호출 금지**.
Bela 의 `AuxiliaryTask` 로 별도 태스크를 만들어 거기서 `DisplayController::update()` 를
주기적으로(예: 5~10ms) 돌린다. 엔진으로 보내는 값은 lock-free 큐로 넘기는 게 안전하다.

## 연동 예시 (개념)
```cpp
#include "display/src/NextionLink.h"
#include "display/src/DisplayController.h"

disp::NextionLink       gLink;
disp::DisplayController  gDisplay;
// gEngineSink: disp::EffectParamSink 구현(엔진). 받은 값을 effects/ 객체로 라우팅.

bool setup(BelaContext* ctx, void*) {
    gLink.setup("/dev/ttyUSB0", 115200);
    gDisplay.setup(&gLink, &gEngineSink);
    // 보조 태스크 생성: 주기적으로 gDisplay.update() 호출
    //   AuxiliaryTask t = Bela_createAuxiliaryTask(displayTask, 50, "display");
    return true;
}
// render() 에서는 노브 값만 넘겨줌(가벼움):
//   gDisplay.selectKnobFromPot(sel); gDisplay.adjustKnobFromPot(adj);
```

## 엔진 측 EffectParamSink 구현 가이드
`setEffectParameter(target, paramId, value)` 를 받아 target 에 맞는 effects/ 객체로:
```cpp
switch (target) {
  case EngineTarget::Reverb:       gReverb.setParameter(paramId, value); break;
  case EngineTarget::Delay:        gDelay.setParameter(paramId, value); break;
  case EngineTarget::Modulation:   gModSet.setParameter(paramId, value); break; // 오프셋 포함
  case EngineTarget::ChannelStrip: gStrip.setParameter(paramId, value); break;
  case EngineTarget::Distortion:   gDist.setParameter(paramId, value); break;
}
```

## assets/
- `ui_mockup_screen.png` — 완성 화면 목업
- `effect_hexagon_assets.png` — 이펙트별 일반/글로우 + UI 요소 시트 (Editor 에 등록)

## TODO
- [ ] Nextion Editor 프로젝트(.HMI) 작성 + 컴포넌트 번호를 EffectMenu 와 동기화
- [ ] 슬라이더 Touch Move 전송 포맷 확정(슬롯<<8|퍼센트 권장)
- [ ] 엔진 송출을 lock-free 큐 경유로(오디오 스레드 안전)
- [ ] 포커스 슬롯 시각 표시, 빈 슬롯 처리
- [ ] 페이지가 여러 장이면(하단 점들) 페이지 전환 처리

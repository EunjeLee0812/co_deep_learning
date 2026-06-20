# hardware — 하드웨어 입력 파트 (Model 84 레퍼런스)

물리 노브/페이더/스위치의 전기신호를 읽어 신스 파라미터로 바꾸고, 엔진과 디스플레이가
가져다 쓸 수 있게 통지한다.

## Bela 입출력 기본
- 아날로그 입력 **8개** : 0~4.096V → `analogRead()` 가 0~1 로 반환
- 디지털 I/O **16개** : 0/3.3V. **5V 절대 금지(CPU 손상)**. `digitalRead()`
- 노브가 21개라 8개로 부족 → **멀티플렉서로 확장** (캐플릿 or 외장 74HC4067)

## IDE 설정
Settings → Capelet Settings → Multiplexer capelet → **32 channels (8×4)** 선택.
(21개면 32채널로 충분. 64채널은 채널당 2.75kHz로 느려지니 불필요)
> Bela Gem(PocketBeagle 2)에서 캐플릿 지원 여부는 IDE에서 확인. 미지원이면 외장 MUX 사용.

## 배선표 — 노브/페이더 (멀티플렉서 x.y, x=아날로그입력, y=mux채널)
| 컨트롤 | 핀 | | 컨트롤 | 핀 |
|---|---|---|---|---|
| LFO Rate | 0.0 | | Pitch→DCO | 3.0 |
| LFO Delay | 0.1 | | Pitch→LFO | 3.1 |
| DCO LFO | 0.2 | | Mod→LFO | 3.2 |
| DCO PWM | 0.3 | | Glide Time | 3.3 |
| DCO Sub | 1.0 | | Env A | 4.0 |
| DCO Noise | 1.1 | | Env D | 4.1 |
| LPF Freq | 1.2 | | Env S | 4.2 |
| LPF Res | 1.3 | | Env R | 4.3 |
| LPF Env | 2.0 | | VCA Level | 5.0 |
| LPF LFO | 2.1 | | (여유) | 5.1~7.3 |
| LPF Track | 2.2 | | | |
| Volume | 2.3 | | | |

**노브/페이더 배선:** 양 끝 단자 → 한쪽 GND, 다른쪽 아날로그 기준전압(예: 3.3V/Vref),
가운데(wiper) → 해당 mux 입력. 선형 테이퍼(B타입) 권장. (오디오용 A/로그 테이퍼면
코드의 Curve 로 보정)

## 배선표 — 스위치 (디지털 핀)
| 스위치 | 핀 | 포지션 |
|---|---|---|
| DCO Range | D0,D1 | 16'/8'/4' |
| PWM Source | D2 | LFO/MAN |
| Wave Square | D3 | on/off |
| Wave Saw | D4 | on/off |
| Env Polarity | D5 | +/- |
| Voicing | D6,D7 | POLY1/POLY2/UNISON |
| VCA Shape | D8 | ENV/GATE |

**스위치 배선:** 한쪽 3.3V, 핀에 **풀다운 저항(10kΩ)** 달아 안 눌렀을 때 0V 고정.
3포지션 토글은 핀 2개로 디코딩(코드의 `readSwitchPosition` 매핑을 실제 배선에 맞게 조정).

> 배선표를 바꾸려면 `HardwareInput.cpp` 상단의 `kPots` / `kSwitches` 테이블만 수정하면 됨.
> 거기가 사실상 배선도다.

## 파일 구조
| 파일 | 역할 |
|---|---|
| `ControlIds.h/.cpp` | 파라미터 목록 + 범위/커브 스펙 테이블 (단위/범위는 여기서 조정) |
| `AnalogControl.*` | 노브 1개: 노이즈 필터 → 데드밴드 변화감지 → 커브/범위 변환 |
| `SwitchControl.*` | 스위치 1개: 디바운스 + 포지션 디코딩 |
| `ParameterSink.h` | 하드웨어/디스플레이가 변경을 흘려보내는 공통 인터페이스 |
| `HardwareInput.*` | 최상위: 배선 매핑 + 매 블록 읽기 + 통지 + 디스플레이 양방향 |

## 디스플레이 연동 (양방향)
- **디스플레이 → 하드웨어**: `setFromDisplay(id, value)` — 프리셋 로드/터치 조작.
  물리 노브 위치는 안 건드리고 값만 갱신, 노브가 실제로 움직이면 다시 하드웨어 우선.
- **하드웨어 → 디스플레이**: `setDisplayFeedbackSink()` 로 sink 등록하면, 물리 노브가
  움직일 때마다 그 변경이 디스플레이로도 통지되어 터치 UI가 동기화된다.
  (오디오 스레드에서 호출되므로 sink 구현은 lock-free 큐에 넣고 별도로 전송할 것)

## render.cpp 연동 예시
```cpp
#include "hardware/HardwareInput.h"
hw::HardwareInput gHardware;

// 엔진이 hw::ParameterSink 를 구현한다고 가정 (setParameter(id,value))
bool setup(BelaContext* context, void*) {
    gHardware.setup(context, &gEngineSink);
    // gHardware.setDisplayFeedbackSink(&gDisplayQueue); // 선택
    return true;
}
void render(BelaContext* context, void*) {
    gHardware.process(context);   // 컨트롤 읽고 변경분 통지
    // ... 오디오 생성 ...
}
```

## TODO / 확장
- [ ] 외장 74HC4067 사용 시 `readPot` 만 교체(select 핀 digitalWrite + 공통 analogRead)
- [ ] AnalogControl "soft pickup" 모드(값 점프 제거) 옵션
- [ ] 스위치 3포지션 디코딩을 실제 배선에 맞게 확정
- [ ] ControlIds 를 shared/protocol 로 승격해 디스플레이와 단일 출처 공유

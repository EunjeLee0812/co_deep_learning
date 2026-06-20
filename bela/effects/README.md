# effects — 음향 이펙터 파트

모든 이펙터는 `Effect` (Effect.h) 를 상속한다. 공통 계약:

```cpp
bool setup(float sampleRate, unsigned int maxBlockSize); // 시작 전 1회
void process(float* left, float* right, unsigned int numFrames); // 스테레오 in-place
void setParameter(int paramId, float value);  // 디스플레이에서 온 값 적용
void setBypass(bool); void reset(); void cleanup();
```

- **디스플레이 → Bela** 통신은 `(paramId:int, value:float)` 한 쌍. 각 이펙트의 `enum class Param`
  값이 paramId 다. value 단위/범위는 각 헤더 Param 주석 참고.
- 파라미터 급변 글리치 방지용 `SmoothedValue` 유틸이 Effect.h 에 있음 (실제 동작 구현됨).
- 세부 DSP 는 각 `.cpp` 의 `TODO(구현자)` 를 채우면 됨. 헤더 시그니처/Param 은 바꾸지 말 것(통신 규약과 직결).

## 이펙터 목록

| 이펙트 | 파일 | 비고 |
|--------|------|------|
| Reverb | Reverb.* | 룸/홀 블렌딩 + 프리딜레이 + wet/dry + 내부 3밴드 EQ |
| Delay | Delay.* | 좌우 독립(타임/오프셋/스핀/피드백/EQ) + sync/blur + 링크 |
| ModulationSet | ModulationSet.* | **Flanger+Phaser+Chorus 묶음** (순서/LFO sync 관리) |
| └ Flanger | Flanger.* | 세트 서브모듈 |
| └ Phaser | Phaser.* | 세트 서브모듈 |
| └ Chorus | Chorus.* | 세트 서브모듈 (레퍼런스 없음, 표준 파라미터) |
| ChannelStrip | ChannelStrip.* | **EQ+Comp 묶음** + 드라이브 + 출력 + EQ↔COMP 순서 |
| └ Equalizer | Equalizer.* | HP/Lo shelf/Mid peak/Hi shelf, HP>EQ·HP>SC |
| └ Compressor | Compressor.* | threshold/ratio/attack/release/drywet, GR 미터, 사이드체인 |
| Distortion | Distortion.* | 스펙 미정 — 표준 파라미터 placeholder |

## 묶음(세트) 파라미터 ID 규약
ModulationSet / ChannelStrip 처럼 여러 서브모듈을 묶은 이펙트는 **100단위 구간**으로 ID 를 나눠
한 세트가 하나의 ID 공간을 갖게 했다. 디스플레이/통신 레이어가 단순해짐.

- ModulationSet: `0~99 Flanger`, `100~199 Phaser`, `200~299 Chorus`, `300~ 세트 글로벌`
- ChannelStrip: `0~99 Equalizer`, `100~199 Compressor`, `200~ 스트립 글로벌`

예) 페이저 Stages 변경 → paramId = 100(kPhaserBase) + 2(Phaser::Param::Stages) = 102

## 새 이펙트 추가하는 법
1. `Effect` 상속한 `MyFx.h/.cpp` 작성, `enum class Param` 정의
2. `setParameter` 에 switch 라우팅
3. `process` 에 DSP (또는 TODO)
4. 체인에 등록 (synthesis/ 쪽 이펙트 체인에서 인스턴스화)

> 통신 규약과 맞추려면 paramId 들을 `shared/protocol/` 에도 반영할 것(추후).

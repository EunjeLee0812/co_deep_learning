# synthesis — 연주/소리 생성 파트 (개편판)

## 무엇이 바뀌었나
음악이론 추론(텐션 선택·보이싱 추론)을 **전부 제거**하고, 연주센서 → 음 매핑을
단순·결정적(deterministic)으로 바꿨다.

| 항목 | 구버전 | **신버전** |
|---|---|---|
| 연주 센서 | Ring + Bar 3 (Quality/Complexity/Voicing) | **Ring 1 + Bar 4 (Bass + R5/R8/R3)** |
| 음 결정 | ChordPerformer → MusicTheory.buildChord (텐션/공통음 추론) | **5도권 루트 + 1·5·8·3 보이싱 (고정 규칙)** |
| 보이스 | 동적 풀(최대 16, 보이스 스틸) | **고정 4개 (바 1개 = 보이스 1개)** |
| 피치 | 정수 MIDI(이산) | **연속(바이올린처럼) + 등간격 + 짧은 글라이드** |
| 삭제됨 | — | `MusicTheory.h/.cpp`, `ChordPerformer.h`, `CompTables.h`, `gen_tables.py` |

## 조작 매핑
- **Trill Ring** = 코드(베이스 루트) 선택. 5도권 12등분(이산). 터치 중 실시간 갱신,
  **손을 떼면 마지막 루트를 래치**(계속 유지). 슬라이드하면 누르고 있던 바의 음도
  새 루트에 맞춰 미끄러진다.
- **Bar 4개** (가운데=기준음, 위=낮음 / 아래=높음, 한 바 = 한 옥타브 전부):
  - **Bass**(왼쪽) = 1음(루트) · 베이스 음역 (+서브 오실레이터)
  - **R5** = 5음 / **R8** = 8음(옥타브) / **R3** = 3음
  - 예) 링 E 선택 → 가운데가 각각 **B, E, G#** (Bass는 E2)

## 파일
- `ChordVoicing.h` — **음악 계층(순수 함수)**. 5도권 매핑 + 1·5·8·3 보이싱 + 바→연속피치.
  보이싱/옥타브 배치는 `kVoiceCenterOffset[]`, 음역폭은 `kBarSemitoneSpan` **한 줄로 변경**.
- `Voice.h` — 바 1개용 연속 피치 보이스. center(글라이드) + offset(즉각) 분리.
  글라이드 시간은 `kGlideSeconds`.
- `SynthEngine.h/.cpp` — 링 래치 + 4바 게이트/피치 + 믹스 → 이펙터 → 마스터.
- `Oscillator/Filter/Envelope/Lfo/SynthParams.h` — DSP 블록(변경 없음, 그대로 재사용).

## 튜닝 포인트 (TODO/취향)
- `kVoiceCenterOffset[Third]`: 기본 `+16`(오픈 10도). 닫힌 3도 원하면 `+4`.
- `kBarSemitoneSpan`: 한 바 음역(기본 12=옥타브). 더 좁게/넓게 가능.
- `kGlideSeconds`: 루트 전환 미끄러짐 정도(기본 20ms). 바 추적은 항상 즉각.
- `BarTouch.strength`: 하드웨어가 채우면 벨로시티로 동작(0이면 풀 볼륨). [선택]

## 하드웨어 담당 핸드오프
`hardware/TrillInput.cpp` 의 TODO 에 Ring 1 + Bar 4 의 I2C 읽기를 채우면 된다.
**바 방향 규약**: `pos 0=맨위(낮음) / 0.5=가운데 / 1=맨아래(높음)` — 장착이 뒤집혔으면
`(1.0f - raw)` 로 보정. 게터 시그니처(계약)는 변경 금지.

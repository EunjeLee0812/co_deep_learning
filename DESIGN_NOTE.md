# Trill 코드 신디사이저 — 설계 노트

Bela Gem (PocketBeagle 2) 위에서 도는 **코드 연주형 신디사이저**.
Trill Ring 1개 + Trill Bar 3개로 코드를 고르고 발음한다.
Juno-106 / Softube Model 84 계열의 신스 보이스 + 스테레오 이펙트 체인.

---

## 1. 파트 분담 (누가 어디를 채우나)

| 영역 | 폴더 | 상태 | 담당 |
|------|------|------|------|
| **신스 합성 (소리 만들기)** | `synthesis/`, `render.cpp` | ✅ **완성** | 이은제 |
| **Trill 하드웨어 입력** | `hardware/TrillInput.cpp` | 🔲 TODO 스켈레톤 | 하드웨어 담당 |
| **패널(노브/토글) 입력** | `hardware/HardwareInput.cpp` | (기존) | 하드웨어 담당 |
| **이펙트 DSP 내부** | `effects/*/**.cpp` | 🔲 패스스루 stub | 이펙트 담당 |

> 핵심: **합성 파트는 컴파일·동작 검증까지 끝나 있다.**
> 하드웨어/이펙트 담당이 자기 `.cpp`의 TODO만 채우면 바로 소리가 난다.
> (이펙트는 기본 전부 bypass라, 채우기 전에도 dry 신스 소리는 들림.)

---

## 2. 시그널 흐름

```
[Trill 센서 4개]  --I2C(AuxiliaryTask)-->  TrillInput
                                              │  snapshot() → TrillFrame
                                              ▼
[노브/토글 패널] --mux/digital--> HardwareInput --setParameter()--> SynthEngine
                                                                      │
                                            applyPerformance(TrillFrame)
                                              │  (코드 선택 + 트리거)
                                              ▼
                                        Voice[16]  (Osc+Filter+Env)
                                              │  블록 합산
                                              ▼
                                        EffectChain (스테레오)
                                          Strip→Drive→Mod→Echo→Verb
                                              ▼
                                          master volume → audioWrite(L,R)
```

- **Trill 입력은 절대 오디오 스레드에서 I2C를 읽지 않는다.** `render.cpp`가 `AuxiliaryTask` 안에서 `gTrill.poll()`을 돌리고, 오디오 쪽은 `snapshot()`으로 마지막 프레임만 읽는다 (단일 작성자/단일 독자).
- 합성은 **블록 단위**(`render(L,R,numFrames)`). 이펙트 체인이 스테레오 블록 in-place 계약이라 거기에 맞춰 보이스도 블록으로 처리.

---

## 3. Trill → 코드 매핑

| 센서 | 역할 | 매핑 |
|------|------|------|
| **Ring** | 코드 root | 5도권. 12시=C, 시계방향 C-G-D-A-E-B-F#-Db-Ab-Eb-Bb-F. `pc = (segment*7)%12` |
| **가로 Bar** | 퀄리티 | 4등분: Major / minor / aug / dim |
| **왼쪽 세로 Bar** | 복잡도 | 6등분(아래→위): Power · Triad · Add9 · Maj7 · Dom7 · Dom7+Tension |
| **오른쪽 세로 Bar** | 보이싱폭 + 세기 + **트리거** | 터치=게이트온. 높이=보이싱 폭, 터치면적=벨로시티 |

**트리거 모델:** 오른쪽 바를 만지는 순간(rising edge)에 *나머지 3개 바의 상태를 래치*해서 코드를 확정하고 발음한다. 손가락이 닿아 있는 동안은 그 코드가 유지되고, 떼면 릴리즈. → 만지는 도중 다른 바를 흔들어도 보이싱이 떨리지 않는다.

---

## 4. 텐션 선택 알고리즘 (`MusicTheory.cpp : chooseTensions`)

이은제가 질문했던 "이전 코드로 텐션을 예측할 수 있나"에 대한 **구현된 답**.
완전한 화성 분석 대신, **이전 코드와의 관계 + root 진행**으로 점수를 매겨 고른다.

후보 텐션: `b9, 9, #9, #11, b13, 13`. 각 후보에 점수:

1. **공통음 유지 (+2.0):** 직전 코드에 들어있던 음과 같은 pitch-class면 가산. → 코드가 바뀌어도 공통음이 남아 연결이 매끄럽고, 결과적으로 "모호하고 복잡한" 색을 만든다.
2. **Root 진행 (±1.5/±1.0):**
   - 5도 하행(V→I 해결, 진행거리 5) → **얼터드 텐션**(b9/#9/b13) 가산. 도미넌트의 긴장을 강조.
   - 순차/기타 진행 → **내추럴 텐션**(9/13/#11) 가산. 부드러운 색.
   - 직전 코드 없음 → 9, 13에 약한 기본 가산.
3. **결정적 기본 선호**(아주 작은 상수): 같은 점수일 때 매번 같은 결과가 나오도록(난수 없음).

뽑는 개수 `nTensions = 1 + round(voicingWidth*2)` → 보이싱 바를 위로 올릴수록 1~3개로 텐션이 늘어난다.

> DSP 담당이 신경 쓸 것 없음 — 순수 정수/float 연산, 오디오 스레드에서 호출해도 안전.

---

## 5. 빌드 메모

- C++17. `synthesis/`와 `effects/`는 Bela 헤더 없이도 컴파일된다(검증됨).
- include 경로에 **`effects/`를 추가**해야 한다 (이펙트 헤더가 `"Effect.h"`를 상대경로로 참조).
- `render.cpp` / `HardwareInput.cpp` / `TrillInput.cpp`만 실제 `Bela.h`가 필요 → 보드(또는 IDE)에서 빌드.
- 검증: 합성+이펙트 전체 링크/실행 OK. 트리거 시 피크 ~0.23(클립 없음), 릴리즈 정상 감쇠, NaN/Inf 없음.

---

## 6. 채워야 할 TODO 한눈에

- `hardware/TrillInput.cpp` — `Trill::RING/BAR`, `readI2C`, `getNumTouches`, `touchLocation`, `touchSize` 호출부. **비어 있으면 무음**(게이트가 안 열림).
- `effects/*/**.cpp` — 각 이펙트 `process()` 내부. 비어 있으면 패스스루(소리는 정상).
- `synthesis/Voice.h` 의 Unison 해석: 현재는 "디튠된 보조 osc 레이어"로 구현(코드 악기에 맞춤). 원래 Juno식 "모든 보이스 같은 음"으로 바꾸려면 주석 지점만 교체.

# 프로젝트 완성 로드맵 — 맥북 기준 작업 순서

> 어떤 작업을 어떤 컴퓨터에서, 어떤 순서로 하면 되는지 한눈에 정리.
> 상세 절차는 각 단계에 표시한 문서 참조.

## 컴퓨터별 역할 분담

| 작업 | 맥북 | 윈도우 PC |
|---|---|---|
| Bela 코드 편집/빌드/실행 | ✅ Bela IDE 는 브라우저 기반 (`http://bela.local`) | 불필요 |
| Bela 터미널 (오버레이 설치 등) | ✅ 기본 터미널에서 `ssh root@bela.local` | 불필요 |
| Nextion HMI 편집/컴파일/시뮬레이션 | ❌ | ✅ Nextion Editor (Windows 전용) |
| `.tft` 를 microSD 에 복사 | ✅ (FAT32 카드는 맥에서 그냥 복사됨) | ✅ 어느 쪽이든 |
| 납땜/배선 | 컴퓨터 무관 | |

즉 **윈도우가 필요한 건 Nextion Editor 딱 하나**다. HMI 는 한번 완성하면 자주 안
바꾸므로, 팀원(디스플레이 담당)의 윈도우 PC 에서 HMI 를 만들고 `.tft` 만 받아도 된다.

## 단계별 순서

### 0단계 — 코드 배치 (맥, 10분)
`co_deep_effect_chain.zip` 의 파일들을 Bela IDE 프로젝트에 넣는다:
- 신규: `effects/PluginChain.*`, `display/src/EffectDefs.h`, `display/src/ChainUiController.*`
- 교체: `synthesis/SynthEngine.h`, `render.cpp`
- `.bak` 처리(빌드 제외): `display/src/DisplayController.*`, `EffectMenu.*`,
  `NextionLink.*`, `EffectParamSink.h`, `comm/CommHandler.*`, `effects/EffectChain.*`
  — **`SerialPort.*` 는 남길 것** (계속 사용)
- 이 시점에 빌드가 통과해야 한다. 디스플레이가 아직 없어도 신스는 정상 동작
  (콘솔에 "Nextion 시리얼 열기 실패 — 디스플레이 없이 계속 진행" 경고만 뜸).

### 1단계 — Bela 에 UART4 살리기 (맥 터미널, 20분)
`CONNECTION_GUIDE.md` 3절. 오버레이 빌드 → extlinux.conf 추가 → 재부팅 →
`ls /dev/ttyS4` 확인 → **P1.20↔P2.20 점퍼 루프백 테스트**까지.
이 단계는 디스플레이가 아직 없어도 완료할 수 있다.

### 2단계 — Nextion HMI 제작 (윈도우, 반나절)
`NEXTION_GUIDE.md` 전체. 배경/아이콘 이미지 준비 → 페이지 3개 + 이벤트 코드 →
**Debug 시뮬레이터에서 `a5 …` 바이트가 나가는지 확인** → `.tft` 출력.
끝나면 에디터에 표시된 Picture ID 를 보고 맥에서
`display/src/EffectDefs.h` 의 `kPicNormal/kPicSelected/kBgPic` 를 맞춘다.

### 3단계 — 배선 (컴퓨터 무관, 30분)
`CONNECTION_GUIDE.md` 1~2절. 전원 다 끈 상태에서:
- Bela P1.20(TX) → Nextion RX
- Nextion TX → **분압저항(1k/2k)** → Bela P2.20(RX)
- GND 공통, Nextion 5V 는 별도 어댑터

### 4단계 — 플래싱 & 통합 테스트 (맥, 30분)
`.tft` 를 microSD 로 Nextion 에 플래싱(CONNECTION_GUIDE 5절) 후,
`CONNECTION_GUIDE.md` 6절의 4단계 테스트를 순서대로:
1. (이미 완료) 루프백
2. `echo -ne 'page 0\xff\xff\xff' > /dev/ttyS4` → 화면 반응 = Bela→Nextion OK
3. `cat /dev/ttyS4 | xxd` + 슬라이더 조작 → `a5 06 …` = Nextion→Bela OK
4. 프로젝트 실행 → `+` → SPACES → REVERB → Trill 연주 → MIX 올려 잔향 확인

### 5단계 — 마무리 점검
- [ ] IDE pin diagram 에서 P1.20/P2.20 이 D0~D3(kMuxPins)와 겹치지 않는지 확인
- [ ] 8개 이펙트 각각 추가→파라미터 조작→삭제 스모크 테스트
- [ ] 같은 이펙트 2개 추가 후 서로 독립적으로 동작하는지 확인 (예: 딜레이 2개, 다른 시간)
- [ ] 드래그로 순서 변경 → 소리 변화 확인 (예: DIST→REVERB vs REVERB→DIST)
- [ ] 체인 8개 꽉 채우고 CPU 미터 확인 (Bela IDE 우상단) — 여유 확인용
- [ ] 최종 배치 전 `render.cpp` 의 `[임시]` 0.2배 게인 제거

## 문제 생겼을 때
어느 단계까지 성공했는지로 원인을 좁힌다 (CONNECTION_GUIDE 6절 말미):
1단계 실패 = 오버레이, 2단계(윈도우 시뮬레이터) 실패 = HMI 이벤트 코드,
4-2 실패 = Bela TX 배선, 4-3 실패 = Nextion TX/분압저항, 4-4 실패 = 코드/보레이트.

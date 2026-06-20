# (프로젝트명) — Bela 기반 음악 컴퓨팅 악기

디스플레이로 컨트롤하고, Bela에서 실시간으로 소리를 생성/처리하는 악기 프로젝트.

## 구조 한눈에 보기

| 폴더 | 역할 (네가 말한 파트) |
|------|----------------------|
| `bela/instruments/` | 악기 파트 — 음원/보이스(오실레이터, 신스, 샘플러) |
| `bela/effects/` | 음향 이펙터 파트 — 리버브, 딜레이, 필터 등 |
| `bela/synthesis/` | 소리 생성 파트 — DSP 엔진, 보이스 관리, 믹서 |
| `bela/hardware/` | 하드웨어 정보 가져오는 파트 — 센서/ADC/GPIO/MIDI 입력 |
| `bela/comm/` | 디스플레이↔Bela 통신 (Bela 수신측) |
| `display/` | 디스플레이 파트 — UI / 컨트롤(설정·이펙터 파라미터) |
| `display/comm/` | 디스플레이↔Bela 통신 (디스플레이 송신측) |
| `shared/protocol/` | 양쪽이 공유하는 통신 프로토콜 정의 (파라미터 ID, 메시지 포맷) |
| `bela/core/` | 공통 유틸 (링버퍼, 파라미터 타입, 상수) |
| `tools/` | 빌드/배포 스크립트 |
| `tests/` | 테스트 |
| `docs/` | 설계 문서 |
| `legacy/` | 과거 파일 임시 보관 → 정리 후 위 폴더로 이동 |

## 데이터 흐름 (대략)

```
[하드웨어 입력] ─┐
                 ├─> [synthesis 엔진] ─> [instruments] ─> [effects] ─> 오디오 출력
[디스플레이] ─comm─┘   (파라미터 제어)
```

## 시작하기
- Bela 측: `bela/` 를 Bela 보드에 올려 빌드 (render.cpp 가 진입점)
- 디스플레이 측: `display/` (스택 미정 — README 참고)
- 통신 규약: `docs/communication-protocol.md`

## TODO (상세 기능은 추후 확정)
- [ ] 통신 프로토콜 확정 (`shared/protocol/`)
- [ ] 각 파트 인터페이스 → 구현
- [ ] legacy 파일 정리

# 통신 프로토콜 (디스플레이 ↔ Bela)

> 양쪽이 공유하는 "계약". 여기 바뀌면 display/comm 과 bela/comm 둘 다 맞춰야 함.

## 전송 방식 (선택 필요)
- [ ] OSC over UDP  (Bela 표준, 추천)
- [ ] WebSocket
- [ ] Serial

## 메시지 종류 (초안)
| 방향 | 메시지 | 내용 |
|------|--------|------|
| Display → Bela | SET_PARAM | paramId, value |
| Display → Bela | SET_BYPASS | effectId, on/off |
| Display → Bela | NOTE | note, velocity |
| Bela → Display | STATUS | (선택) 현재 값 동기화 |

## 파라미터 ID 표 (초안 — 채워나갈 것)
| ID | 이름 | 범위 | 대상 파트 |
|----|------|------|-----------|
| 0  | masterVolume | 0.0~1.0 | synthesis |
| 1  | reverbMix | 0.0~1.0 | effects |
| ...|      |      |       |

# comm — 디스플레이↔Bela 통신 (Bela 수신측)
디스플레이가 보낸 설정/이펙터 제어 메시지를 받아 SynthEngine 에 반영.
전송 방식(OSC over UDP / WebSocket / 시리얼 등)은 docs/communication-protocol.md 에서 확정.
메시지 포맷·파라미터 ID 는 shared/protocol/ 와 반드시 일치시킬 것.

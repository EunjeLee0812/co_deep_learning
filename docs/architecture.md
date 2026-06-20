# 아키텍처

## 큰 그림
- **Bela (실시간 오디오)**: hardware 입력 + display 제어를 받아 synthesis 가
  instruments → effects 체인으로 소리를 만들어 출력.
- **Display (제어)**: 설정/이펙터 파라미터를 UI 로 조작해 comm 으로 Bela 에 전송.

## 모듈 의존 방향
display/comm  ──(protocol)──>  bela/comm  ──>  synthesis.setParam
hardware  ──>  synthesis.setParam / noteOn
synthesis ──>  instruments ──>  effects ──> audio out

## 원칙
- 파라미터 변경은 항상 synthesis 의 setParam 한 곳으로 모은다.
- 통신 포맷은 shared/protocol 단일 출처를 따른다.

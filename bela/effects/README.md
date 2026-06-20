# effects — 음향 이펙터 파트
리버브, 딜레이, 필터, 디스토션 등.
`Effect.h` 를 상속. 디스플레이에서 파라미터(setParam)와 on/off(setBypass)를 제어함.
이펙터 체인 순서 관리는 synthesis/ 의 믹서/체인에서 담당.

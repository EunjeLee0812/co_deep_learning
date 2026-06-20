# synthesis — 소리 생성 파트
DSP 엔진의 중심. instruments 보이스들을 합치고(보이스 할당/믹스),
effects 체인을 통과시켜 최종 출력 샘플을 만든다.
파라미터 변경은 setParam 으로 일원화 (comm / hardware 가 호출).

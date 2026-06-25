# 이펙트 파라미터 맵 (디스플레이 ↔ Bela)

디스플레이(Nextion)에서 노브/버튼 하나가 움직이면
**`EffectChain::setParameter(chainParamId, value)`** 한 번만 호출하면 된다.

```
chainParamId = slot * 1000 + (이펙트 내부 paramId)
```

- 슬롯: `Strip=0, Drive=1, Mod=2, Echo=3, Verb=4`
- 라우팅은 `EffectChain::setParameter()` 가 `id/1000` → 슬롯, `id%1000` → 내부 paramId 로 자동 분배.
- 켜고 끄기는 파라미터가 아니라 `setSlotBypass(slot, bypass)`. **모든 슬롯은 기본 bypass(=꺼짐)** 이므로
  쓰려는 이펙트는 `setSlotBypass(<Slot>, false)` 로 한 번 켜줘야 소리에 반영된다.

> 값(value)의 단위/범위는 각 헤더 `enum class Param` 주석이 최종 기준이다. 아래 표의 범위는 요약.

---

## 공통

| 노브 | 위치 | 비고 |
|------|------|------|
| Gain | 각 이펙트 paramId 0 | 입력 게인 dB (−inf..+12). ChannelStrip 만 글로벌(200)에 있음 |
| Out  | 각 이펙트 paramId 1 | 출력 레벨 dB (−inf..+12). ChannelStrip 만 글로벌(201)에 있음 |

---

## Spaces

### 1) Reverb  (slot Verb=4 → +4000)

| 노브/탭 | chainParamId | 값 |
|---|---|---|
| Gain | **4000** | dB |
| Out | **4001** | dB |
| Plate/Hall 선택 탭 | **4002** | 0=Plate, 1=Hall |
| Size 노브 | **4003** | 0..1 |
| Decay 노브 | **4004** | 0..1 |
| Low cut (리버브 전) | **4005** | 20..1000 Hz |
| High cut | **4006** | 1000..20000 Hz (코어 댐핑 코너) |
| Spin | **4007** | 0..5 Hz |
| Spin depth | **4008** | 0..1 |
| Mix | **4009** | 0=dry..1=wet |

### 2) Delay  (slot Echo=3 → +3000)

| 노브/버튼 | chainParamId | 값 |
|---|---|---|
| Gain | **3000** | dB |
| Out | **3001** | dB |
| Feedback | **3002** | 0..0.98 |
| Tempo(BPM) | **3003** | 20..300 |
| BPM sync 버튼 | **3004** | 0=시간모드, 1=박자모드 |
| Link 버튼 | **3005** | 0/1 (1이면 좌=우) |
| Normal/PingPong | **3006** | 0=Normal, 1=PingPong |
| High cut | **3007** | 1000..20000 Hz (피드백 LP) |
| Low cut | **3008** | 20..2000 Hz (피드백 HP) |
| Mix | **3009** | 0..1 |
| Left 시간 | **3010** | 1..4000 ms (sync **off** 일 때) |
| Left 박자 | **3011** | 0..6 노트분할 (sync **on** 일 때) |
| Right 시간 | **3012** | 1..4000 ms |
| Right 박자 | **3013** | 0..6 노트분할 |

> 노트분할 코드: `0=1/1, 1=1/2, 2=1/4, 3=1/8, 4=1/16, 5=점8, 6=셋잇단8`.
> 같은 "좌/우 조절 노브"가 sync 버튼 상태에 따라 시간(3010/3012) 또는 박자(3011/3013) paramId 로 전송되도록
> 디스플레이에서 분기하면 된다. Link=1 이면 우측 값은 무시되고 좌측을 따라간다.

---

## ChannelStrip  (slot Strip=0 → +0)

내부 베이스: EQ=0, Compressor=100, Strip글로벌=200.

### Equalizer

| 노브 | chainParamId | 값 |
|---|---|---|
| Enable | **0** | 0/1 |
| Highpass 주파수 | **1** | 20..300 Hz |
| HP>EQ / HP>SC | **2** | 0=HP를 EQ경로, 1=HP를 컴프 사이드체인 |
| Lo shelf 주파수 | **3** | 35..300 Hz |
| Lo shelf 게인 | **4** | −15..+15 dB |
| Mid peak 주파수 | **5** | 250..8000 Hz |
| Mid peak 게인 | **6** | −15..+15 dB |
| (Mid bell 고정) | **7** | 미사용 시 1 |
| Hi shelf 주파수 | **8** | 4000..16000 Hz |
| Hi shelf 게인 | **9** | −15..+15 dB |

### Compressor

| 노브/토글 | chainParamId | 값 |
|---|---|---|
| On/Off 토글 | **100** | 0/1 |
| Threshold | **101** | −50..0 dBFS |
| Ratio (3:1/5:1/10:1) | **102** | 3 / 5 / 10 직접 전송 |
| Dry/Wet mix | **105** | 0..1 |
| (Attack, 패널 외) | 103 | 0.1..100 ms |
| (Release, 패널 외) | 104 | 10..1000 ms |
| (Makeup, 패널 외) | 106 | 0..24 dB |

### Strip 글로벌

| 노브 | chainParamId | 값 |
|---|---|---|
| 공통 Gain (입력) | **200** | dB |
| 공통 Out (출력) | **201** | dB |
| EQ↔COMP 순서 | **202** | 0=EQ→COMP, 1=COMP→EQ |

> **HP>SC 동작**: EQ paramId 2 를 1(=HP>SC)로 두면, 메인 신호에는 하이패스가 적용되지 않아 저음이 들리지만,
> 하이패스된 신호가 컴프레서 사이드체인으로 들어가 저음에 컴프가 과반응하는 것을 막는다.
> 이 라우팅은 순서가 EQ→COMP(202=0)일 때 가장 의도대로 동작한다.

---

## ModulationSet  (slot Mod=2 → +2000)

내부 베이스: Flanger=0, Phaser=100, Chorus=200, 세트글로벌=300.
세 모듈은 한 슬롯 안에서 직렬로 처리되며, 순서는 세트 글로벌에서 지정.

### Flanger

| 노브/버튼 | chainParamId | 값 |
|---|---|---|
| In/Out | **2000** | 0/1 |
| Rate | **2001** | sync off: 0.05..8 Hz / sync on: 0..6 노트분할 |
| BPM sync 버튼 | **2002** | 0/1 |
| Depth | **2003** | 0..1 |
| Feed(back) | **2004** | 0..0.95 |
| Phase | **2005** | 0..1 (좌우 LFO 위상차) |
| Mix | **2006** | 0..1 |

### Phaser

| 노브/버튼 | chainParamId | 값 |
|---|---|---|
| In/Out | **2100** | 0/1 |
| Rate | **2101** | sync off: 0.05..8 Hz / sync on: 0..6 |
| BPM sync 버튼 | **2102** | 0/1 |
| Depth | **2103** | 0..1 (스윕 폭, 옥타브) |
| Freq (≈스펙의 "fred") | **2104** | 100..5000 Hz (스윕 중심 주파수) |
| Feed(back) | **2105** | 0..0.95 |
| Phase | **2106** | 0..1 |
| Mix | **2107** | 0..1 |

### Chorus

| 노브/버튼 | chainParamId | 값 |
|---|---|---|
| In/Out | **2200** | 0/1 |
| Rate | **2201** | sync off: 0.05..8 Hz / sync on: 0..6 |
| BPM sync 버튼 | **2202** | 0/1 |
| Delay 1 | **2203** | 5..30 ms |
| Delay 2 | **2204** | 5..30 ms |
| Depth | **2205** | 0..1 |
| Feed(back) | **2206** | 0..0.9 |
| Lpf | **2207** | 1000..18000 Hz |
| Mix | **2208** | 0..1 |

### 세트 글로벌

| 노브 | chainParamId | 값 |
|---|---|---|
| Tempo(BPM) 공통 | **2300** | 20..300 (세 모듈 sync 에 공통 적용) |
| 순서 슬롯 0 | **2301** | 0=Flanger,1=Phaser,2=Chorus |
| 순서 슬롯 1 | **2302** | 〃 |
| 순서 슬롯 2 | **2303** | 〃 |

---

## Distortion  (slot Drive=1 → +1000)

| 노브/버튼 | chainParamId | 값 |
|---|---|---|
| Gain | **1000** | dB |
| Out | **1001** | dB |
| Tube/SoftClip/HardClip | **1002** | 0=Tube,1=SoftClip,2=HardClip |
| EQ Off/Pre/Post | **1003** | 0=Off,1=Pre,2=Post |
| EQ low/band/high 모프 | **1004** | 0=lowcut(HP)..0.5=band(BP)..1=highcut(LP) **연속** |
| EQ 주파수 | **1005** | 50..12000 Hz (기준 1개) |
| EQ resonance | **1006** | 0..1 |
| Drive | **1007** | 0..1 |
| Mix | **1008** | 0..1 |

> 스펙의 "eq lowcut/band/highcut 조절 노브(이산적이지 않게)" 는 노브 **하나**(1004)가
> 0→0.5→1 로 가면서 HP→BP→LP 로 연속 변형되도록 구현했다. 주파수는 별도 노브 1개(1005).

---

## 디스플레이 측 배선 예시 (의사코드)

```cpp
// 노브 1개 콜백
void onKnob(int chainParamId, float value) {
    chain.setParameter(chainParamId, value);  // 끝. 라우팅은 EffectChain 이 알아서.
}

// 이펙트 on/off 토글
void onEffectEnable(EffectChain::Slot slot, bool on) {
    chain.setSlotBypass(slot, /*bypass=*/!on);
}
```

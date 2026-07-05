# Nextion ↔ Bela Gem 연결 가이드 (처음 하는 사람 기준)

> 목표: Nextion 디스플레이를 **Bela Gem(Stereo) 에 직접** 연결해서,
> 화면 터치로 이펙트 체인을 편집하고 신스 소리가 그 체인을 통과해 나오게 한다.

---

## 0. 왜 ESP32 를 빼고 직결하나

팀원이 만든 ESP32 중계 방식의 장점(벨라 없이 개발, 디버깅 편의)은 실제로 유효하다.
하지만 우리가 원하는 기능(체인 추가/삭제/순서변경, 파라미터 실시간 반영)은
**체인의 "진짜 상태"가 Bela 안(PluginChain)에 있어야** 하고, 화면은 그 상태를
비추는 거울이어야 한다. 중간에 ESP32 가 끼면:

1. 같은 상태를 세 군데(Nextion, ESP32, Bela)서 관리해야 해서 동기화 버그가 생기기 쉽고
2. ESP32 펌웨어라는 유지보수 대상이 하나 더 늘고
3. UART 홉이 하나 늘어 지연/디버깅 지점이 추가된다.

Nextion 은 어차피 UART TTL 4선(5V, GND, TX, RX)만 있으면 되고, Bela 는 리눅스라
시리얼 포트를 그냥 열면 된다. 레포에 이미 그 용도의 `SerialPort` 계층도 있다.

**ESP32 는 버리지 말 것** — Bela 없이 Nextion HMI 를 개발/테스트할 때
"화면만 먼저 굴려보는 개발용 지그"로 계속 쓰면 된다 (팀원 장점 1, 2 그대로 활용).
최종 악기에서만 빠진다.

---

## 1. 준비물

| 품목 | 비고 |
|---|---|
| Nextion 디스플레이 (800×480, 7인치급) | 팀원이 이미 보유 |
| 점퍼 와이어 4가닥 (암-암 또는 상황에 맞게) | Nextion 쪽은 JST-XH 4핀 케이블이 동봉됨 |
| 저항 2개 (1kΩ + 2kΩ, 또는 10kΩ + 20kΩ) | **전압 분배용 — 필수 안전장치** |
| 5V 전원 (2A 이상 어댑터 또는 여유 있는 USB 전원) | 7인치 Nextion 은 최대 ~0.5A 를 먹는다 |
| microSD 카드 (**32GB 이하, FAT32**) | Nextion 펌웨어(.tft) 플래싱용. 아래 5절 참고 |

---

## 2. 어느 핀에 연결하나 (Bela Gem = PocketBeagle 2)

Bela Gem 은 PocketBeagle 2(PB2) 위에 얹히는 케이프이고, PB2 의 P1/P2 헤더 핀이
Gem 보드 가장자리로 그대로 나와 있다. 우리는 **UART4** 를 쓴다.
Bela 공식 문서가 Gem 에서 UART4 를 활성화하는 예제를 제공하는 조합이라 가장 안전하다:

| 신호 | PB2/Gem 핀 | 연결 상대 (Nextion) |
|---|---|---|
| UART4 **TX** (Bela가 말함) | **P1.20** | Nextion **RX** (보통 노란 선) |
| UART4 **RX** (Bela가 들음) | **P2.20** | Nextion **TX** (보통 파란 선) — **반드시 분압 저항 경유!** |
| GND | Gem 의 아무 GND 핀 (P1.15/P1.16/P1.22 등) | Nextion **GND** (검정) |
| 5V | 외부 5V 어댑터 (권장) | Nextion **+5V** (빨강) |

핀 위치는 Bela IDE 의 pin diagram 탭 또는 보드 실크스크린(P1.20 / P2.20)으로 확인.
TX↔RX 가 **서로 교차**되는 게 맞다 (내 입 → 상대 귀).

### ⚠️ 전압 주의 — 이게 제일 중요하다

- PB2 의 모든 I/O 핀은 **3.3V 전용**이다. 공식 문서 경고: 5V 로직을 물리면 보드가 죽는다.
- Nextion 은 5V 로 구동되며, 개체에 따라 TX 출력이 3.3V 수준인 것도 있지만
  스펙상 5V TTL 로 취급하는 게 안전하다.
- 그래서 **Nextion TX → Bela RX(P2.20) 사이에 전압 분배기**를 넣는다:

```
Nextion TX ──[ R1 = 1kΩ ]──●──→ Bela P2.20 (UART4 RX)
                           │
                        [ R2 = 2kΩ ]
                           │
                          GND
```

5V × 2k/(1k+2k) = 3.33V. (10k/20k 조합도 동일 비율이라 OK.)
반대 방향(Bela TX 3.3V → Nextion RX)은 그대로 직결해도 된다 — Nextion RX 는
3.3V 신호를 인식한다 (팀원이 ESP32(3.3V)로 이미 구동 성공한 것이 그 증거).

### 전원

- 7인치 Nextion 은 백라이트 때문에 최대 ~500mA 를 먹는다.
- **별도 5V 어댑터로 Nextion 에 전원을 주고, GND 만 Bela 와 공통으로 묶는 것을 권장**
  (오디오 장비라 백라이트 전류가 Bela 5V 레일을 타면 노이즈가 낄 수 있다).
- 급하면 Bela 를 5V/3A USB-C 어댑터로 구동하면서 Gem 의 5V(VIN) 핀에서 따와도
  동작은 하지만, 노이즈/전류 여유 면에서 차선책이다.
- **전원 인가 순서**: 배선을 모두 끝낸 뒤에 전원을 넣는다. 보드에 전원이 없는 상태에서
  핀에 전압을 걸면 안 된다 (PB2 공식 경고 사항).

---

## 3. Bela 쪽 소프트웨어: UART4 를 /dev/ttyS4 로 살리기

PB2 의 핀은 부팅 시 "디바이스 트리"라는 설정으로 기능이 정해진다.
기본 상태에선 P1.20/P2.20 이 UART 가 아니므로, **오버레이 하나를 만들어 올린다.**
아래는 Bela 공식 문서(learn.bela.io → Device tree overlays)의 절차 그대로다.

Bela IDE 하단 콘솔(또는 `ssh root@bela.local`)에서:

```bash
# 1) 부트 설정 백업 (실수 대비)
cp /boot/firmware/extlinux/extlinux.conf /boot/firmware/extlinux/extlinux.conf.bak

# 2) 오버레이 소스 생성
cd /opt/source/dtb-6.12-Beagle
touch src/arm64/overlays/PB2-UART4.dtso
```

`src/arm64/overlays/PB2-UART4.dtso` 에 아래 내용을 그대로 넣는다:

```dts
/*
 * PB2-UART4.dts - Device Tree Overlay for UART4 (ttyS4)
 * Target: PocketBeagle 2 (TI AM6254)
 *   UART4: P2.20 (RX) / P1.20 (TX) -> /dev/ttyS4
 */
#include <dt-bindings/gpio/gpio.h>
#include <dt-bindings/leds/common.h>
#include "ti/k3-pinctrl.h"

/dts-v1/;
/plugin/;

&{/chosen} {
    overlays {
        PB2-UART4.kernel = __TIMESTAMP__;
    };
};

&main_pmx0 {
    P1_20_uart4_txd: P1-20-uart4-txd-pins {
        pinctrl-single,pins = <
            AM62X_IOPAD(0x00CC, PIN_OUTPUT, 4) /* (Y24) VOUT0_DATA5.UART4_TXD */
        >;
    };
    P2_20_uart4_rxd: P2-20-uart4-rxd-pins {
        pinctrl-single,pins = <
            AM62X_IOPAD(0x00C8, PIN_INPUT, 4) /* (Y25) VOUT0_DATA4.UART4_RXD */
        >;
    };
};

&main_uart4 {
    status = "okay";
    pinctrl-names = "default";
    pinctrl-0 = <&P1_20_uart4_txd>, <&P2_20_uart4_rxd>;
};
```

```bash
# 3) 빌드 + 설치 (성공하면 /boot/firmware/overlays/PB2-UART4.dtbo 생성)
cd /opt/source/dtb-6.12-Beagle
./build_n_install.sh
```

4) `/boot/firmware/extlinux/extlinux.conf` 를 열어 `label microSD (default)`
섹션의 `fdtoverlays` 줄 **끝에** 추가:

```
fdtoverlays /overlays/PB2-BELA.dtbo /overlays/PB2-I2C-GPIO.dtbo /overlays/PB2-UART4.dtbo
```

5) 재부팅 후 확인:

```bash
ls /proc/device-tree/chosen/overlays/    # PB2-UART4 가 보여야 함
ls /dev/ttyS4                            # 장치 파일이 생겨야 함
```

6) **루프백 자가 테스트** (Nextion 연결 전, 배선이 맞는지 검증):
P1.20 과 P2.20 을 점퍼선 하나로 직접 이어 두고,

```bash
cat /dev/ttyS4 & echo "hello" > /dev/ttyS4
```

`hello` 가 되돌아 나오면 UART4 가 정상이다. 점퍼선을 빼고 Nextion 을 연결한다.

> ⚠️ P1.20/P2.20 이 Bela 디지털 채널 중 하나에 해당할 수 있다. 오버레이를 올린 뒤엔
> 그 두 핀에 대응하는 디지털 채널은 코드에서 쓰지 말 것. 우리 프로젝트의 MUX
> 셀렉트는 D0~D3 이므로 IDE pin diagram 에서 D0~D3 위치가 P1.20/P2.20 과
> 겹치지 않는지 한 번만 확인해 두자 (겹치면 `HardwareInput.h` 의 `kMuxPins` 를
> 다른 디지털 핀으로 옮기면 된다).

---

## 4. 프로젝트 코드 배치

이번에 추가/수정된 파일:

```
effects/PluginChain.h            (신규) 동적 이펙트 체인 — 추가/삭제/이동, 최대 8슬롯
effects/PluginChain.cpp          (신규)
display/src/EffectDefs.h         (신규) 이펙트/파라미터 정의 테이블 (UI 의 단일 출처)
display/src/ChainUiController.h  (신규) Nextion ↔ PluginChain 컨트롤러
display/src/ChainUiController.cpp(신규)
synthesis/SynthEngine.h          (수정) EffectChain → PluginChain 교체 (3줄)
render.cpp                       (교체) 디스플레이 태스크 통합 정식 버전
```

- `SynthEngine.cpp` 는 **수정할 필요 없다** (setup/process/cleanup 시그니처 동일).
- 구버전 파일 정리 권장: `display/src/DisplayController.*`, `EffectMenu.*`,
  `NextionLink.*`, `EffectParamSink.h`, `comm/CommHandler.*`, `effects/EffectChain.*`
  는 이제 안 쓰인다. 그대로 둬도 컴파일은 되지만, 팀 관례대로 `.bak` 확장자를 붙여
  빌드에서 제외해 두면 깔끔하다. (`SerialPort.*` 는 계속 사용하니 남길 것!)
- 하위 폴더 `.cpp` 는 상대경로 include 를 쓰는 기존 규칙을 그대로 따랐으므로
  Bela Makefile 에서 추가 설정 없이 빌드된다.

**신호 흐름 확인**: `SynthEngine::render()` 안에서
`보이스 합산 → fx_.process() → 마스터볼륨` 순서이므로, 화면에서 추가한 모든
이펙트를 통과한 소리가 최종 출력된다. 체인이 비어 있으면 드라이 사운드 그대로.

---

## 5. Nextion 켜기 & 펌웨어(.tft) 넣기 — 진짜 처음부터

Nextion 은 "화면 UI 프로그램(.HMI → 컴파일하면 .tft)"을 자기 안에 저장해서 도는
독립 장치다. 켜는 데 특별한 절차는 없다: **5V 와 GND 만 주면 저장된 UI 가 뜬다.**

UI 를 갱신(플래싱)하는 방법 (팀원 설명 그대로, 조금 더 자세히):

1. Windows PC 에서 **Nextion Editor** 로 `.HMI` 프로젝트를 연다
   (맥은 네이티브 지원이 없다 — 팀원 PC 나 Windows VM 사용.
   ESP32 없이 **에디터의 Debug 버튼**으로 PC 상에서 화면 동작을 시뮬레이션할 수 있고,
   시뮬레이터의 "User MCU Input/Output" 창으로 우리 프로토콜 바이트도 눈으로 확인 가능).
2. 메뉴 File → **TFT File Output** 으로 `.tft` 파일 생성.
3. microSD 를 **FAT32** 로 포맷하고 `.tft` **한 개만** 루트에 복사.
   - Nextion 은 공식적으로 **32GB 이하 FAT32** 만 지원한다. 팀원이 256GB 를
     포맷해서 임시로 쓰고 있다는데, 인식이 불안정하면 32GB 이하 카드로 바꾸거나
     256GB 카드에 **32GB 짜리 FAT32 파티션 하나만** 만들어 쓰면 된다.
4. Nextion 전원을 **끄고**, SD 를 꽂고, 전원 인가 → 화면에 업데이트 진행률이 뜬다.
5. 100% 가 되면 전원을 끄고 SD 를 **뺀 뒤** 다시 전원 인가 → 새 UI 로 부팅.

시리얼 속도: 우리 프로젝트는 **115200** 을 쓴다. HMI 프로젝트의 program.s 첫 줄에
`baud=115200` 이 아니라 **`bauds=115200`** (s 붙은 쪽 = 저장되는 설정) 을 넣을 것.
Bela 쪽은 `render.cpp` 의 `kNextionBaud = 115200` 과 일치해야 한다.

---

## 6. 최초 통합 테스트 순서 (문제 분리용)

1. **UART 루프백** (3-6절) — 배선/오버레이 확인. ✅ 후 다음.
2. **Nextion 수신 확인**: Nextion 연결 후 Bela 터미널에서
   `echo -ne 'page 0\xff\xff\xff' > /dev/ttyS4`
   → 화면이 page 0 으로 전환되면 **Bela→Nextion 방향 OK**.
3. **Nextion 송신 확인**: `cat /dev/ttyS4 | xxd` 를 띄워 두고 화면의 아무
   슬라이더를 움직인다 → `a5 06 ...` 바이트가 보이면 **Nextion→Bela 방향 OK**
   (분압 저항 방향이 맞다는 뜻이기도 하다).
4. **프로젝트 실행**: IDE 에서 실행 → 콘솔에 `🖥️ Nextion 연결됨` 이 떠야 한다.
   화면에서 `+` → SPACES → REVERB 추가 → Trill 바를 눌러 소리 확인 →
   MIX 슬라이더를 올렸을 때 잔향이 커지면 전 구간 완성이다.

문제가 생기면 위 단계 중 **어디까지 성공했는지**로 원인 구간을 좁힌다:
1 실패=배선/오버레이, 2 실패=Bela TX 선, 3 실패=Nextion TX 선/분압기, 4 실패=코드/보레이트.

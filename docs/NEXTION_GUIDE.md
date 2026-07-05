# Nextion HMI 제작 가이드 — 이펙트 체인 UI

> 이 문서대로 Nextion Editor 에서 페이지/컴포넌트/이벤트 코드를 만들면,
> Bela 쪽 `ChainUiController` 와 그대로 맞물려 동작한다.
> **컴포넌트 이름과 이벤트 코드는 글자 하나까지 정확히** — Bela 가 이 이름들로
> 화면을 그리기 때문이다 (`p0.pic=7`, `va_sel.val=2` 같은 명령을 보냄).

- 작업 환경: **Nextion Editor 는 Windows 전용.** HMI 편집/컴파일만 윈도우 PC 에서 하고,
  나머지(코드, Bela IDE)는 전부 맥에서 하면 된다.
- 에디터의 **Debug 버튼**을 누르면 PC 시뮬레이터가 뜬다. 시뮬레이터 하단
  "Instruction Input Area / Simulator Return Data" 창에서 우리 프로토콜 바이트가
  실제로 나가는지 보드 없이 확인할 수 있다 (ESP32 지그도 필요 없음).

---

## 1. 프로젝트 생성 & 기본 설정

1. New Project → 모델: 보유한 Nextion 모델 선택 (7인치 800×480), 방향 Horizontal.
2. 좌측 하단 **Program.s** 탭을 열고 `baud=9600` 줄을 다음으로 교체:

```
bauds=115200
```

   (`bauds` = 저장되는 설정. `baud` 는 재부팅하면 날아간다.
    Bela 쪽 `render.cpp` 의 `kNextionBaud = 115200` 과 반드시 일치.)

## 2. 이미지 리소스 등록 (순서 중요)

좌측 하단 Picture 패널에서 **이 순서대로** import 하면 ID 가 자동으로 붙는다:

| 순서 | 이미지 | 크기 | ID |
|---|---|---|---|
| 1 | 메인 페이지 배경 (통짜 이미지) | 800×480 | **0** |
| 2~9 | 육각형 아이콘 일반판: EQ, COMP, DIST, CHORUS, FLANGER, PHASER, REVERB, DELAY | 88×88 | **1~8** |
| 10~17 | 같은 순서의 글로우(선택)판 | 88×88 | **9~16** |

등록 후 Picture 패널에 표시되는 실제 ID 를 보고, Bela 쪽
`display/src/EffectDefs.h` 의 세 상수를 맞춰라 (TODO 표시해 둠):

```cpp
static const int kPicNormal[]   = { 1, 2, 3, 4, 5, 6, 7, 8 };
static const int kPicSelected[] = { 9,10,11,12,13,14,15,16 };   // ← 실제 ID 로 수정
static const int kBgPic = 0;
```

> 배경이 "통짜 이미지"여야 하는 이유: 드래그 중 아이콘 잔상을
> `picq`(배경 이미지의 해당 영역만 다시 그리기) 명령으로 지우기 때문.
> 페이지 배경은 색상(sta=color)이 아니라 **image** 로, pic=0 을 지정할 것.

## 3. 페이지 구성

총 3페이지. 페이지 이름도 그대로 쓸 것: `main`(page0), `pCat`(page1), `pFx`(page2).

### 3.1 page0 `main` — 메인 화면

배경: pic=0 (2절의 배경 이미지).

**변수 (Variable 컴포넌트, 전부 vscope=global, val 기본값 0):**

| 이름 | 용도 |
|---|---|
| `va_sel` | 선택 슬롯 (Bela 가 동기화해 줌) |
| `va_pg` | 파라미터 페이지 (Bela 가 클램프 후 동기화) |
| `va_drag` | 드래그 시작 슬롯 |
| `va_drg` | 드래그 중 플래그 |
| `va_pick` | 선택한 카테고리 (pFx 페이지에서 참조) |
| `va_pic` | 드래그 고스트 아이콘 ID |
| `va_gx`, `va_gy` | 고스트 좌표/임시 계산 |

**타이머 (Timer 컴포넌트):**

| 이름 | tim | en(기본) | 용도 |
|---|---|---|---|
| `tm_lp` | 600 | 0 | 롱프레스 판정 (0.6초) |
| `tm_drag` | 50 | 0 | 드래그 중 고스트 갱신 (20fps) |

**표시/조작 컴포넌트 (이름, 타입, 권장 좌표 — 좌표는 취향껏 조정 가능, 이름은 불변):**

| 이름 | 타입 | x,y,w,h | 비고 |
|---|---|---|---|
| `thdr` | Text | 8,4,460,32 | 선택 이펙트 헤더 |
| `bbyp` | Dual-state Button | 480,4,90,32 | 텍스트 "BYP" |
| `b_del` | Button | 580,4,90,32 | 텍스트 "DEL" |
| `b_add` | Button | 700,4,90,32 | 텍스트 "+" |
| `p0`…`p7` | Picture | x=8+i×98, y=48, 88×88 | 슬롯 아이콘 (pic 아무거나, Bela 가 덮어씀) |
| `t0`…`t7` | Text | x=8+i×98, y=138, 88×20 | 슬롯 이름 (가운데 정렬) |
| `tpg` | Text | 8,170,80,28 | "1/4" 페이지 표시 |
| `b_prev` | Button | 640,170,70,28 | "<" |
| `b_next` | Button | 720,170,70,28 | ">" |
| `lp0`…`lp3` | Text | 8, y=210+r×66, 150×28 | 파라미터 라벨 |
| `h0`…`h3` | Slider | 170, y=210+r×66, 450×40 | **minval=0, maxval=1000** |
| `vp0`…`vp3` | Text | 640, y=210+r×66, 150×28 | 값 표시 |

모든 컴포넌트의 **Send Component ID 체크는 전부 해제** (Touch Press/Release 둘 다).
통신은 아래 이벤트 코드의 `printh`/`print` 로만 한다.

**이벤트 코드:**

`main` 페이지 자체의 Preinitialize Event — 진입할 때마다 Bela 에 전체 재전송 요청:

```
printh a5 08
```

`b_add` Touch Release:

```
page pCat
```

`b_del` Touch Release — "현재 선택 슬롯 삭제" (-1 = 선택 슬롯, Bela 가 해석):

```
printh a5 02 ff ff ff ff
```

`bbyp` Touch Release:

```
printh a5 07
print bbyp.val
```

`b_prev` Touch Release:

```
if(va_pg.val>0)
{
  va_pg.val--
}
printh a5 05
print va_pg.val
```

`b_next` Touch Release (범위 초과는 Bela 가 클램프해서 va_pg 를 되돌려줌):

```
va_pg.val++
printh a5 05
print va_pg.val
```

슬라이더 `h0` — **Touch Move 와 Touch Release 양쪽에 같은 코드**:

```
printh a5 06 00 00 00 00
print h0.val
```

`h1`, `h2`, `h3` 도 동일하되 네 번째 바이트(행 번호)만 바꾼다:

```
h1:  printh a5 06 01 00 00 00   +  print h1.val
h2:  printh a5 06 02 00 00 00   +  print h2.val
h3:  printh a5 06 03 00 00 00   +  print h3.val
```

**슬롯 아이콘 `p0`…`p7` — 탭(선택)과 롱프레스 드래그의 시작점.**
`p3` 기준 예시 (다른 슬롯은 숫자 3만 해당 인덱스로 교체):

Touch Press:

```
va_drag.val=3
tm_lp.en=1
```

Touch Release:

```
tm_lp.en=0
if(va_drg.val==1)
{
  tm_drag.en=0
  va_drg.val=0
  ref 0
  va_gx.val=tch2
  va_gx.val-=8
  if(va_gx.val<0)
  {
    va_gx.val=0
  }
  va_gx.val/=98
  printh a5 03
  print va_drag.val
  print va_gx.val
}else
{
  va_sel.val=3
  printh a5 04
  print va_sel.val
}
```

> 동작 원리: 손을 뗀 좌표(`tch2`)에서 슬롯 폭(98px)으로 나눠 드롭 위치를 구하고
> MOVE(0x03)를 보낸다. `ref 0` 은 페이지 전체를 다시 그려 드래그 잔상을 지우고,
> 직후 Bela 가 새 순서로 아이콘을 다시 그린다. 화면 밖에 떨어뜨려도 Bela 가
> 유효 범위로 클램프하니 안전하다. 롱프레스가 아니었으면 그냥 "선택(0x04)".

`tm_lp` Timer Event — 0.6초 이상 눌렀다 = 드래그 시작:

```
tm_lp.en=0
va_drg.val=1
if(va_drag.val==0)
{
  va_pic.val=p0.pic
}
if(va_drag.val==1)
{
  va_pic.val=p1.pic
}
if(va_drag.val==2)
{
  va_pic.val=p2.pic
}
if(va_drag.val==3)
{
  va_pic.val=p3.pic
}
if(va_drag.val==4)
{
  va_pic.val=p4.pic
}
if(va_drag.val==5)
{
  va_pic.val=p5.pic
}
if(va_drag.val==6)
{
  va_pic.val=p6.pic
}
if(va_drag.val==7)
{
  va_pic.val=p7.pic
}
va_gx.val=-1
tm_drag.en=1
```

(Nextion 은 배열 인덱싱이 없어서 if 8개로 "잡은 슬롯의 아이콘 ID"를 복사한다.)

`tm_drag` Timer Event — 손가락 따라 고스트 아이콘 이동:

```
if(va_gx.val>=0)
{
  picq va_gx.val,va_gy.val,88,88,0
}
if(tch0!=0||tch1!=0)
{
  va_gx.val=tch0-44
  va_gy.val=tch1-44
  if(va_gx.val<0)
  {
    va_gx.val=0
  }
  if(va_gy.val<0)
  {
    va_gy.val=0
  }
  if(va_gx.val>712)
  {
    va_gx.val=712
  }
  if(va_gy.val>392)
  {
    va_gy.val=392
  }
  pic va_gx.val,va_gy.val,va_pic.val
}
```

> `picq` 가 직전 고스트 자리를 배경 이미지(ID 0)로 복원하고, `pic` 이 현재
> 손가락 위치(중심 보정 -44)에 아이콘을 다시 그린다. 고스트가 다른 아이콘 위를
> 지나가면 잠깐 지워져 보일 수 있는데, 드롭 순간 `ref 0` + Bela 재전송으로
> 전부 복구되므로 미관상 문제일 뿐 기능엔 영향 없다.

### 3.2 page1 `pCat` — 카테고리 선택 팝업

버튼 4개면 끝. 배경은 반투명 느낌의 이미지나 단색 아무거나.

| 이름 | 텍스트 | Touch Release 코드 |
|---|---|---|
| `bCat0` | CH STRIP | `main.va_pick.val=0` ↵ `page pFx` |
| `bCat1` | MODULATION | `main.va_pick.val=1` ↵ `page pFx` |
| `bCat2` | SPACES | `main.va_pick.val=2` ↵ `page pFx` |
| `b_x` | CANCEL | `page main` |

### 3.3 page2 `pFx` — 이펙트 선택 팝업

버튼 `bFx0`, `bFx1`, `bFx2` + `b_x`(CANCEL, `page main`).

페이지 Preinitialize Event — 카테고리에 맞춰 버튼 라벨을 바꾼다:

```
if(main.va_pick.val==0)
{
  bFx0.txt="EQ"
  bFx1.txt="COMP"
  bFx2.txt="DIST"
  vis bFx2,1
}
if(main.va_pick.val==1)
{
  bFx0.txt="CHORUS"
  bFx1.txt="FLANGER"
  bFx2.txt="PHASER"
  vis bFx2,1
}
if(main.va_pick.val==2)
{
  bFx0.txt="REVERB"
  bFx1.txt="DELAY"
  vis bFx2,0
}
```

(SPACES 는 이펙트가 2개뿐이라 세 번째 버튼을 숨긴다.)

`bFx0` Touch Release — 이펙트 타입 = 카테고리×3 + 버튼번호 규칙으로 ADD 전송:

```
main.va_pick.val*=3
printh a5 01
print main.va_pick.val
page main
```

`bFx1` Touch Release:

```
main.va_pick.val*=3
main.va_pick.val++
printh a5 01
print main.va_pick.val
page main
```

`bFx2` Touch Release:

```
main.va_pick.val*=3
main.va_pick.val+=2
printh a5 01
print main.va_pick.val
page main
```

> `page main` 으로 돌아가는 순간 main 의 Preinitialize 가 SYNC(0xA5 0x08)를 보내고,
> Bela 가 방금 추가된 이펙트를 선택 상태로 전체 화면을 다시 그린다.
> 타입 번호표: 0 EQ, 1 COMP, 2 DIST, 3 CHORUS, 4 FLANGER, 5 PHASER, 6 REVERB, 7 DELAY
> (= `fx::EffectType`, `EffectDefs.h` 와 동일).

---

## 4. 프로토콜 참고표 (Nextion → Bela)

모든 프레임: `0xA5` + 명령 1바이트 + 페이로드.
`printh` 는 16진수 바이트를 그대로, `print 변수.val` 은 **4바이트 리틀엔디언 정수**를 보낸다.

| cmd | 의미 | 페이로드 |
|---|---|---|
| 0x01 | ADD | type (4B) |
| 0x02 | REMOVE | slot (4B, -1=현재 선택) |
| 0x03 | MOVE | from (4B), to (4B) |
| 0x04 | SELECT | slot (4B) |
| 0x05 | PAGE | page (4B) |
| 0x06 | PARAM | row 0..3 (4B), raw 0..1000 (4B) |
| 0x07 | BYPASS | on (4B) |
| 0x08 | SYNC | 없음 |

Bela → Nextion 은 표준 Nextion 명령(ASCII + `0xFF 0xFF 0xFF`)만 쓰므로
Nextion 쪽에서 따로 만들 것이 없다.

## 5. 컴파일 & 플래싱

1. 상단 **Compile** → 에러 0 확인. **Debug** 로 시뮬레이터에서 동작 확인
   (슬라이더를 움직여 Return Data 창에 `a5 06 …` 이 찍히는지 볼 것).
2. File → **TFT File Output** 으로 `.tft` 생성.
3. FAT32 microSD(32GB 이하 권장) 루트에 `.tft` **한 개만** 복사 → Nextion 전원 OFF →
   SD 삽입 → 전원 ON → 진행률 100% → 전원 OFF → SD 제거 → 전원 ON.
   (자세한 내용과 주의사항은 CONNECTION_GUIDE.md 5절.)

## 6. 자주 나는 문제

| 증상 | 원인/해결 |
|---|---|
| 터치해도 Bela 무반응 | ① 보레이트 불일치(program.s `bauds=115200` 확인) ② Nextion TX→분압저항→P2.20 배선 ③ 오버레이 미적용(`ls /dev/ttyS4`) |
| Bela 는 로그가 찍히는데 화면이 안 바뀜 | Bela TX(P1.20)→Nextion RX 선 확인. `echo -ne 'page 0\xff\xff\xff' > /dev/ttyS4` 로 단독 테스트 |
| 아이콘이 엉뚱한 그림으로 나옴 | `EffectDefs.h` 의 `kPicNormal/kPicSelected` 가 에디터의 실제 Picture ID 와 불일치 |
| 드래그 잔상이 남음 | 페이지 배경이 통짜 이미지(pic=0)가 아님 → `picq` 가 복원할 원본이 없어서 생기는 현상 |
| 슬라이더가 512 근처에서만 움직임 | 슬라이더 maxval 이 기본 100 으로 남아 있음 → **maxval=1000** 으로 |

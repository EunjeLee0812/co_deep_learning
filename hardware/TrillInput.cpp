// TrillInput.cpp
// ═══════════════════════════════════════════════════════════════════════════
//  Trill 연주센서 입력 — Ring 1개 + Bar 4개 (전부 활성)
//
//  [배치]  Ring        → 5도권(코드=베이스 루트) 선택        · frame_.ring*
//          Bar(Bass)   → 베이스(1음) 중심 · 서브오실 있음     · frame_.bass
//          Bar(R5)     → 5음  중심                            · frame_.r5
//          Bar(R8)     → 8음(옥타브) 중심                     · frame_.r8
//          Bar(R3)     → 3음  중심                            · frame_.r3
//
//  ★ poll() 은 오디오 스레드가 아니라 AuxiliaryTask 에서 호출된다.
//    (이 프로젝트에선 render.cpp 의 gTrillTask 가 매 블록 poll() 을 예약함)
//    ※ 예전 코드처럼 이 파일에서 또 자체 태스크를 만들면 poll() 이 이중 호출되어
//      같은 I2C 를 두 스레드가 동시에 때린다. 그래서 자체 태스크는 두지 않는다.
// ═══════════════════════════════════════════════════════════════════════════
#include "TrillInput.h"
#include <Bela.h>
#include <libraries/Trill/Trill.h>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace hw {

// ┌───────────────────────────────────────────────────────────────────────┐
// │  ★★★ 여기만 고치면 됩니다 : 역할 ↔ I2C 주소 표 ★★★                     │
// │                                                                         │
// │  절차:                                                                  │
// │   1) 일단 이대로 빌드→실행. 부팅 로그의 [스캔] 목록에서 버스에 실제로   │
// │      붙어있는 주소/종류를 확인한다. (예: 0x20 BAR, 0x21 BAR, 0x38 RING) │
// │   2) kIdentifyMode = true 인 상태로 바를 하나씩 만져본다. 콘솔에        │
// │      "만진 주소 = 지금 배정된 역할" 이 찍힌다. 원하는 역할과 다르면     │
// │      아래 주소 숫자를 바꿔 역할을 재배정한다. (물리 배선은 안 건드려도  │
// │      됨 — 주소만 바꿔 끼우면 된다.)                                     │
// │   3) 4개 바 + 링 매핑이 다 맞으면 kIdentifyMode = false 로.             │
// └───────────────────────────────────────────────────────────────────────┘
namespace {

constexpr unsigned int kI2cBus = 1;   // Bela 기본 I2C 버스 (핀다이어그램 SDA/SCL)

// ── 역할별 I2C 주소 ──  (Bar 기본 0x20, 점퍼로 +1씩; Ring 기본 0x38)
//
// [2143 → 1234 교정]  물리 바를 순서대로 눌렀을 때 음높이가 2 1 4 3 으로 나와서,
//   "물리 순서대로 음높이 오름차순(1 2 3 4)"이 되도록 주소를 스왑했다.
//   (물리 위치는 고정 · 주소만 재배정 → 배선은 안 건드림)
//     Bass↔R5 : 0x20 ↔ 0x21 스왑   /   R8↔R3 : 0x22 ↔ 0x23 스왑
//   ※ 이 스왑은 "현재 하드웨어가 이 파일의 이전 주소값대로 붙어있다"는 전제에서
//     역산한 것. 실제로 순서가 여전히 안 맞으면 kIdentifyMode=true 로 각 바를
//     만져 [식별] 로그의 주소를 보고 아래 4줄만 다시 맞추면 된다.
constexpr uint8_t kAddrRing = 0x38;   // Trill Ring → 5도권(코드 선택)
constexpr uint8_t kAddrBass = 0x21;   // Trill Bar  → 베이스(1음)   [was 0x20]
constexpr uint8_t kAddrR5   = 0x20;   // Trill Bar  → 5음          [was 0x21]
constexpr uint8_t kAddrR8   = 0x23;   // Trill Bar  → 8음(옥타브)   [was 0x22]
constexpr uint8_t kAddrR3   = 0x22;   // Trill Bar  → 3음          [was 0x23]

// ── 바가 물리적으로 뒤집혀 달렸으면 true (규약: 위=낮음 0.0 / 아래=높음 1.0) ──
//    식별 모드에서 위로 밀었는데 pos 가 커지면 그 바를 true 로 바꾸면 된다.
constexpr bool kFlipBass = false;
constexpr bool kFlipR5   = false;
constexpr bool kFlipR8   = false;
constexpr bool kFlipR3   = false;

// ── 식별 모드 : 만진 바의 (주소=역할)을 콘솔에 찍어 물리↔주소를 찾게 해줌 ──
//    매핑을 다 확정했으면 false 로 (로그가 조용해짐).
constexpr bool kIdentifyMode = true;

// 터치 면적 → 0..1 벨로시티 정규화 기준값 (손가락 면적 맞춰 조절)
constexpr float kStrengthFullScale = 0.05f;

inline float clamp01(float v) { return std::min(1.0f, std::max(0.0f, v)); }

} // anonymous namespace

// 1. 실제 센서 객체 (Ring 1 + Bar 4 = 5개)
static Trill gRing;
static Trill gBass;
static Trill gR5;
static Trill gR8;
static Trill gR3;

// 각 센서가 setup() 에서 정상 초기화됐는지 기억 → 초기화 실패한 건 poll() 에서 건너뜀
static bool gRingOk = false;
static bool gBassOk = false;
static bool gR5Ok   = false;
static bool gR8Ok   = false;
static bool gR3Ok   = false;

namespace {

// Trill::Device → 사람이 읽는 이름. 못 알아본(없는) 종류면 nullptr.
//   ※ Trill::NONE 상수를 직접 참조하지 않으려고 화이트리스트 방식으로 구현.
const char* deviceName(Trill::Device d) {
    switch (d) {
        case Trill::BAR:    return "BAR";
        case Trill::RING:   return "RING";
        case Trill::SQUARE: return "SQUARE";
        case Trill::HEX:    return "HEX";
        case Trill::CRAFT:  return "CRAFT";
        case Trill::FLEX:   return "FLEX";
        default:            return nullptr;   // NONE/UNKNOWN → 없음 취급
    }
}

// 버스 0x20~0x50 을 스캔해 붙어있는 Trill 을 전부 콘솔에 출력.
//   → "주소를 바꿔놓고 뭐가 어디인지 까먹었을 때" 용. (공식 detect-all-devices 방식)
//   ※ 만약 쓰는 Bela 이미지에서 Trill::probe 시그니처가 달라 빌드가 안 되면,
//     이 함수 본문만 통째로 주석 처리해도 된다(진단용일 뿐, 실제 init 은 setup()이 함).
void scanBus() {
    rt_printf("──────────── [스캔] I2C 버스 %u 의 Trill 목록 ────────────\n", kI2cBus);
    rt_printf("  주소(hex/dec) | 종류\n");
    unsigned int total = 0;
    for (uint8_t addr = 0x20; addr <= 0x50; ++addr) {
        Trill::Device d = Trill::probe(kI2cBus, addr);
        const char* nm = deviceName(d);
        if (nm) {
            rt_printf("  0x%02X (%3d)    | %s\n", addr, addr, nm);
            ++total;
        }
    }
    rt_printf("  → 총 %u 개 발견.\n", total);
    rt_printf("─────────────────────────────────────────────────────────\n");
}

// 센서 1개를 초기화(관용적: 실패해도 죽지 않고 false 만 돌려줌).
bool initSensor(Trill& s, Trill::Device type, uint8_t addr, const char* role) {
    if (s.setup(kI2cBus, type, addr) != 0) {
        rt_printf("⚠️  [%s] 초기화 실패 (주소 0x%02X). 배선/주소 확인 — 일단 건너뜀.\n",
                  role, addr);
        return false;
    }
    s.setMode(Trill::CENTROID);          // 위치+면적 둘 다 읽기
    rt_printf("✅ [%s] 준비됨 (주소 0x%02X)\n", role, addr);
    return true;
}

// Bar 1개 읽어 BarTouch 로 정리 (+ 식별모드면 만진 주소=역할 출력).
void readBar(Trill& s, bool ok, BarTouch& out, bool flip,
             uint8_t addr, const char* role) {
    if (!ok) { out.active = false; out.strength = 0.0f; return; }
    s.readI2C();
    if (s.getNumTouches() > 0) {
        out.active   = true;
        float p      = clamp01(s.touchLocation(0));
        out.pos      = flip ? (1.0f - p) : p;
        out.strength = clamp01(s.touchSize(0) / kStrengthFullScale);
        if (kIdentifyMode)
            rt_printf("[식별] 0x%02X = 역할:%-4s  pos=%.2f\n", addr, role, out.pos);
    } else {
        out.active   = false;
        out.strength = 0.0f;
    }
}

} // anonymous namespace

bool TrillInput::setup(BelaContext* /*context*/) {
    // ── 0) 버스에 실제로 뭐가 붙어있는지 먼저 훑어 출력 ──
    scanBus();

    // ── 1) 표에 적은 주소로 5개 센서 초기화 (실패해도 계속 진행) ──
    gRingOk = initSensor(gRing, Trill::RING, kAddrRing, "Ring");
    gBassOk = initSensor(gBass, Trill::BAR,  kAddrBass, "Bass");
    gR5Ok   = initSensor(gR5,   Trill::BAR,  kAddrR5,   "R5");
    gR8Ok   = initSensor(gR8,   Trill::BAR,  kAddrR8,   "R8");
    gR3Ok   = initSensor(gR3,   Trill::BAR,  kAddrR3,   "R3");

    frame_ = TrillFrame{};

    // ── 2) 요약표 ──
    const int okCnt = int(gRingOk)+int(gBassOk)+int(gR5Ok)+int(gR8Ok)+int(gR3Ok);
    rt_printf("──────────── [Trill 매핑 요약] %d/5 개 활성 ────────────\n", okCnt);
    rt_printf("  Ring(5도권/코드) 0x%02X : %s\n", kAddrRing, gRingOk ? "OK" : "없음");
    rt_printf("  Bass(1음)        0x%02X : %s\n", kAddrBass, gBassOk ? "OK" : "없음");
    rt_printf("  R5  (5음)        0x%02X : %s\n", kAddrR5,   gR5Ok   ? "OK" : "없음");
    rt_printf("  R8  (8음)        0x%02X : %s\n", kAddrR8,   gR8Ok   ? "OK" : "없음");
    rt_printf("  R3  (3음)        0x%02X : %s\n", kAddrR3,   gR3Ok   ? "OK" : "없음");
    if (kIdentifyMode)
        rt_printf("  ※ 식별모드 ON: 바를 만지면 [식별] 로그로 주소↔역할이 찍힙니다.\n");
    rt_printf("──────────────────────────────────────────────────────\n");

    // 하나도 못 붙었으면 실패로 간주(배선/주소 완전히 틀린 경우). 하나라도 있으면 진행.
    return okCnt > 0;
}

void TrillInput::poll() {
    // ── Ring(5도권): 터치 위치를 0..1 로 담는다. 떼면 엔진이 마지막 루트를 래치 ──
    if (gRingOk) {
        gRing.readI2C();
        if (gRing.getNumTouches() > 0) {
            frame_.ringActive = true;
            frame_.ringPos    = clamp01(gRing.touchLocation(0));
            if (kIdentifyMode)
                rt_printf("[식별] 0x%02X = 역할:Ring  pos=%.2f\n", kAddrRing, frame_.ringPos);
        } else {
            frame_.ringActive = false;   // 손 뗌 → 엔진이 마지막 루트 유지(래치)
        }
    } else {
        frame_.ringActive = false;
    }

    // ── Bar 4개 : 물리 배치상 위/아래가 뒤집혔으면 위 표의 kFlip* 를 true 로 ──
    readBar(gBass, gBassOk, frame_.bass, kFlipBass, kAddrBass, "Bass");
    readBar(gR5,   gR5Ok,   frame_.r5,   kFlipR5,   kAddrR5,   "R5");
    readBar(gR8,   gR8Ok,   frame_.r8,   kFlipR8,   kAddrR8,   "R8");
    readBar(gR3,   gR3Ok,   frame_.r3,   kFlipR3,   kAddrR3,   "R3");
}

void TrillInput::cleanup() {
    // 종료 시 특별히 처리할 것 없음.
}

} // namespace hw
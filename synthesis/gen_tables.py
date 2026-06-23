#!/usr/bin/env python3
# gen_tables.py
# ─────────────────────────────────────────────────────────────────────────────
# 녹음 없이, 재즈 보이싱 이론으로 "코드 컴핑 확률 표"를 손으로 채워서
# C++ 헤더(CompTables.h)로 내보낸다.
#
# 표 두 개:
#   kBase[14]      : 각 후보음(근음 기준 반음 오프셋)을 누를 기본 확률
#   kClash[14][14] : 두 후보음을 "같이" 눌러도 되는 정도 (1=괜찮음, 작을수록 충돌)
#
# ※ 이건 "연주자한테 배운 통계"가 아니라 "이론으로 만든 그럴듯한 기본값"이다.
#   나중에 진짜 녹음이 생기면 이 두 표만 데이터로 다시 채우면 코드는 그대로.
# ─────────────────────────────────────────────────────────────────────────────

# 후보음 목록: (근음 기준 반음 오프셋, 이름)
# 0~11은 화음 구성음(이미 buildChord가 놓는 것), 13~ 은 상부 텐션.
CANDIDATES = [
    (0,  "R"),    # 0  근음
    (3,  "m3"),   # 1  단3도
    (4,  "M3"),   # 2  장3도
    (7,  "P5"),   # 3  완전5도
    (9,  "13lo"), # 4  6도(낮은 13)
    (10, "b7"),   # 5  단7도(도미넌트)
    (11, "M7"),   # 6  장7도
    (13, "b9"),   # 7  ♭9   ┐
    (14, "9"),    # 8   9   │
    (15, "#9"),   # 9  ♯9   │ 텐션
    (17, "11"),   # 10 11   │
    (18, "#11"),  # 11 ♯11  │
    (20, "b13"),  # 12 ♭13  │
    (21, "13"),   # 13 13   ┘
]
N = len(CANDIDATES)
name_to_idx = {name: i for i, (_off, name) in enumerate(CANDIDATES)}

# ── 표 1: 기본 확률 (도미넌트 텐션 코드 맥락) ────────────────────────────────
# "도미넌트 위에 텐션을 얹을 때 이 음을 쓸 가능성"으로 손튜닝.
# 화음 구성음은 이미 buildChord가 놓으므로 선택 대상이 아니지만 표엔 채워둔다.
base = {
    "R":   1.00, "m3": 0.00, "M3": 0.90, "P5": 0.50, "13lo": 0.20,
    "b7":  0.95, "M7": 0.05,
    # ── 텐션 ──
    "9":   0.75,   # 가장 무난·기본
    "13":  0.70,   # 색채감 좋은 기본
    "#11": 0.40,   # 리디안 도미넌트 느낌
    "b9":  0.45,   # 변형(해결감)
    "#9":  0.40,   # 변형(블루지)
    "b13": 0.45,   # 변형(어두움)
    "11":  0.08,   # 도미넌트에서 보통 피함(M3와 충돌)
}
kBase = [base[name] for (_off, name) in CANDIDATES]

# ── 표 2: 충돌(clash) 행렬 ───────────────────────────────────────────────────
# 기본 1.0(같이 써도 됨). 충돌하는 쌍만 1보다 작게.
# 이게 네가 말한 "동시에 칠 확률이 낮은 음들" — 이론적으로 명확한 사실들.
clash = [[1.0] * N for _ in range(N)]

def set_clash(a, b, v):
    i, j = name_to_idx[a], name_to_idx[b]
    clash[i][j] = v
    clash[j][i] = v

set_clash("b9",  "9",   0.05)  # ♭9 와 9 는 같이 못 씀
set_clash("9",   "#9",  0.10)  # 9 와 ♯9 충돌
set_clash("b9",  "#9",  0.20)  # ♭9·♯9 둘 다는 드묾
set_clash("13",  "b13", 0.05)  # 13 과 ♭13 충돌
set_clash("b13", "P5",  0.10)  # ♭13(=♯5) 과 완전5도 충돌
set_clash("#11", "P5",  0.35)  # ♯11 과 5도 약한 충돌
set_clash("11",  "M3",  0.05)  # 11(완전4도) 과 장3도 충돌(에비드 노트)
set_clash("11",  "#11", 0.15)  # 11 과 ♯11 충돌
set_clash("M7",  "b7",  0.05)  # 장7 과 단7 은 양립 불가
set_clash("m3",  "M3",  0.02)  # 단3·장3 동시 불가
set_clash("#9",  "M3",  0.65)  # ♯9 과 M3 은 약한 마찰(도미넌트#9는 의도적이라 약하게만)
set_clash("13lo","b7",  0.85)  # 6도와 b7 미세 마찰

# ── C++ 헤더로 내보내기 ──────────────────────────────────────────────────────
def fmt(x): return f"{x:.3f}f"

lines = []
lines.append("// CompTables.h  (자동 생성: gen_tables.py)")
lines.append("// 코드 컴핑 확률 표 — 재즈 이론 기반 기본값. 녹음으로 교체 가능.")
lines.append("#pragma once")
lines.append("namespace syn {")
lines.append("")
lines.append(f"static const int   kCompN = {N};   // 후보음 개수")
lines.append("")
lines.append("// 후보음의 근음 기준 반음 오프셋")
offs = ", ".join(str(off) for (off, _n) in CANDIDATES)
lines.append(f"static const int   kCandOffset[kCompN] = {{ {offs} }};")
lines.append("")
lines.append("// 각 후보음의 기본 확률")
basevals = ", ".join(fmt(v) for v in kBase)
lines.append(f"static const float kBase[kCompN] = {{ {basevals} }};")
lines.append("")
lines.append("// 충돌 행렬 (1=같이 OK, 작을수록 충돌)")
lines.append("static const float kClash[kCompN][kCompN] = {")
for i in range(N):
    row = ", ".join(fmt(clash[i][j]) for j in range(N))
    nm = CANDIDATES[i][1]
    lines.append(f"  {{ {row} }},  // {nm}")
lines.append("};")
lines.append("")
lines.append("} // namespace syn")

with open("/home/claude/work/CompTables.h", "w") as f:
    f.write("\n".join(lines) + "\n")

print("CompTables.h 생성 완료")
print(f"후보음 {N}개, 충돌쌍 설정 완료")

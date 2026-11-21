# Data/ 작업 규칙

콘텐츠는 데이터다. AI는 후보를 많이 만들고, 거르는 일은 검사기와 사람이 한다. 루트 `AGENTS.md`를 먼저 읽는다.

| 폴더 | 내용 | AI가 써도 되는가 |
|---|---|---|
| `defs/units.csv`, `defs/rules.csv` | 병과 수치, 규칙 매개변수 | 사람이 정한 값을 옮겨 적을 때만 |
| `candidates/roundN/` | AI가 낸 후보 | 된다. 후보는 여기에만 쓴다 |
| `accepted/squads.csv`, `relics.csv`, `formations.csv` | 채택본 | 사람이 후보 id를 짚어 채택을 지시했을 때만 |
| `reports/` | 관문 기록, BalanceSim CSV | 도구가 쓴다. 손으로 고치지 않는다 |

## 세 관문

| 관문 | 검사하는 쪽 | 검사 | 통과 기준 [초기값] |
|---|---|---|---|
| 1 스키마 | `node tools/validate_content.js` | 필수 필드, 어휘 밖의 값, 값 범위, 칸 연결성, 회전 중복, 이름 8자 | 하나라도 어기면 탈락 |
| 2 기여도 | BalanceSim, 후보당 1만 판 | 후보를 넣은 덱과 뺀 덱의 승률 차이 | +2~+8%p |
| 2 독주 | 같음 | 후보가 들어간 두 장 조합의 승률 | 어떤 조합도 70% 이하 |
| 2 중복 | 같음 | 기존 카드와 효과 벡터의 코사인 유사도 | 0.9 미만 |
| 2 해골 진형 | 같음 | 배정된 밤에서 탐색 봇의 승률 | 목표 곡선 ±5%p |
| 2 다양성 [기록만] | 같음 | 한 배치가 여러 밤을 이기는 비율 | 통과 기준이 아니다. 기록하고, 기준으로 쓸지는 측정 뒤 사람이 정한다(2026-11-21 결정 3 ⑪) |
| 2 병과 검사 [제안] | BalanceSim `class` 모드 | 단일 병과 덱의 승률이 혼합 덱보다 낮은지, 병과 1명당 가치의 편차 | 기준값은 사람이 정한다. 정하기 전에는 리포트만 낸다 |
| 3 사람 | 직접 플레이 | 설명만 읽고 효과를 예측해 실제와 비교, 고를 이유가 있는지 | 예측이 틀리면 설명이 아니라 효과를 고친다 |

목표 곡선(밤 1~12의 탐색 봇 승률): 90, 86, 82, 70, 78, 74, 70, 58, 66, 60, 54, 45%. (기획서 §3. 11/21 결정 3 ⑪의 "사이 밤은 직선 보간"은 옮겨 적다 생긴 오류라 이 곡선을 쓴다, 2026-11-22 결정)

후보를 생성하기 전에 끝나 있어야 하는 것. 하나라도 없으면 생성을 시작하지 않고 알린다.
- 병과 수치 확정(`defs/units.csv`). 기획 단계 점검에서 전원 척후 덱이 혼합 덱보다 잘 이겼다 [산정]. 기반이 어긋난 채 걸러 내면 수치를 고친 뒤 전부 다시 돌려야 한다.
- 시작 덱과 밤별 기대 덱 크기. 탐색 봇과 목표 곡선이 여기에 달려 있다. [초기값]은 11/21 결정 3 ⑤로 정했고(`Source/RankGame/AGENTS.md`), 측정 뒤 다시 정한다.
- `RankCore` 기준 확정과 BalanceSim 동작.

- 관문 1·2를 통과해도 채택이 아니다. 무엇을 넣을지는 사람이 플레이해 보고 정한다.
- 통과 기준은 사람이 정한다. 통과율이 낮다고 기준을 넓히거나 검사를 빼지 않는다.
- 탈락한 후보를 지우지 않는다. 회차 폴더에 그대로 두어야 통과율의 근거가 남는다.
- 탈락 사유는 JSON으로 모아 다음 회차의 생성 요청에 붙인다.
- 회차마다 `reports/gates.csv`에 한 줄을 더한다: 회차, 종류, 후보 수, 관문 1 통과, 관문 2 통과, 채택, 가장 많은 탈락 사유.

## 후보를 만들 때

- 정해진 어휘만 조합한다. 새 효과가 필요해 보이면 만들지 않고 제안으로 남긴다. 어휘 추가는 사람이 하고 테스트를 붙인다.
- 카드 설명문을 쓰지 않는다. 설명은 조각에서 템플릿으로 만든다. `desc` 같은 필드를 더하지 않는다.
- 이름은 8자 이하다. 기존 게임, 작품, 인물의 이름을 쓰지 않는다.
- 한 회차의 분량은 분대 60, 유물 30이다. 출시 목표는 분대 16, 유물 6, 해골 진형 24(밤마다 2개)다(2026-11-21 범위 축소, 결정 3 ⑥). 해골 진형은 AI가 만들지 않고 생성기가 만든다(아래 "해골 진형").
- 생성 요청에는 스키마, 설계 기둥, 기존 카드 요약, 지난 회차의 탈락 사유를 함께 넣는다.

## 형식

콘텐츠와 수치 파일은 모두 CSV다(2026-11-18 결정). 식별자 문자열의 기준은 `tools/validate_content.js`의 목록이다. 아래 예시에 없는 문자열을 지어내지 않는다.

CSV 공통 규칙
- 인코딩은 UTF-8이다. 엑셀에서는 "CSV UTF-8(쉼표로 분리)"로 저장한다. 파일 앞의 BOM은 있어도 된다. UTF-8이 아닌 파일은 관문 1에서 탈락한다.
- 첫 줄은 열 이름이고, 한 줄에 항목 하나를 적는다. 열 이름과 순서는 아래 예시를 따른다.
- 구분자는 쉼표다. 값에 쉼표나 큰따옴표가 들어가면 값 전체를 큰따옴표로 감싸고, 안의 큰따옴표는 두 번 쓴다.
- 줄바꿈은 CRLF와 LF를 모두 받는다.
- 빈 칸은 "값 없음"이다. 참과 거짓은 `TRUE`, `FALSE`로 쓴다.
- 콘텐츠 CSV(`accepted/`)의 첫 열 `id`가 DataTable의 행 이름이다. `defs`의 두 파일은 CSV의 줄 번호(1부터)가 행 이름이고 병과 키는 열이다(행 이름은 대소문자를 가리지 않아 `M`·`m`, `R`·`r`을 쓸 수 없다, 2026-11-22 결정). 엔진 CSV 임포트는 쓰지 않는다(2026-11-21 결정 4 ④, 방법은 `Source/RankEditor/AGENTS.md`). `id`는 대소문자만 다른 것도 겹침으로 보고 관문 1에서 탈락시킨다.
- 테스트 기록(`Source/RankTests/Fixtures/*.json`)과 검사기가 내는 탈락 사유는 콘텐츠가 아니므로 JSON으로 둔다.

분대 (`accepted/squads.csv`)

```csv
id,name,class,shape,tier,traitWhen,traitDo,traitWho,traitN,traitLimit
sq_K_L3_02,성벽 방패대,K,0:0;1:0;1:1,2,OnHit,Shield,Linked,2,3
```

- `class`: `K` 방패병, `B` 광전사, `R` 석궁수, `M` 마법사, `H` 척후.
- `shape`: 칸 1~4개를 `행:열`로 적고 `;`로 잇는다. 칸은 상하좌우로 이어져야 한다. 같은 모양을 돌린 것은 중복이다. 거울상(예: L과 J)은 다른 모양이다. 분대는 뒤집지 않는다(11/18, `docs/rules.md` §2.1).
- `trait*`: 특성 0~1개. 없으면 다섯 칸을 모두 비운다. 뜻은 `docs/rules.md` §10이다.
  - `traitWhen`: `BattleStart`(전투 시작), `OnAttack`(공격할 때), `OnHit`(맞을 때), `OnKill`(처치할 때), `OnAllyDeath`(아군이 쓰러질 때)
  - `traitDo`: `AtkUp`(공격+), `ArmorUp`(방어+), `Heal`(회복), `Shield`(보호막), `IntervalDown`(간격−), `GateRepair`(성문 수리)
  - `traitWho`: `Self`(자신), `Linked`(링크된 이웃), `Column`(같은 열), `Squad`(같은 분대), `Allies`(아군 전체)
  - `traitN`은 수치(1 이상의 정수), `traitLimit`은 전투당 발동 횟수(1 이상)다. `traitWhen`이 `BattleStart`면 `traitLimit`을 비운다. `traitDo`가 `GateRepair`면 `traitWho`는 `Self`다. 어기면 관문 1에서 탈락한다.

해골 진형 (`accepted/formations.csv`)

```csv
id,night,kind,row0,row1,row2,row3
fm_n07_a,7,normal,wwmmww,mrmmrm,.g..g.,..rr..
```

- `row0`~`row3`: 행마다 6자. `row0`이 행 0(최전열)이다. `m` 졸개, `w` 전사, `r` 궁수, `g` 술사, `.` 빈칸.
- `night`: 1~12. 밤마다 2개를 둔다(2026-11-21 범위 축소). 4·8번째 밤은 정예, 12번째 밤은 우두머리다.
- 해골 진형은 `RankEditor`의 생성기 커맨들릿이 밤별 예산으로 만들고, 관문 2를 통과한 것 가운데 사람이 고른다(2026-11-20, 11/21 결정 3 ⑨).
- 밤별 해골 수 예산 [초기값]: 밤 1 4명에서 밤 12 14명까지 늘리고, 정예 밤은 +2명, 우두머리 밤은 +3명이다. 초반은 졸개·전사 위주이고 뒤로 갈수록 궁수·술사 비율을 늘린다. 측정으로 조정한다(11/21 결정 3 ⑩).

유물 (`accepted/relics.csv`)

```csv
id,name,effectKind,effectParam,effectDelta,effectTarget
rl_iron_gate,철문,Rule,GateHp,10,
rl_war_drum,전쟁 북,Start,AtkUp,1,B
```

- 뜻은 `docs/rules.md` §10.7이다. 유물은 진영 0에만 쓴다.
- `effectKind`: `Rule`(규칙 매개변수를 바꾼다) 또는 `Start`(전투 시작 효과).
- `Rule`: `effectParam`은 `GateHp`, `LinkStackMax`, `RangeMax`, `IntervalMin` 가운데 하나, `effectDelta`는 바꾸는 양(정수), `effectTarget`은 비운다. `MaxTicks`와 `GateDamage`는 쓰지 않는다(2026-11-21 결정 3 ⑦, §10.7).
- `Start`: `effectParam`은 `AtkUp`, `ArmorUp`, `Heal`, `Shield`, `IntervalDown` 가운데 하나(`traitDo`와 같은 식별자, `GateRepair`는 쓰지 않는다), `effectDelta`는 N(1 이상의 정수), `effectTarget`은 `All` 또는 병과 키 하나(`K`, `B`, `R`, `M`, `H`).

후보(`candidates/roundN/`)도 같은 열을 쓴다.

id는 분대 `sq_{병과}_{모양}_{번호}`, 진형 `fm_n{밤 두 자리}_{a~b}`, 유물 `rl_{영문 소문자와 밑줄}`이다. 채택된 id는 바꾸지 않는다. 리포트와 결정 기록이 id로 가리킨다.

## defs

- `units.csv`: 열은 `key,name,hp,atk,armor,interval,range,target,magic,revive,linkStat,linkAmount`. 병과 9종(`K`, `B`, `R`, `M`, `H`, `m`, `w`, `r`, `g`)의 체력, 공격, 방어, 간격, 사거리, 표적, 마법 여부, 부활 값, 링크 효과.
- `rules.csv`: 열은 `maxTicks,gateHp,gateDamage,linkStackMax,rangeMax,intervalMin`이고 데이터는 한 줄이다. 최대 600틱, 성문과 결계 60, 성문 피해 1, 링크 겹 상한 3, 사거리 상한 4, 간격 하한 4.
- `target`은 `Front`(맨 앞), `Back`(맨 뒤), `Support`(지원), `Area`(범위). `linkStat`은 `None`, `Armor`, `Atk`, `Range`, `Hp`, `Interval`이고, `linkAmount`는 한 겹의 크기다(간격은 −1처럼 음수).
- 지금 값은 [초기값]이다. 조정안은 낼 수 있지만 값은 사람이 정한다.
- 값을 바꾸면 순서대로 한다: 영향을 받는 손계산 시나리오의 기대값을 AI가 다시 계산하고 교차 검증을 다시 한다(다른 곳은 사람이 판정) → `tools\run_tests.bat`으로 회귀 테스트에서 결과가 바뀐 대전을 확인해 사람에게 보인다 → 승인 뒤 `tools\record_regression.bat` → BalanceSim으로 문서의 수치를 다시 잰다 → `docs/decisions.md`에 전후 값과 근거를 적는다.

## reports

- CSV는 도구가 낸 그대로 둔다. 숫자를 손으로 고치지 않는다. 요약과 해석은 따로 적는다.
- 리포트마다 조건을 남긴다: 명령, 시드, 판 수, 콘텐츠 버전 해시, 커밋.
- 같은 명령과 같은 시드는 같은 CSV를 내야 한다. 다르면 버그로 알린다.
- 콘텐츠 버전 해시는 채택본 CSV 3개와 `defs` CSV 2개의 내용을, BOM을 빼고 줄바꿈을 LF로 맞춘 뒤 FNV-1a로 묶은 값이다. `accepted/`나 `defs/`가 바뀌면 해시가 바뀐다. 예전 해시로 잰 수치를 지금 콘텐츠의 수치처럼 쓰지 않는다.

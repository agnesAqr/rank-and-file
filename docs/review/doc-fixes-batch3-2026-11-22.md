# 결정 6 묶음 3: 하위 규칙 파일 수정안 [제안]

작성: 2026-11-22. 상태: 같은 날 "모두 권장안"으로 승인되어 반영했다(`docs/decisions.md` 11/22 묶음 3 행). 이 파일은 그 기록이다.

근거는 11/21~22의 결정 행(`docs/decisions.md`), `docs/review/unconfirmed-check-2026-11-21.md`(엔진 소스 확인), `docs/review/t11-shipping-investigation-2026-11-21.md` 7.1절, 11/21 일지의 에이전트 측정이다. 배경 사양(`Content/AGENTS.md` "배경 이미지" 절, `docs/background.md`)은 11/21 "배경 관련 작업은 모두 보류" 결정에 따라 바꾸지 않고 보류 표시 한 줄만 단다.

## 0. 고를 것

| # | 질문 | 권장안 | 다른 안 | 권장 이유 [판단] |
|---|---|---|---|---|
| 가 | `Data/defs`(병과 수치, 규칙 매개변수)를 DataTable에 넣을 때 행 이름 | CSV의 줄 번호(1부터)를 행 이름으로 하고, 병과 키는 열로 둔다 | ② defs는 DataTable 대신 데이터 에셋 하나(배열)에 넣는다 ③ 키를 행 이름으로 쓰되 소문자 키에 표시를 붙인다(예: `m` → `m_lower`) | 행 이름(`FName`)은 대소문자를 가리지 않아 `M`(마법사)과 `m`(해골 졸개), `R`과 `r`이 같은 행이 된다(`NameTypes.h:628`, 11/21 조사). 코어는 병과를 행 이름이 아니라 키로 찾으므로 행 이름에 뜻이 없어도 된다. 결정 4 ④(DataTable에 `AddRow`)를 그대로 지킨다 |
| 나 | 관문 2의 목표 승률 곡선 | 기획서의 12밤 곡선을 그대로 쓴다: 90, 86, 82, 70, 78, 74, 70, 58, 66, 60, 54, 45%(정예 밤 4·8에서 내려간다) | 결정 3 ⑪ 문장대로 밤 3(82%)·7(70%)·12(45%) 사이를 직선으로 잇는다 | `Data/AGENTS.md` 24줄과 `docs/design.md` 671줄(기획서 §3)이 12밤 곡선이다. 결정 3 ⑪의 "사이 밤은 직선 보간"은 AI가 선택지를 쓰며 옮겨 적다 생긴 오류다. 직선이면 정예 밤이 다른 밤보다 쉬워진다 |
| 다 | 특성·유물 구현 시점 | 기준 확정 뒤에 구현한다 | 지금 다른 일과 병렬로 구현한다 | 구현은 `RankCore`를 바꾼다. 본인이 지금 리뷰하는 코드가 리뷰 중에 바뀌면 기준 확정의 리뷰 범위가 흐려진다. 반드시 순서를 지켜야 하는 일이다(11/21 결정 3 ⑧) |

아래 문장은 세 질문 모두 권장안을 넣은 모양이다.

## 1. 루트 `AGENTS.md` 5절 (11/21 에이전트 측정 반영)

지금:
```
  - 함께 켜지는 것 넷: MCP가 의존하는 `ToolsetRegistry`, `PythonScriptPlugin`, `EditorScriptingUtilities`, `FileSandbox`. 이 플러그인들에 기대는 게임 코드는 넣지 않는다.
  - MCP 사용 통계는 끈다(콘솔 변수 `ModelContextProtocol.EnableAnalytics`).
```
바꾼 뒤:
```
  - MCP가 의존하는 넷: `ToolsetRegistry`(MCP를 켜면 새로 켜진다), `PythonScriptPlugin`, `EditorScriptingUtilities`, `FileSandbox`(셋은 엔진 기본 플러그인이 이미 켜고 있다, 2026-11-21 측정). 이 플러그인들에 기대는 게임 코드는 넣지 않는다.
  - MCP 사용 통계는 끈다(`Config/DefaultEditor.ini`의 `[ConsoleVariables]`에 `ModelContextProtocol.EnableAnalytics=0`. 에디터에서만 적용되고 패키지에 들어가지 않는다).
```

## 2. `Source/AGENTS.md`

(1) "UI" 행의 끝 문장. 지금: `[미정] 처음부터 디자이너로 배치할지, 코드로 배치하고 조정값만 에디터에 열어 둘지는 사람이 정한다` → 바꾼 뒤: `배치는 처음부터 위젯 블루프린트 디자이너에서 한다. 위젯 블루프린트는 AI가 MCP(UMG ToolSet)로 만들고, 사람이 화면 캡처를 보고 받아들인다(2026-11-21 결정 3 ④)`

(2) "애니메이션 재생" 행. 지금: `블렌딩 같은 세부 조정이 필요하면 C++ UAnimInstance 하위 클래스를 부모로 하는 애니메이션 블루프린트를 만든다.` → 바꾼 뒤: `블렌딩 조정은 시간이 남으면 하는 항목이고(2026-11-21 범위 축소), 그때 사람이 에디터에서 C++ UAnimInstance 하위 클래스를 부모로 하는 애니메이션 블루프린트를 만든다(MCP로는 만들 수 없다, 11/21 AI 기능 결정 ⑤).`

(3) 50줄 `BindWidget`의 [미확인]. 바꾼 뒤:
```
- BindWidget은 5.8.4에서 이렇게 처리된다(엔진 소스로 확인, 2026-11-21). 위젯 블루프린트를 컴파일할 때 에디터가 부모 C++ 클래스의 meta=(BindWidget) 속성마다 이름과 형이 같은 위젯을 찾고, 없거나 형이 다르면 컴파일 오류를 낸다(처음 만든 블루프린트는 경고, Engine/Source/Editor/UMGEditor/Private/WidgetBlueprintCompiler.cpp:1257-1305). meta=(BindWidgetOptional)은 없어도 안내만 낸다. 실행 중에는 메타데이터와 상관없이 위젯과 이름이 같은 객체 속성에 위젯을 넣는다(Engine/Source/Runtime/UMG/Private/WidgetBlueprintGeneratedClass.cpp:270-277). 그래서 C++ 위젯 클래스의 위젯 아닌 객체 속성 이름을 디자이너의 위젯 이름과 겹치게 짓지 않는다.
```

(4) 52줄 Visual Studio의 [미확인]. 바꾼 뒤:
```
- 빌드 도구는 Visual Studio 2022 17.14 이상 또는 Visual Studio 2026이다(5.8 릴리스 노트). 이 PC에서 UnrealBuildTool이 쓰는 툴체인(toolchain)은 Visual Studio 2022 17.14의 MSVC 14.44.35207(컴파일러 14.44.35228)이다 [측정: 2026-11-19~21의 에디터·Shipping 빌드 기록]. 엔진의 선호 목록(Engine/Config/Windows/Windows_SDK.json의 PreferredVisualCppVersions)이 14.50(VS 2026 18.0)과 14.44(VS 2022 17.14)뿐이라, 같은 PC의 VS 2026 18.11(MSVC 14.51)은 고르지 않는다. 값과 조건은 README.md "빌드 도구"에 있다. 툴체인이 바뀌면 그 표를 고친다.
```

## 3. `Source/RankCore/AGENTS.md`

(1) "구현의 기준" 표의 "링크" 행 앞부분. 지금: `상하좌우 이웃의 병과가 다르고 그 병과에 링크 효과가 있을 때 한 겹. 진영은 보지 않는다.` → 바꾼 뒤: `같은 진형판 안에서 상하좌우 이웃의 병과가 다르고 그 병과에 링크 효과가 있을 때 한 겹. 두 진형판이 맞닿는 선은 넘지 않는다(§9 19번). 진영은 보지 않는다.` (뒷부분은 그대로)

(2) "특성과 유물" 절 전체. 바꾼 뒤:
```
처리 시점과 조각 수는 `docs/rules.md` §10에 정해졌다(2026-11-22, 조각 16개). 구현은 §10대로 하고, §10과 다르게 짜야 할 곳이 보이면 멈추고 알린다.
- 구현은 기준 확정 뒤에 한다. 기준 확정 전에 이 모듈을 바꾸면 리뷰 범위가 흐려진다(2026-11-22 결정).
- 구현하기 전에 코어 입력에 분대 번호, 특성, 유물을 더하는 공개 형태 변경을 알린다(`docs/design.md` 6.1).
- 조각마다 AI가 §10으로 계산하고 교차 검증한 예제를 2개 이상 테스트로 둔다(T7).
- 특성이 들어간 덱으로 결정성과 미러 테스트를 다시 돌린다. 유물은 진영 0에만 쓰므로 미러 테스트에 넣지 않는다(§10.7).
- 특성과 유물이 없는 전투의 결과와 해시가 바뀌지 않는지 회귀 테스트로 본다. 동작이 바뀌는 것이므로 회귀 테스트 결과 다시 기록도 승인을 받는다.
```

## 4. `Source/RankGame/AGENTS.md`

(1) 45줄 [미정]. 바꾼 뒤:
```
- 위젯 배치는 처음부터 위젯 블루프린트 디자이너에서 한다(C++ 위젯 클래스를 부모로, AI가 MCP로 만들고 사람이 캡처를 보고 받아들인다). 배치 입력은 3D 전장 위 클릭이다. 배치 화면에서 그 밤의 해골 진형을 모두 보여 준다(2026-11-21 결정 3 ②③④).
```
(2) 46줄 [미정]. 바꾼 뒤:
```
- 시작 덱과 밤별 기대 덱은 [초기값]이다: 분대 3개(병사 5~6명), 밤마다 보상 3장 가운데 1장, 분대와 유물 2:1, 기대 덱 밤 3 약 6명·밤 7 약 10명·밤 12 약 12명. BalanceSim으로 잰 뒤 다시 정한다(2026-11-21 결정 3 ⑤). 값은 데이터에 두고 코드에 적지 않는다. 저장과 이어하기는 만들지 않는다(11/21 범위 축소).
```
(3) 61줄(11/21 함수 모으기 반영, 에이전트 제안). 바꾼 뒤:
```
- 입력 CSV를 코어 입력으로 바꾸는 함수(`RankRegression::ParseInputsCsv`, `RankRegression::MakeBattleInput`)와 읽는 파일의 경로(`RankRegressionData.h`)는 이 모듈에 하나만 두고, RankRegression 커맨들릿과 T1도 같은 것을 쓴다. 읽는 코드가 둘이면 T11이 어긋났을 때 원인이 코어인지 읽는 코드인지 가를 수 없다.
```

## 5. `Source/RankEditor/AGENTS.md`

(1) 3줄 첫 문장. 지금: `에디터에서만 도는 도구 셋이 있다: 콘텐츠 임포터, BalanceSim 커맨들릿(commandlet), 회귀 테스트 결과를 기록하는 RankRegression 커맨들릿.` → 바꾼 뒤: `에디터에서만 도는 도구 넷이 있다: 콘텐츠 임포터, BalanceSim 커맨들릿(commandlet), 회귀 테스트 결과를 기록하는 RankRegression 커맨들릿, 해골 진형 생성기 커맨들릿(2026-11-21 결정 3 ⑨).`

(2) 13줄 [미확인]. 바꾼 뒤:
```
- CSV는 RankGame의 CSV 읽기 함수로 읽고, 읽은 행 구조체를 UDataTable::AddRow로 넣는다. 엔진 CSV 임포트(CreateTableFromCSVString)는 쓰지 않는다(2026-11-21 결정 4 ④). 5.8.4의 AddRow는 에디터 전용이 아니고, 문제 메시지를 돌려주지 않고, 같은 행 이름이 있으면 경고 없이 덮어쓰고, 한 형태(AddRow(이름, 행))는 넘긴 행의 형을 RowStruct와 맞춰 보지 않는다(엔진 소스로 확인, Engine/Source/Runtime/Engine/Private/DataTable.cpp:519-560). 그래서 임포터는 RowStruct를 먼저 정하고, 형을 받는 형태(AddRow(이름, 데이터, 구조체))로 넣고, 넣기 전에 행 이름 겹침(대소문자 무시)을 검사한다. 읽기 함수의 오류나 겹침이 하나라도 있으면 에셋을 저장하지 않고 실패로 끝낸다. 저장은 UPackage::SavePackage로 한다. 행 구조체의 필드는 모두 UPROPERTY여야 에셋에 남는다.
- 행 이름: 콘텐츠 CSV는 첫 열 id다. defs의 두 파일(units.csv, rules.csv)은 CSV의 줄 번호(1부터)이고, 병과 키는 열로 둔다. 행 이름(FName)이 대소문자를 가리지 않아 병과 키 M과 m, R과 r을 행 이름으로 쓸 수 없다(Engine/Source/Runtime/Core/Public/UObject/NameTypes.h:628, 2026-11-22 결정).
```

(3) "BalanceSim" 절 앞에 새 절:
```
## 해골 진형 생성기

- 밤별 예산(`Data/AGENTS.md` "해골 진형")에 맞춰 해골 진형 후보를 만든다. 난수는 `RankCore`의 정수 난수기(`RankRandom`)만 쓰고, 같은 시드면 같은 후보가 나온다.
- 후보는 `Data/candidates/roundN/formations.csv`에 쓴다. 관문 1과 관문 2(BalanceSim `formation` 모드)를 거쳐 사람이 채택한다.
- 전투 결과는 계산하지 않는다. 거르는 것은 BalanceSim이다.
```

## 6. `Content/AGENTS.md`

(1) 8줄. 지금: `MCP로 하는 일: 위젯 블루프린트와 애니메이션 블루프린트 생성·설정(부모는 C++ 클래스, 이벤트 그래프는 비움), 캐릭터 임포트, 데이터 에셋 연결, 전장 액터·머티리얼 배치.` → 바꾼 뒤 세 줄:
```
- MCP로 하는 일: 위젯 블루프린트 생성·설정(UMG ToolSet, 부모는 C++ 클래스, 이벤트 그래프는 비움), 데이터 에셋 연결과 속성 설정(Editor 도구 묶음), 전장 액터·머티리얼 배치.
- AI가 하지 않고 사람이 에디터에서 하는 일: 캐릭터 임포트(임포트 창에서 Skeleton을 지정한다. MCP 임포트로는 Skeleton을 지정할 수 없다), 애니메이션 블루프린트(MCP로 만들 수 없다. 블렌딩은 시간이 남으면 하는 항목)(2026-11-21 AI 기능 결정 ④⑤).
- MCP 서버는 자동으로 뜨지 않는다. 쓸 때 에디터를 실행 인자 -ModelContextProtocolStartServer로 띄운다(127.0.0.1:8000, 2026-11-21 측정). MCP 클라이언트 설정 파일 .mcp.json은 커밋하지 않는다(11/21 결정 4 ③).
```
(2) 20줄 "Characters/" 행 끝. 지금: `블렌딩 조정이 필요해지면 C++ 부모를 둔 애니메이션 블루프린트(ABP_) 하나를 9종이 함께 쓴다` → 바꾼 뒤: `블렌딩 조정은 시간이 남으면 사람이 C++ 부모를 둔 애니메이션 블루프린트(ABP_) 하나를 만들어 9종이 함께 쓴다`

(3) 30줄 끝에 더함: `임포트는 사람이 에디터 임포트 창에서 한다(2026-11-21).`

(4) 32줄. 지금: `몸만 가져온다. 무기는 개별 파일을 handslot.l, handslot.r 본에 붙인다.` → 바꾼 뒤:
```
- 모험가 5종은 GLB에 붙은 투구·망토·무기·방패 메시를 그대로 쓰고, 여러 개인 방패·검은 하나만 남기고 숨긴다(2026-11-21 결정 4 ①). [미확인] 숨기는 방법(메시 구역 숨기기 등)은 임포트 결과를 보고 정한다.
- 해골 4종의 GLB에는 무기 메시가 없다(2026-11-21 측정). [미정] 해골 무기를 같은 팩의 별도 파일(Assets/gltf/)로 handslot 본에 붙일지, 무기 없이 둘지(결정 6 묶음 4에서 정한다).
```
(5) "배경 이미지 (AI 생성)" 절 제목 아래에 더함: `[보류] 2026-11-21부터 배경 작업은 보류다(본인이 3D 생성 AI로 바꾸는 중). 아래 2D 사양과 기준은 방법이 정해질 때까지 그대로 둔다.`

## 7. `Data/AGENTS.md`

(1) "세 관문" 표에 행 추가(관문 2 해골 진형 행 아래):
```
| 2 다양성 [기록만] | 같음 | 한 배치가 여러 밤을 이기는 비율 | 통과 기준이 아니다. 기록하고, 기준으로 쓸지는 측정 뒤 사람이 정한다(2026-11-21 결정 3 ⑪) |
```
(2) 24줄 목표 곡선 끝에 더함: `(기획서 §3. 11/21 결정 3 ⑪의 "사이 밤은 직선 보간"은 옮겨 적다 생긴 오류라 이 곡선을 쓴다, 2026-11-22 결정)`

(3) 28줄. 지금: `- 시작 덱과 밤별 기대 덱 크기. 탐색 봇과 목표 곡선이 여기에 달려 있다.` → 바꾼 뒤: `- 시작 덱과 밤별 기대 덱 크기. 탐색 봇과 목표 곡선이 여기에 달려 있다. [초기값]은 11/21 결정 3 ⑤로 정했고(Source/RankGame/AGENTS.md), 측정 뒤 다시 정한다.`

(4) 42줄. 지금: `- 한 회차의 분량은 분대 60, 유물 30, 해골 진형 90이다. 출시 목표는 24, 12, 36이다.` → 바꾼 뒤: `- 한 회차의 분량은 분대 60, 유물 30이다. 출시 목표는 분대 16, 유물 6, 해골 진형 24(밤마다 2개)다(2026-11-21 범위 축소, 결정 3 ⑥). 해골 진형은 AI가 만들지 않고 생성기가 만든다(아래 "해골 진형").`

(5) 55줄. 바꾼 뒤:
```
- 콘텐츠 CSV(accepted/)의 첫 열 id가 DataTable의 행 이름이다. defs의 두 파일은 CSV의 줄 번호(1부터)가 행 이름이고 병과 키는 열이다(행 이름은 대소문자를 가리지 않아 M·m, R·r을 쓸 수 없다, 2026-11-22 결정). 엔진 CSV 임포트는 쓰지 않는다(2026-11-21 결정 4 ④, 방법은 Source/RankEditor/AGENTS.md). id는 대소문자만 다른 것도 겹침으로 보고 관문 1에서 탈락시킨다.
```
(6) 67줄 `trait*`. 바꾼 뒤:
```
- `trait*`: 특성 0~1개. 없으면 다섯 칸을 모두 비운다. 뜻은 `docs/rules.md` §10이다.
  - `traitWhen`: `BattleStart`(전투 시작), `OnAttack`(공격할 때), `OnHit`(맞을 때), `OnKill`(처치할 때), `OnAllyDeath`(아군이 쓰러질 때)
  - `traitDo`: `AtkUp`(공격+), `ArmorUp`(방어+), `Heal`(회복), `Shield`(보호막), `IntervalDown`(간격−), `GateRepair`(성문 수리)
  - `traitWho`: `Self`(자신), `Linked`(링크된 이웃), `Column`(같은 열), `Squad`(같은 분대), `Allies`(아군 전체)
  - `traitN`은 수치(1 이상의 정수), `traitLimit`은 전투당 발동 횟수(1 이상)다. `traitWhen`이 `BattleStart`면 `traitLimit`을 비운다. `traitDo`가 `GateRepair`면 `traitWho`는 `Self`다. 어기면 관문 1에서 탈락한다.
```
(7) 77줄. 지금: `- night: 1~12. 밤마다 3개를 둔다. 4·8번째 밤은 정예, 12번째 밤은 우두머리다.` → 바꾼 뒤:
```
- `night`: 1~12. 밤마다 2개를 둔다(2026-11-21 범위 축소). 4·8번째 밤은 정예, 12번째 밤은 우두머리다.
- 해골 진형은 `RankEditor`의 생성기 커맨들릿이 밤별 예산으로 만들고, 관문 2를 통과한 것 가운데 사람이 고른다(2026-11-20, 11/21 결정 3 ⑨).
- 밤별 해골 수 예산 [초기값]: 밤 1 4명에서 밤 12 14명까지 늘리고, 정예 밤은 +2명, 우두머리 밤은 +3명이다. 초반은 졸개·전사 위주이고 뒤로 갈수록 궁수·술사 비율을 늘린다. 측정으로 조정한다(11/21 결정 3 ⑩).
```
(8) 79~86줄 유물. 바꾼 뒤:
````
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
````
(9) 90줄 id 규칙. 지금: `fm_n{밤 두 자리}_{a~c}` → 바꾼 뒤: `fm_n{밤 두 자리}_{a~b}`

## 8. `tools/AGENTS.md`

(1) 42줄 [미확인]을 t11 조사 문서 7.1절의 여섯 줄로 바꾸고, 셋째·넷째·다섯째 줄 끝에 11/21 확인을 붙인다:
```
- 엔진 소스로 확인한 것(UE 5.8.4, 근거와 파일:줄은 `docs/review/t11-shipping-investigation-2026-11-21.md`):
  - 실행 인자: Shipping에서도 `FCommandLine::Get()`, `FParse::Param`, `FParse::Value`가 그대로 동작한다. 인자를 거르는 허용 목록(`UE_COMMAND_LINE_USES_ALLOW_LIST`)은 기본값이 0이고 이 프로젝트는 켜지 않았다. 실행 파일은 시작할 때 작업 디렉터리를 실행 파일 폴더로 바꾸므로 경로 인자는 절대 경로로 넘긴다.
  - 결과 파일 위치: Shipping의 `FPaths::ProjectSavedDir()`는 사용자 폴더(로컬 AppData) 아래라 쓰지 않는다. 스크립트가 저장소 `Saved/T11/` 안의 절대 경로를 인자로 넘기고, 실행 파일은 그 경로에 쓴다. 입력 CSV와 `Data/defs`도 패키지에 넣지 않고 저장소의 파일을 절대 경로로 읽는다. 11/20 Shipping 패키지를 실행한 뒤 사용자 폴더의 `Saved` 아래에 설정 파일이 생긴 것으로도 확인했다.
  - 로그: Shipping은 `NO_LOGGING`이라 `UE_LOG`, `UE_LOGFMT`가 Fatal 말고는 컴파일되지 않는다. 켜는 설정(`bUseLoggingInShipping`)은 설치형 엔진에서 쓸 수 없다. 판정은 결과 파일(상태, 오류 문장, 빌드 구성, 수치, 대전 20건)과 종료 코드로만 한다. 같은 실행 뒤 그 폴더의 `Logs`는 비어 있었다.
  - 종료 코드: `FPlatformMisc::RequestExitWithStatus(true, 코드)`가 Windows에서 그 코드로 프로세스를 끝낸다. `false`로는 종료 코드가 바뀌지 않는다. 엔진은 크래시하면 3으로, 정상 종료면 0으로 끝내므로 종료 코드 0만으로 통과로 보지 않는다. 인자가 무시되면 게임이 그대로 뜨므로 시간 제한을 둔다. 에디터 테스트 실행(2026-11-21)에서 `RequestExitWithStatus(true, -1)`이 종료 코드 255로 나온 것으로 Windows의 동작을 확인했다(종료 코드는 `uint8`이다. Shipping에서는 아직 시험 전).
- [미확인] 위 방식을 실제 Shipping 패키지에서 돌려 본 결과: `-nullrhi`로 모듈 시작까지 가는지, 모듈 시작 시점에서 돌고 강제 종료했을 때 결과 파일과 종료 코드가 남는지, 스크립트가 종료 코드를 받는지, 스테이징 루트의 `RankAndFile.exe`(부트스트랩)가 인자와 종료 코드를 넘기는지. T11 코드를 넣은 첫 패키지에서 확인하고, 확인되면 이 표시를 지운다. 확인하기 전에는 `Saved/StagedBuilds/Windows/RankAndFile/Binaries/Win64/RankAndFile-Win64-Shipping.exe`를 직접 부른다.
```
(2) 56줄 `run_tests.bat`. 바꾼 뒤:
```
- `run_tests.bat`: `UnrealEditor-Cmd.exe`를 `-ExecCmds="Automation RunTests RankAndFile; Quit" -unattended -nullrhi -nosplash -stdout -ReportExportPath=<저장소>\Saved\TestReport`로 부르고, 보고서 `Saved/TestReport/index.json`으로 판정한다. 실행된 테스트가 0개면 실패로 처리한다. 이 조합은 2026-11-21 실행(테스트 8개 실행, 보고서 생성)으로 확인했다. 테스트가 하나라도 실패하면 에디터는 종료 코드 255로 끝난다(`Engine/Source/Developer/AutomationController/Private/AutomationCommandline.cpp:491-504`, -1이 `uint8`로 바뀐다). 스크립트는 보고서로 판정하고, 모두 통과인데 에디터 종료 코드가 0이 아니면 실패로 본다.
```
(3) "validate_content.js" 절 첫 줄의 검사 목록 끝에 더함: `, 특성 식별자와 조합(GateRepair는 Self만, BattleStart는 traitLimit 비움), 유물 형식(effectKind별 effectParam과 effectTarget), 진형의 밤별 개수(2개), 대소문자만 다른 id 겹침`

## 9. `.gitignore` (규칙 파일은 아니다)

끝에 더함(11/21 결정 4 ③):
```
# MCP 클라이언트 설정(사람마다 다르다)
.mcp.json
```

## 고친 뒤 할 일

- `docs/decisions.md`에 승인 행, 질문 가·나·다의 결정 행(나는 11/21 결정 3 ⑪의 옮겨 적은 오류를 바로잡는 행), 11/22 일지에 기록
- `node tools/hand_crosscheck.js export --check`(규칙 문서는 바뀌지 않으므로 최신이어야 한다)

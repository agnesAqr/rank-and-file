# RankEditor 작업 규칙

에디터에서만 도는 도구 셋이 있다: 콘텐츠 임포터, BalanceSim 커맨들릿(commandlet), 회귀 테스트 결과를 기록하는 RankRegression 커맨들릿. Shipping 패키지에는 들어가지 않는다. 루트와 `Source/AGENTS.md`를 먼저 읽는다.

## 콘텐츠 임포터

- 입력은 `Data/accepted/*.csv`, 출력은 `Content/Data/`의 DataTable 에셋이다.
- 관문 1(`node tools/validate_content.js`)을 통과하지 않은 파일은 임포트하지 않는다.
- 행 구조체와 "행 → 코어 구조체" 변환은 `RankGame`의 것을 쓴다. 이 모듈에 따로 만들지 않는다.
- 문제 메시지가 하나라도 나오면 임포트 전체를 실패로 처리한다. 일부만 들어간 테이블을 남기지 않는다.
- 임포트한 뒤 DataTable의 행 수가 CSV의 항목 수(첫 줄을 뺀 줄 수)와 같아야 한다(T8).
- 콘텐츠 버전 해시(채택본 CSV 3개와 `defs` CSV 2개의 내용을, BOM을 빼고 줄바꿈을 LF로 맞춘 뒤 FNV-1a로 묶은 값)를 함께 저장한다.
- CSV는 `UDataTable::CreateTableFromCSVString`으로 읽는다. [미확인] 이 함수의 5.8 동작(첫 열을 행 이름으로 쓰는지, 문제 메시지를 돌려주는지)을 엔진 소스로 확인하고, 결과를 `docs/decisions.md`에 적은 뒤 이 표시를 지운다.

## BalanceSim

`-run=BalanceSim`으로 부르는 커맨들릿이다. `RankCore`를 직접 부르고 월드, 액터, 렌더링을 쓰지 않는다.

| 모드 | 입력 | 출력 CSV의 열 |
|---|---|---|
| `card` | 후보 CSV, 기준 덱 묶음 | 후보 id, 넣은 승률, 뺀 승률, 기여도, 최고 두 장 조합 승률, 최대 유사도, 판정 |
| `formation` | 진형 CSV, 배정된 밤 | 진형 id, 밤, 봇 승률, 목표, 차이, 판정 |
| `class` | 병과 수치 | 병과, 단일 병과 덱 승률, 1명당 가치 |
| `curve` | 채택본 전체 | 밤, 봇 승률, 목표, 차이, 런 클리어율 |

같은 명령과 같은 시드는 같은 CSV를 내야 한다. 그래서 아래를 지킨다.

- 시드는 인자로 받는다. 시각으로 시드를 만들지 않는다.
- 난수는 자체 정수 난수기를 쓴다. 엔진 난수 함수를 부르지 않는다.
- 출력 행의 순서를 고정한다. 순서가 보장되지 않는 컨테이너를 순회한 결과를 그대로 쓰지 않는다.
- 시각, 소요 시간처럼 실행마다 달라지는 값을 CSV에 넣지 않는다. 처리량(판/초)은 로그로 낸다.

그 밖의 규칙

- 전투 규칙을 여기서 다시 구현하지 않는다. 승률은 `Rank::Simulate`와 `Rank::Judge`의 결과를 센 값이다.
- 관문 2의 통과 기준(기여도 +2~+8%p, 조합 70% 이하, 유사도 0.9 미만, 목표 곡선 ±5%p)을 코드에 숫자로 적지 않는다. 인자나 데이터로 받는다. 기준은 사람이 바꾼다.
- 탐색 봇의 정의를 바꾸지 않는다: 그 밤까지 무작위로 드래프트한 덱으로 무작위 배치 200개를 시험해 가장 나은 배치를 고른다. 봇이 바뀌면 목표 곡선과 지난 리포트를 비교할 수 없다.
- CSV는 `Data/reports/`에 쓴다. 판 수, 시드, 콘텐츠 버전 해시를 파일 이름이나 첫 줄에 남긴다.
- 처리량은 아직 재지 않았다 [측정 필요]. 잰 값은 PC 사양, 빌드 구성, 판 수와 함께 적는다.

## RankRegression

`-run=RankRegression`으로 부르는 커맨들릿이다. `tools\record_regression.bat`이 부른다.

- 기준으로 확정된 `RankCore`로 입력 파일의 대전 20건을 돌려 `Source/RankTests/Fixtures/regression_battles_v1.json`에 쓴다. 대전마다 양쪽 진형, 결과, 틱, 이벤트 수, 해시, 성문 값, 생존 수, 체력 합, 로그 체크섬(`Source/RankTests/AGENTS.md`), 실제로 지난 갈래를 남기고, 파일에 기록한 커밋과, 기록에 쓴 병과 수치·규칙 매개변수를 `units.csv`, `rules.csv`와 같은 열 이름과 값으로 그대로 넣는다.
- 대전 20건의 입력은 `Source/RankTests/Fixtures/regression_inputs.csv`에 고정한다(11/18 결정). 열은 `id,ally_row0,ally_row1,ally_row2,ally_row3,enemy_row0,enemy_row1,enemy_row2,enemy_row3`이고, 한 줄이 대전 하나다. 난수기로 배치를 만들지 않는다. 입력은 AI가 초안을 만들고 사람이 확인하며, 바꾸려면 사람의 승인을 받는다. 입력을 바꾸면 결과도 다시 기록한다.
- 입력 CSV는 `RankGame`의 변환 함수로 읽는다. 이 모듈에 따로 만들지 않는다.
- 갈래 집계(11/18): 대전마다 이벤트 로그, 결과, 입력에서 `Source/RankTests/Fixtures/regression_branches.csv`의 갈래를 몇 번 지났는지 세어 표로 출력한다. 확인 방법은 그 파일의 `detect` 열을 따른다. `tests`에 T1이 있는 갈래 가운데 0건인 것이 있으면 기록하지 않고 그 갈래를 알린다.
- 덮어쓰기 전에 지금 파일과 새 결과의 비교표를 출력한다. 덮어쓰기는 명시적인 인자(`-overwrite`)가 있을 때만 한다.

## 검증

- `tools\run_sim.bat`을 같은 인자로 두 번 돌려 두 CSV가 같은지 비교한다.
- 임포터를 바꿨으면 `tools\run_tests.bat`의 T8을 확인한다.
- 이 모듈의 코드를 Runtime 모듈이 부르지 않는지 `tools\package.bat`으로 확인한다.

# tools/ 작업 규칙

검사와 실행을 한 줄 명령으로 만드는 스크립트가 여기 있다. 루트 `AGENTS.md`를 먼저 읽는다.

| 파일 | 하는 일 | 실행 |
|---|---|---|
| `validate_content.js` | 관문 1 스키마 검사 | `node tools/validate_content.js` |
| `run_tests.bat` | 자동화 테스트 | `tools\run_tests.bat` |
| `record_regression.bat` | 기준으로 확정된 `RankCore`로 회귀 테스트 입력 20건을 돌려 결과를 기록한다 | `tools\record_regression.bat` |
| `run_sim.bat` | BalanceSim | `tools\run_sim.bat` |
| `package.bat` | Shipping 패키지 | `tools\package.bat` |
| `check_shipping_determinism.bat` | Shipping 패키지로 회귀 테스트 입력 20건을 돌려 기록과 비교한다(T11) | `tools\check_shipping_determinism.bat` |
| `hand_crosscheck.js` | 손계산 시나리오 교차 검증: 눈가림 입력 만들기, 검증자 결과의 형식 검사, 기대값과 비교, 확인 상태 적기 | `node tools/hand_crosscheck.js <export\|validate\|compare\|apply>` |

JS 참조 구현은 이 폴더에 두지 않는다. 이 저장소의 기준이 아니다(루트 1절).

## validate_content.js

- 검사: UTF-8 인코딩, 열 이름과 순서, 필수 필드, 어휘 밖의 값, 값 범위, 분대 모양 문자열의 형식(`행:열;행:열`), 칸 연결성, 회전 중복, 이름 8자, 특성 식별자와 조합(`GateRepair`는 `Self`만, `BattleStart`는 `traitLimit` 비움), 유물 형식(`effectKind`별 `effectParam`과 `effectTarget`), 진형의 밤별 개수(2개), 대소문자만 다른 `id` 겹침. 입력 형식은 `Data/AGENTS.md`의 "형식" 절이다.
- CSV 읽기는 내장 모듈로 직접 짠다(아래 "내장 모듈만 쓴다").
- 하나라도 어기면 탈락이다. 경고로 낮추지 않는다. 탈락이 있으면 0이 아닌 종료 코드로 끝난다.
- 탈락 사유는 후보 id, 어긴 규칙, 값으로 이루어진 JSON으로 낸다. 다음 생성 요청에 그대로 붙인다.
- 스키마를 바꾸면 `Data/AGENTS.md`의 형식 설명을 같은 변경 안에서 고친다.
- 내장 모듈만 쓴다. `package.json`과 외부 패키지를 더하지 않는다. 설치 단계 없이 한 줄로 돌아야 한다.

## record_regression.bat

- `UnrealEditor-Cmd.exe`로 `-run=RankRegression` 커맨들릿을 부른다. 입력은 `Source/RankTests/Fixtures/regression_inputs.csv`, 출력은 `Source/RankTests/Fixtures/regression_battles_v1.json`이다.
- 실행하는 때는 둘뿐이다: `RankCore`를 처음 기준으로 확정한 직후, 사람이 승인한 규칙·수치 변경이나 버그 수정 뒤. 테스트가 실패해서 통과시키려고 돌리지 않는다.
- 다시 기록하기 전에 지금 파일과 새 결과를 대전마다 비교한 표(결과, 틱, 이벤트 수, 해시, 성문 값, 생존 수, 체력 합, 로그 체크섬)를 사람에게 보인다.
- 갈래 집계에서 T1이 맡은 갈래가 0건이면 기록하지 않는다(`Source/RankEditor/AGENTS.md`). 승인 없이 파일을 덮어쓰지 않는다.
- 같은 커밋이면 바이트까지 같은 파일이 나와야 한다. 날짜, 시각, 실행 환경을 출력에 넣지 않고 키 순서를 고정한다.

## check_shipping_determinism.bat

- `tools\package.bat`이 만든 Shipping 패키지의 실행 파일을 회귀 확인용 실행 인자로 띄운다. 실행 파일은 `Source/RankTests/Fixtures/regression_inputs.csv`의 20건을 `RankCore`로 돌려 대전마다 T1과 같은 항목(결과, 틱, 이벤트 수, 해시, 성문 값, 생존 수, 체력 합, 로그 체크섬)을 파일로 쓰고 끝난다. 스크립트는 그 값을 `Source/RankTests/Fixtures/regression_battles_v1.json`과 대전마다 비교한다(T11).
- 통과 기준은 20/20 일치다. 다른 대전이 있으면 대전 id, 기대값, 실제값을 적고 0이 아닌 종료 코드로 끝난다. 패키지, 기록 파일, 결과 파일 가운데 하나라도 없거나 결과가 20건이 아니면 실패다.
- 어긋난 대전을 찾을 수 있도록, 대전 하나를 골라 그 이벤트 로그를 파일로 쓰는 실행도 할 수 있어야 한다.
- 실행하는 때: `tools\record_regression.bat`으로 기록하거나 다시 기록한 직후, 그리고 기준 확정 뒤 `tools\package.bat`으로 패키지를 만들 때마다. 빌드 동결 전 마지막 패키지에서도 돌린다.
- 비교만 한다. 기록 파일을 만들거나 고치지 않는다.
- 보고에는 패키지를 만든 커밋과 빌드 구성(Shipping)을 적는다.
- 엔진 소스로 확인한 것(UE 5.8.4, 근거와 파일:줄은 `docs/review/t11-shipping-investigation-2026-11-21.md`):
  - 실행 인자: Shipping에서도 `FCommandLine::Get()`, `FParse::Param`, `FParse::Value`가 그대로 동작한다. 인자를 거르는 허용 목록(`UE_COMMAND_LINE_USES_ALLOW_LIST`)은 기본값이 0이고 이 프로젝트는 켜지 않았다. 실행 파일은 시작할 때 작업 디렉터리를 실행 파일 폴더로 바꾸므로 경로 인자는 절대 경로로 넘긴다.
  - 결과 파일 위치: Shipping의 `FPaths::ProjectSavedDir()`는 사용자 폴더(로컬 AppData) 아래라 쓰지 않는다. 스크립트가 저장소 `Saved/T11/` 안의 절대 경로를 인자로 넘기고, 실행 파일은 그 경로에 쓴다. 입력 CSV와 `Data/defs`도 패키지에 넣지 않고 저장소의 파일을 절대 경로로 읽는다. 11/20 Shipping 패키지를 실행한 뒤 사용자 폴더의 `Saved` 아래에 설정 파일이 생긴 것으로도 확인했다.
  - 로그: Shipping은 `NO_LOGGING`이라 `UE_LOG`, `UE_LOGFMT`가 Fatal 말고는 컴파일되지 않는다. 켜는 설정(`bUseLoggingInShipping`)은 설치형 엔진에서 쓸 수 없다. 판정은 결과 파일(상태, 오류 문장, 빌드 구성, 수치, 대전 20건)과 종료 코드로만 한다. 같은 실행 뒤 그 폴더의 `Logs`는 비어 있었다.
  - 종료 코드: `FPlatformMisc::RequestExitWithStatus(true, 코드)`가 Windows에서 그 코드로 프로세스를 끝낸다. `false`로는 종료 코드가 바뀌지 않는다. 엔진은 크래시하면 3으로, 정상 종료면 0으로 끝내므로 종료 코드 0만으로 통과로 보지 않는다. 인자가 무시되면 게임이 그대로 뜨므로 시간 제한을 둔다. 에디터 테스트 실행(2026-11-21)에서 `RequestExitWithStatus(true, -1)`이 종료 코드 255로 나온 것으로 Windows의 동작을 확인했다(종료 코드는 `uint8`이다. Shipping에서는 아직 시험 전).
- [미확인] 위 방식을 실제 Shipping 패키지에서 돌려 본 결과: `-nullrhi`로 모듈 시작까지 가는지, 모듈 시작 시점에서 돌고 강제 종료했을 때 결과 파일과 종료 코드가 남는지, 스크립트가 종료 코드를 받는지, 스테이징 루트의 `RankAndFile.exe`(부트스트랩)가 인자와 종료 코드를 넘기는지. T11 코드를 넣은 첫 패키지에서 확인하고, 확인되면 이 표시를 지운다. 확인하기 전에는 `Saved/StagedBuilds/Windows/RankAndFile/Binaries/Win64/RankAndFile-Win64-Shipping.exe`를 직접 부른다.

## hand_crosscheck.js

- `export`: `hand_v1.json`에서 병과 수치, 규칙 매개변수, 양쪽 진형만 뽑아 `docs/crosscheck/hand_v1/inputs.json`을 만든다. 기대값, 계산 과정, 목적, 조항 번호, 갈래는 옮기지 않는다. 같은 입력이면 같은 파일이 나온다. `--check`는 지금 파일이 최신인지만 본다.
- `validate`: 검증자 결과 파일의 형식만 본다. `hand_v1.json`을 읽지 않는다. 검증자가 실행해도 되는 하위 명령은 이것뿐이다.
- `compare`: 표준 출력에는 개수, 시나리오 id, 어긋난 항목 이름만 낸다. 값은 `--report`가 쓰는 사람용 보고서(`report.md`)에만 적는다. 전부 일치하고 모호 표시와 허용 목록 밖 파일이 없을 때만 종료 코드 0이다. 결과 파일의 `inputsId`가 지금의 `hand_v1.json`, `docs/rules.md`와 다르면 비교하지 않는다.
- `apply`: 비교한 항목이 전부 일치하고 지금 `확인 전`인 시나리오의 확인 상태만 `교차 확인(모델, 날짜)`로 바꾼다. 파일의 다른 글자는 건드리지 않는다. 바꾼 뒤 다시 읽어 확인 상태 말고 달라진 것이 있으면 쓰지 않는다. 허용 목록 밖 파일이 있는 실행에는 적지 않는다.
- 전투 규칙을 해석하는 코드를 넣지 않는다. 형식 검사와 값 비교만 한다. 내장 모듈만 쓴다.

## 실행 스크립트 공통

- 실패하면 0이 아닌 종료 코드로 끝난다. 실패를 삼키지 않는다.
- 엔진 경로는 `%UE_ROOT%`에서 읽는다. 사용자 폴더가 들어간 절대 경로를 적지 않는다.
- `run_tests.bat`: `UnrealEditor-Cmd.exe`를 `-ExecCmds="Automation RunTests RankAndFile; Quit" -unattended -nullrhi -nosplash -stdout -ReportExportPath=<저장소>\Saved\TestReport`로 부르고, 보고서 `Saved/TestReport/index.json`으로 판정한다. 실행된 테스트가 0개면 실패로 처리한다. 이 조합은 2026-11-21 실행(테스트 8개 실행, 보고서 생성)으로 확인했다. 테스트가 하나라도 실패하면 에디터는 종료 코드 255로 끝난다(`Engine/Source/Developer/AutomationController/Private/AutomationCommandline.cpp:491-504`, -1이 `uint8`로 바뀐다). 스크립트는 보고서로 판정하고, 모두 통과인데 에디터 종료 코드가 0이 아니면 실패로 본다.
- `run_sim.bat`: `-run=BalanceSim`. 모드와 시드를 인자로 받고 CSV를 `Data/reports/`에 쓴다.
- `package.bat`: `RunUAT.bat BuildCookRun`. 인자는 에디터의 실행 프로필(Custom Launch Profile)이 만든 명령을 옮긴다. 인자를 추측으로 쓰지 않는다.

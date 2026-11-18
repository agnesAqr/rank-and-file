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

JS 참조 구현은 이 폴더에 두지 않는다. 이 저장소의 기준이 아니다(루트 1절).

## validate_content.js

- 검사: UTF-8 인코딩, 열 이름과 순서, 필수 필드, 어휘 밖의 값, 값 범위, 분대 모양 문자열의 형식(`행:열;행:열`), 칸 연결성, 회전 중복, 이름 8자. 입력 형식은 `Data/AGENTS.md`의 "형식" 절이다.
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
- [미확인] Shipping 실행 파일이 실행 인자를 읽는 방법, 결과 파일을 쓸 수 있는 위치, Shipping에서 로그가 꺼지는지, 끝날 때 종료 코드를 돌려주는 방법. 엔진 소스로 확인하고(`docs/design.md` 14.1절), 확인되면 이 표시를 지운다. 확인하기 전에 추측으로 짜지 않는다.

## 실행 스크립트 공통

- 실패하면 0이 아닌 종료 코드로 끝난다. 실패를 삼키지 않는다.
- 엔진 경로는 `%UE_ROOT%`에서 읽는다. 사용자 폴더가 들어간 절대 경로를 적지 않는다.
- `run_tests.bat`: `UnrealEditor-Cmd.exe`를 `-ExecCmds="Automation RunTests RankAndFile; Quit" -unattended -nullrhi -nosplash -stdout`으로 부르고 결과를 `Saved/TestReport`에 남긴다. 실행된 테스트가 0개면 실패로 처리한다. [미확인] 이 인자 조합은 첫 실행에서 확인하고, 확인되면 이 표시를 지운다.
- `run_sim.bat`: `-run=BalanceSim`. 모드와 시드를 인자로 받고 CSV를 `Data/reports/`에 쓴다.
- `package.bat`: `RunUAT.bat BuildCookRun`. 인자는 에디터의 실행 프로필(Custom Launch Profile)이 만든 명령을 옮긴다. 인자를 추측으로 쓰지 않는다.

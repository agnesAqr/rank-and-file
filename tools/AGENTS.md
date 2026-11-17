# tools/ 작업 규칙

검사와 실행을 한 줄 명령으로 만드는 스크립트가 여기 있다. 루트 `AGENTS.md`를 먼저 읽는다.

| 파일 | 하는 일 | 실행 |
|---|---|---|
| `validate_content.js` | 관문 1 스키마 검사 | `node tools/validate_content.js` |
| `run_tests.bat` | 자동화 테스트 | `tools\run_tests.bat` |
| `record_golden.bat` | 기준으로 확정된 `RankCore`로 골든 벡터 20건을 기록한다 | `tools\record_golden.bat` |
| `run_sim.bat` | BalanceSim | `tools\run_sim.bat` |
| `package.bat` | Shipping 패키지 | `tools\package.bat` |

JS 참조 구현은 이 폴더에 두지 않는다. 이 저장소의 기준이 아니다(루트 1절).

## validate_content.js

- 검사: 필수 필드, 어휘 밖의 값, 값 범위, 칸 연결성, 회전 중복, 이름 8자.
- 하나라도 어기면 탈락이다. 경고로 낮추지 않는다. 탈락이 있으면 0이 아닌 종료 코드로 끝난다.
- 탈락 사유는 후보 id, 어긴 규칙, 값으로 이루어진 JSON으로 낸다. 다음 생성 요청에 그대로 붙인다.
- 스키마를 바꾸면 `Data/AGENTS.md`의 형식 설명을 같은 변경 안에서 고친다.
- 내장 모듈만 쓴다. `package.json`과 외부 패키지를 더하지 않는다. 설치 단계 없이 한 줄로 돌아야 한다.

## record_golden.bat

- `UnrealEditor-Cmd.exe`로 `-run=RankGolden` 커맨들릿을 부르고 `Source/RankTests/Fixtures/golden_v1.json`을 쓴다.
- 실행하는 때는 둘뿐이다: `RankCore`를 처음 기준으로 확정한 직후, 사람이 승인한 규칙·수치 변경이나 버그 수정 뒤. 테스트가 실패해서 통과시키려고 돌리지 않는다.
- 다시 기록하기 전에 지금 파일과 새 결과를 대전마다 비교한 표(결과, 틱, 이벤트 수, 해시)를 사람에게 보인다. 승인 없이 파일을 덮어쓰지 않는다.
- 같은 커밋이면 바이트까지 같은 파일이 나와야 한다. 날짜, 시각, 실행 환경을 출력에 넣지 않고 키 순서를 고정한다.

## 실행 스크립트 공통

- 실패하면 0이 아닌 종료 코드로 끝난다. 실패를 삼키지 않는다.
- 엔진 경로는 `%UE_ROOT%`에서 읽는다. 사용자 폴더가 들어간 절대 경로를 적지 않는다.
- `run_tests.bat`: `UnrealEditor-Cmd.exe`를 `-ExecCmds="Automation RunTests RankAndFile; Quit" -unattended -nullrhi -nosplash -stdout`으로 부르고 결과를 `Saved/TestReport`에 남긴다. 실행된 테스트가 0개면 실패로 처리한다. [미확인] 이 인자 조합은 첫 실행에서 확인하고, 확인되면 이 표시를 지운다.
- `run_sim.bat`: `-run=BalanceSim`. 모드와 시드를 인자로 받고 CSV를 `Data/reports/`에 쓴다.
- `package.bat`: `RunUAT.bat BuildCookRun`. 인자는 에디터의 실행 프로필(Custom Launch Profile)이 만든 명령을 옮긴다. 인자를 추측으로 쓰지 않는다.

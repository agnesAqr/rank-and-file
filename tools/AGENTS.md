# tools/ 작업 규칙

검사와 실행을 한 줄 명령으로 만드는 스크립트가 여기 있다. 루트 `AGENTS.md`를 먼저 읽는다.

| 파일 | 하는 일 | 실행 |
|---|---|---|
| `reference/rank-and-file-core.js` | 전투 규칙의 JS 참조 구현. C++ 코어의 정답 | 다른 스크립트가 불러 쓴다 |
| `export_golden.js` | 골든 벡터와 픽스처 JSON을 만든다 | `node tools/export_golden.js` |
| `validate_content.js` | 관문 1 스키마 검사 | `node tools/validate_content.js` |
| `run_tests.bat` | 자동화 테스트 | `tools\run_tests.bat` |
| `run_sim.bat` | BalanceSim | `tools\run_sim.bat` |
| `package.bat` | Shipping 패키지 | `tools\package.bat` |

## 참조 구현

- 참조 구현은 정답이다. C++과 결과가 다르면 C++을 먼저 의심한다. 테스트를 통과시키려고 참조 구현을 고치지 않는다.
- 고치는 경우는 둘뿐이다: 사람이 규칙이나 수치를 바꿨을 때, 참조 구현이 `docs/rules.md`와 다르다는 것을 사람이 확인했을 때.
- 고쳤으면 같은 변경 안에서 픽스처를 다시 만들고(`node tools/export_golden.js`), 테스트를 돌리고(`tools\run_tests.bat`), 문서의 [산정] 수치를 다시 계산한다.
- 전투 계산은 정수만 쓴다. 해시는 `Math.imul`과 `>>> 0`으로 32비트를 유지한다.
- `mulberry32`와 `randomFormation`은 실수 연산을 쓴다. 전투가 아니라 무작위 배치를 만드는 함수다. C++로 옮기지 않고 픽스처로 대신한다.
- 참조 구현에는 특성과 유물이 없다. 그 부분의 정답은 손으로 계산한 예제다(`Source/RankTests/AGENTS.md`의 T7).

## export_golden.js

| 출력 (`Source/RankTests/Fixtures/`) | 내용 |
|---|---|
| `golden_v1.json` | 시드 1~20의 대전 20건: 양쪽 진형, 결과, 틱, 이벤트 수, 해시. 상대는 시드를 3으로 나눈 나머지 0·1·2에 따라 밤 3·7·12 |
| `formations_1000.json` | 시드 20261113으로 만든 무작위 배치 1,000개 |

- 같은 입력이면 바이트까지 같은 파일이 나와야 한다. 날짜, 시각, 실행 환경을 출력에 넣지 않고 키 순서를 고정한다.
- 픽스처에 병과 수치와 규칙 매개변수의 해시를 넣는다. 수치가 바뀐 뒤에 예전 픽스처로 테스트하는 일을 막는다.
- 수치는 참조 구현의 상수와 `Data/defs/` 두 곳에 있다. 둘이 같은지 먼저 검사하고, 다르면 픽스처를 만들지 않고 실패한다.
- 무작위 배치 함수는 분대를 40번 시도해 못 놓으면 건너뛴다. 그래서 1,000개 가운데 분대 하나가 빠진 배치가 섞여 있다(현재 7개 [산정]). 걸러 내지 않는다. 문서의 기준 승률이 이 1,000개로 낸 값이다.

## validate_content.js

- 검사: 필수 필드, 어휘 밖의 값, 값 범위, 칸 연결성, 회전 중복, 이름 8자.
- 하나라도 어기면 탈락이다. 경고로 낮추지 않는다. 탈락이 있으면 0이 아닌 종료 코드로 끝난다.
- 탈락 사유는 후보 id, 어긴 규칙, 값으로 이루어진 JSON으로 낸다. 다음 생성 요청에 그대로 붙인다.
- 스키마를 바꾸면 `Data/AGENTS.md`의 형식 설명을 같은 변경 안에서 고친다.

## 실행 스크립트

- 실패하면 0이 아닌 종료 코드로 끝난다. 실패를 삼키지 않는다.
- 엔진 경로는 `%UE_ROOT%`에서 읽는다. 사용자 폴더가 들어간 절대 경로를 적지 않는다.
- `run_tests.bat`: `UnrealEditor-Cmd.exe`를 `-ExecCmds="Automation RunTests RankAndFile; Quit" -unattended -nullrhi -nosplash -stdout`으로 부르고 결과를 `Saved/TestReport`에 남긴다. 실행된 테스트가 0개면 실패로 처리한다. [미확인] 이 인자 조합은 첫 실행에서 확인하고, 확인되면 이 표시를 지운다.
- `run_sim.bat`: `-run=BalanceSim`. 모드와 시드를 인자로 받고 CSV를 `Data/reports/`에 쓴다.
- `package.bat`: `RunUAT.bat BuildCookRun`. 인자는 에디터의 실행 프로필(Custom Launch Profile)이 만든 명령을 옮긴다. 인자를 추측으로 쓰지 않는다.
- Node 스크립트는 내장 모듈만 쓴다. `package.json`과 외부 패키지를 더하지 않는다. 설치 단계 없이 한 줄로 돌아야 한다.

# Source/ 작업 규칙 (C++ 공통)

모듈 네 개의 경계가 이 프로젝트의 구조다. 루트 `AGENTS.md`를 먼저 읽고, 고칠 모듈의 `AGENTS.md`를 이어서 읽는다.

## 모듈

| 모듈 | 타입 | 의존해도 되는 것 | 맡는 일 | Shipping |
|---|---|---|---|---|
| `RankCore` | Runtime | 엔진 `Core`만 | 규칙, 전투, 이벤트 로그, 해시, 링크 계산, 배치 판정 | 포함 |
| `RankGame` | Runtime | `RankCore` | 런 진행, 전투 재생, UI, 설정, DataTable 행 구조체와 코어 구조체의 변환 | 포함 |
| `RankEditor` | Editor | `RankCore`, `RankGame` | JSON → DataTable 임포터, BalanceSim 커맨들릿(commandlet) | 제외 |
| `RankTests` | Editor | `RankCore`, `RankGame` | 자동화 테스트 | 제외 |

- 의존은 한 방향이다: `RankEditor`·`RankTests` → `RankGame` → `RankCore`. 거꾸로 include하지 않는다.
- `*.Build.cs`의 의존 모듈을 늘리기 전에 알린다. `RankCore`에는 `CoreUObject`와 `Engine`을 더하지 않는다.
- Runtime 모듈은 Editor 모듈의 코드를 부르지 않는다. Editor 타입 모듈은 런타임 빌드에 컴파일되지 않는다([Epic: Unreal Engine Modules](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-modules)).
- 모듈을 더하거나 타입을 바꾸지 않는다. 필요해 보이면 이유와 함께 제안한다.

## 코드를 어디에 두는가

| 코드가 하는 일 | 모듈 |
|---|---|
| 전투 결과를 바꾼다(피해, 표적, 링크, 승패, 배치 가능 여부) | `RankCore` |
| 화면, 입력, 런 상태, 설정, 리플렉션이 필요한 타입(`USTRUCT`, `UCLASS`) | `RankGame` |
| 에디터에서만 도는 도구, 콘텐츠 변환, 시뮬레이션 | `RankEditor` |
| 검증 | `RankTests` |

같은 계산이 두 모듈에 필요하면 낮은 쪽(`RankCore`)에 하나만 두고 위에서 부른다. 복사하지 않는다.

## 코드 규칙

- Epic C++ 코딩 표준을 따른다([Epic C++ Coding Standard](https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine)): 타입 접두사(`U`, `A`, `S`, `I`, `E`, `F`, `T`), 불리언의 `b` 접두사, 낱말 첫 글자 대문자, 크기가 정해진 정수형(`int32`, `uint8`), const 정확성.
- 헤더는 `Public/`, 구현은 `Private/`에 둔다. 다른 모듈이 쓰지 않는 헤더는 `Private/`에 둔다.
- 게임 수치를 코드에 적지 않는다. 규칙 문서가 고정한 상수(해시 시작값, 유닛 번호 식)만 코드에 둔다.
- 규칙을 구현한 곳에는 조항 번호를 주석으로 단다(예: `// §2.6 한 틱의 공격은 모은 뒤 한꺼번에 적용`).
- 로직은 C++에 둔다. 블루프린트는 에셋 연결과 화면 배치에만 쓴다. 블루프린트 안의 로직은 테스트와 코드 리뷰에 걸리지 않는다.
- 엔진은 5.8이다. 쓰려는 API가 5.8에 있는지 엔진 소스에서 확인한다. 컴파일 오류를 추측으로 고치지 않는다.
- 플러그인에 기대는 코드를 더하지 않는다. 패키징이 깨지는 원인을 줄인다.

## 검증

- 코드를 바꿨으면 `tools\run_tests.bat`을 돌린다.
- 모듈, `*.Build.cs`, `*.Target.cs`, `.uproject`를 바꿨으면 `tools\package.bat`도 돌린다. 개발 빌드에서 되던 것이 Shipping에서 깨지는 곳이 여기다.
- 컴파일만 되고 실행해 보지 않았으면 "컴파일 확인, 실행 안 함"이라고 적는다.

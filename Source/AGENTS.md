# Source/ 작업 규칙 (C++ 공통)

모듈 네 개의 경계가 이 프로젝트의 구조다. 루트 `AGENTS.md`를 먼저 읽고, 고칠 모듈의 `AGENTS.md`를 이어서 읽는다.

## 모듈

| 모듈 | 타입 | 의존해도 되는 것 | 맡는 일 | Shipping |
|---|---|---|---|---|
| `RankCore` | Runtime | 엔진 `Core`만 | 규칙, 전투, 이벤트 로그, 해시, 링크 계산, 배치 판정 | 포함 |
| `RankGame` | Runtime | `RankCore` | 런 진행, 전투 재생, UI, 설정, DataTable 행 구조체와 코어 구조체의 변환 | 포함 |
| `RankEditor` | Editor | `RankCore`, `RankGame` | JSON → DataTable 임포터, BalanceSim·RankGolden 커맨들릿(commandlet) | 제외 |
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
- 블루프린트에 로직을 넣지 않는다(2026-11-17 결정). 블루프린트 안의 로직은 테스트와 코드 리뷰에 걸리지 않는다. UI와 애니메이션은 먼저 C++로 만들고, 세부 조정만 에디터에서 한다. 블루프린트로 하던 일은 아래처럼 한다.

| 하던 일 | C++로 하는 방법 |
|---|---|
| 게임 모드, 플레이어 컨트롤러 지정 | C++ 클래스를 프로젝트 설정(`Config/DefaultEngine.ini`)에 지정한다 |
| 에셋 연결(메시, 애니메이션, 텍스처, 사운드) | C++로 정의한 데이터 에셋 클래스(`UPrimaryDataAsset` 하위)의 인스턴스에 담는다. 데이터 에셋은 블루프린트가 아니다. 경로 문자열을 코드에 흩어 쓰지 않는다 |
| 애니메이션 재생 | 먼저 `USkeletalMeshComponent::PlayAnimation`으로 시퀀스를 직접 재생한다. 이 함수는 메시를 단일 노드 모드로 바꾼다. 블렌딩 같은 세부 조정이 필요하면 C++ `UAnimInstance` 하위 클래스를 부모로 하는 애니메이션 블루프린트를 만든다. 상태 값(현재 동작, 재생 속도)은 C++이 정하고, 블루프린트에는 애니메이션 그래프만 둔다. 이때부터는 `PlayAnimation`을 부르지 않는다. 애니메이션 블루프린트 모드가 풀린다 |
| UI | 동작(입력 처리, 뷰모델 연결, 표시 값 갱신)은 C++ `UUserWidget` 하위 클래스에 둔다. 배치·글꼴·색·크기는 에디터에서 조정한다: 그 클래스를 부모로 한 위젯 블루프린트에서 배치하고, C++은 `meta=(BindWidget)`으로 이름이 같은 위젯을 받는다. [미정] 처음부터 디자이너로 배치할지, 코드로 배치하고 조정값만 에디터에 열어 둘지는 사람이 정한다 |
| 레벨의 연출과 이벤트 | 레벨 블루프린트 대신 레벨에 놓은 C++ 액터나 서브시스템에서 처리한다 |

- 블루프린트 노출 지정자는 조정에 필요한 것만 쓴다.
  - 허용: `meta=(BindWidget)`, `meta=(BindWidgetOptional)`, 애니메이션 그래프가 읽는 상태 변수의 `BlueprintReadOnly`, 에디터에서 조정할 값의 `EditDefaultsOnly`.
  - 금지: 블루프린트에서 로직을 부르거나 덮어쓰게 하는 지정자(`BlueprintCallable`, `BlueprintImplementableEvent`, `BlueprintNativeEvent`, `BlueprintAssignable`).
- 위젯 블루프린트와 애니메이션 블루프린트의 이벤트 그래프에 노드를 두지 않는다. 조정할 값이 로직처럼 보이면(조건, 계산) C++로 옮긴다.
- [미확인] `BindWidget` 지정자의 동작은 공식 문서보다 포럼 사례로 알려져 있다. 5.8 엔진 헤더에서 확인하고 이 표시를 지운다.
- 데이터 에셋은 다른 에셋이 참조하지 않으면 쿠킹에서 빠질 수 있다. 새 데이터 에셋을 만들면 `tools\package.bat`으로 Shipping에서 읽히는지 확인한다.
- 빌드 도구는 Visual Studio 2022 17.14 이상 또는 Visual Studio 2026이다(5.8 릴리스 노트). [미확인] 실제로 쓰는 버전은 첫 빌드에서 확인해 `README.md`에 적는다.
- 엔진은 5.8이다. 쓰려는 API가 5.8에 있는지 엔진 소스에서 확인한다. 컴파일 오류를 추측으로 고치지 않는다.
- 플러그인에 기대는 코드를 더하지 않는다. 패키징이 깨지는 원인을 줄인다.

## 검증

- 코드를 바꿨으면 `tools\run_tests.bat`을 돌린다.
- 모듈, `*.Build.cs`, `*.Target.cs`, `.uproject`를 바꿨으면 `tools\package.bat`도 돌린다. 개발 빌드에서 되던 것이 Shipping에서 깨지는 곳이 여기다.
- 컴파일만 되고 실행해 보지 않았으면 "컴파일 확인, 실행 안 함"이라고 적는다.

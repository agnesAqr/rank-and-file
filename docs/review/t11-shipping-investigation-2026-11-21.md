# T11 Shipping 결정성 사전 조사 (2026-11-21)

상태: 검토 중 · AI 제안

T11(Shipping 결정성)을 구현하기 전에, `tools/AGENTS.md`의 check_shipping_determinism.bat 절에 남아 있는 [미확인] 4개(실행 인자, 결과 파일 위치, 로그, 종료 코드)와 입력 파일의 패키징을 UE 5.8.4 엔진 소스로 확인했다. 이 결과를 바탕으로 `RankGame`에 둘 코드와 `tools\check_shipping_determinism.bat`의 흐름을 제안하고, `tools/AGENTS.md` 수정안을 낸다. 구현, 설정 변경, 결정은 하지 않았다.

- 엔진: `%UE_ROOT%` 설치본. `Engine/Build/Build.version`이 5.8.4(Changelist 58902052)이고, `Engine/Build/InstalledBuild.txt`가 있는 설치형 엔진이다. 아래 엔진 경로는 엔진 루트 기준이고 줄 번호도 이 설치본 기준이다.
- 저장소: 브랜치 `feat/rules-and-rankcore`, 마지막 커밋 `099f906`. 프로젝트 파일의 줄 번호는 이 커밋 기준이다(작업 트리의 미커밋 변경은 `docs/`뿐이라 인용한 코드와 무관하다).
- 한 일: 엔진 소스와 저장소 파일을 읽기만 했다. 빌드, 에디터 실행, 패키지 만들기, 실행 파일 실행은 하지 않았다. 아래에서 "확인(소스)"는 코드 경로를 읽어서 확인했다는 뜻이다. 실제로 실행해서 확인한 것은 없다.
- 열지 않은 것: `Source/RankTests/Fixtures/hand_v1.json`, `docs/crosscheck/`의 결과와 보고서.
- 부트스트랩 실행 파일(스테이징 루트의 `RankAndFile.exe`)의 소스는 설치형 엔진에 없다. `Engine/Binaries/Win64/BootstrapPackagedGame-Win64-Shipping.exe`만 있다. 그래서 이 실행 파일의 동작은 근거로 쓰지 않았다.

작업 정의(루트 `AGENTS.md` 3절 1번):

```
목표: T11 구현 전에 Shipping 실행 파일의 실행 인자, 쓸 수 있는 위치, 로그, 종료 코드, 입력 파일 접근을 UE 5.8.4 엔진 소스로 확인하고, RankGame 쪽 설계안과 tools/AGENTS.md 수정안을 낸다
완료 기준: Q1~Q5마다 결론, 엔진 소스의 파일:줄과 짧은 인용, 이 프로젝트에 주는 뜻, 남은 [미확인]과 확인 방법이 있다. 설계안과 수정안이 있다. 인용한 파일:줄은 모두 직접 연 것이다
검증 명령: 없음(문서만 만든다). 인용한 엔진 파일:줄을 다시 열어 대조한다
금지: 이 문서 밖의 파일 생성·수정, 코드 작성, 빌드·에디터 실행·패키지·플러그인, .uproject·Config/·*.Target.cs 수정, 규칙 파일·docs/rules.md·README.md 수정, hand_v1.json·docs/crosscheck 결과 열기, git 쓰기 명령, 개인 정보·사용자 폴더 경로
산출물: docs/review/t11-shipping-investigation-2026-11-21.md
```

## 0. 요약

| 질문 | 결론 | 확인 정도 |
|---|---|---|
| Q1 실행 인자 | Shipping에서도 `FCommandLine::Get()`, `FParse::Param`, `FParse::Value`가 그대로 동작한다. 인자를 걸러내는 허용 목록은 기본으로 꺼져 있고 이 프로젝트도 켜지 않았다 | 확인(소스) |
| Q2 쓸 수 있는 위치 | Shipping에서 `FPaths::ProjectSavedDir()`는 사용자 폴더(로컬 AppData) 아래로 간다. 스크립트가 저장소 `Saved/T11/`의 절대 경로를 인자로 넘기고 실행 파일이 그대로 쓰는 방식이 안전하다. 절대 경로 쓰기를 막는 곳은 없다 | 확인(소스). `ProjectDir()`가 어느 분기를 타는지는 일부 |
| Q3 로그 | Shipping은 `NO_LOGGING`이다. 켜는 설정 `bUseLoggingInShipping`은 기본 false이고, 이 프로젝트도 false이며, 설치형 엔진에서는 켤 수 없다. T11은 결과 파일과 종료 코드로만 판정해야 한다 | 확인(소스) |
| Q4 종료 코드와 시점 | `FPlatformMisc::RequestExitWithStatus(true, 코드)`가 Windows에서 `TerminateProcess(코드)`로 끝낸다. `false`(정상 종료)로는 종료 코드를 바꿀 수 없다. 크래시는 3으로 끝난다. 일을 할 시점으로는 `RankGame` 모듈의 `StartupModule`을 권한다(대안 `FCoreDelegates::GetOnPostEngineInit()`) | 일부. 코드 경로는 확인했고 실행 시험은 하지 않았다 |
| Q5 입력 파일 | `DirectoriesToAlwaysStageAsNonUFS`는 Content 폴더 기준 경로라서 `Source/RankTests/Fixtures`와 `Data/defs`에 맞지 않는다. 명령줄로 저장소의 절대 경로를 넘겨 읽게 하는 쪽을 권한다. pak 파일 계층은 `.csv`를 pak 밖에서 읽는 것을 막지 않는다 | 확인(소스). `..`이 든 스테이징 경로의 동작은 미확인 |

## 1. Q1 실행 인자

결론: Shipping 실행 파일에서도 `FCommandLine::Get()`, `FParse::Param`, `FParse::Value`가 개발 빌드와 같은 코드로 동작한다. 인자를 걸러내는 장치는 명령줄 허용 목록(`UE_COMMAND_LINE_USES_ALLOW_LIST`) 하나다. 기본값은 0(꺼짐)이고, 엔진과 이 프로젝트 어디에서도 켜지 않는다.

근거

- 허용 목록의 기본값: `Engine/Source/Runtime/Core/Public/Misc/CommandLine.h:14-20`. `#ifndef UE_COMMAND_LINE_USES_ALLOW_LIST` … `#define UE_COMMAND_LINE_USES_ALLOW_LIST 0`. 같은 파일 167-180줄에서 매크로가 0이면 거르는 함수가 빈 매크로가 된다: `#define ApplyCommandLineAllowList()`.
- 켜졌을 때의 동작: `Engine/Source/Runtime/Core/Private/Misc/CommandLine.cpp:96-97` `// If configured as part of the build, strip out any unapproved args` / `ApplyCommandLineAllowList();`. 기본 허용 목록은 242-243줄 `// Default list most conservative restrictions` / `const TCHAR* OverrideList = TEXT("-fullscreen /windowed");`. 켜면 이 두 인자 말고는 모두 지워진다. 켜는 방법은 232-236줄 주석에 있다: `GlobalDefinitions.Add("UE_OVERRIDE_COMMAND_LINE_ALLOW_LIST=...")`(Target.cs에서 정의).
- 켜는 곳이 없음: `Engine/Source`(UnrealBuildTool 포함), `Engine/Platforms`, `Engine/Config`를 `UE_COMMAND_LINE_USES_ALLOW_LIST`와 옛 이름 `WANTS_COMMANDLINE_WHITELIST`로 찾았더니 정의는 `CommandLine.h`뿐이었다. 이 프로젝트의 `Source/RankAndFile.Target.cs:7-13`과 `Source/*/*.Build.cs`에도 `GlobalDefinitions`가 없다.
- `FCommandLine::Get()`에 Shipping 조건이 없다: `CommandLine.cpp:61-65` `UE_CLOGF(!bIsInitialized, ...)` / `return CmdLine;`.
- `FParse::Param`(`Engine/Source/Runtime/Core/Private/Misc/Parse.cpp:325-351`)과 `FParse::Value`(같은 파일 242-320줄, `FString` 판은 356-376줄)에도 Shipping 조건이 없다. `Parse.cpp`에서 Shipping으로 갈리는 곳은 21-237줄(`DumpConsoleCommands`), 769-780줄(`FParse::Command` 안의 콘솔 명령 수집), 1494-1609줄(자동화 테스트)뿐이다.
- Windows 진입점: `Engine/Source/Runtime/Launch/Private/Windows/LaunchWindows.cpp:338` `Result = LaunchWindowsStartup(hInInstance, hPrevInstance, pCmdLine, nCmdShow, nullptr);`. 명령줄이 `nullptr`이면 198-208줄에서 `ProcessCommandLine()`으로 다시 만든다. 147-183줄은 `::CommandLineToArgvW`로 나눈 뒤, 공백이 든 인자는 `=` 바로 뒤에 따옴표를 넣는다(161-173줄 `Temp = Temp.Left(Quote) + TEXT("\"") + Temp.Mid(Quote) + TEXT("\"");`). 엔진에는 `Engine/Source/Runtime/Launch/Private/LaunchEngineLoop.cpp:1808` `if (!FCommandLine::Set(CmdLine))`로 들어간다.
- Shipping에서 달라지는 것: 명령줄이 `MaxCommandLineSize`(16384, `CommandLine.h:48`)를 넘으면 Shipping은 대화 상자 없이 Fatal로 끝난다(`CommandLine.cpp:102-117`, 대화 상자는 `#if !UE_BUILD_SHIPPING` 안). 메일에서 붙여 넣은 대시 문자가 있어도 Fatal이다(119-133줄 `Command-line contains an invalid '-' character`).
- `FParse::Value`의 성질: 대소문자를 가리지 않고 부분 문자열로 찾는다(`Parse.cpp:265` `FCString::Strifind(Stream, Match, true)`). 따옴표가 없으면 `bShouldStopOnSeparator`(기본 true, `Engine/Source/Runtime/Core/Public/Misc/Parse.h:71`)일 때 `,`, `)`, 공백에서 끊는다(`Parse.cpp:298-300` `TEXT(",) \r\n\t")`). 따옴표가 있으면 닫는 따옴표까지 읽는다(278-291줄).
- `FParse::Param`은 앞이 `-`나 `/`이고 뒤가 공백이나 끝일 때만 참이다(`Parse.cpp:337-344`). 그래서 `-RankT11Root=...`가 있어도 `FParse::Param(..., TEXT("RankT11"))`는 그 인자로는 참이 되지 않는다.

이 프로젝트에 주는 뜻

- `Target.cs`를 바꾸지 않고 자체 실행 인자를 쓸 수 있다.
- 경로 값은 `FParse::Value(..., /*bShouldStopOnSeparator*/ false)`로 읽어야 한다. 그러면 따옴표가 없는 값은 공백에서만 끊긴다. 괄호나 쉼표가 든 경로(공백이 없으면 따옴표가 붙지 않는다)가 잘리지 않는다 [판단].
- 인자 이름은 다른 인자의 일부가 되지 않도록 고유한 접두사를 붙인다(6.2절의 `RankT11`). 스크립트는 ASCII 하이픈만 쓰고 명령줄을 짧게 유지한다.
- 나중에 누군가 허용 목록을 켜면 T11 인자가 조용히 지워진다. 그러면 게임이 그대로 떠서 끝나지 않는다. 그래서 스크립트에 시간 제한이 있어야 하고, "결과 파일이 없음"이 실패로 드러나야 한다(6.7절).

남은 [미확인]

- 실제 Shipping 패키지에서 인자가 전달되는지는 실행해 보지 않았다. 확인 방법: 6.1절의 코드를 넣은 패키지에서 `-RankT11 -RankT11Root=... -RankT11Out=...`로 돌려 결과 파일이 생기는지 본다.

## 2. Q2 쓸 수 있는 위치

결론: Shipping 데스크톱 빌드는 "설치됨"으로 취급되어 `FPaths::ProjectSavedDir()`가 `%LOCALAPPDATA%/RankAndFile/Saved/`로 간다. 사용자마다 다르고 `-userdir=`, `-saveddirsuffix=` 인자로도 바뀌므로 스크립트가 짐작할 위치로 쓰기에 알맞지 않다. `FPaths::ProjectDir()`는 스테이징 폴더 안의 `RankAndFile/`를 가리킨다. 명령줄로 받은 절대 경로에 쓰는 것을 막는 곳은 없다. 상위 폴더가 없으면 엔진이 만든다.

근거

- Shipping 데스크톱은 설치형으로 본다: `Engine/Source/Runtime/Core/Private/Misc/App.cpp:218-222` `#if UE_BUILD_SHIPPING && PLATFORM_DESKTOP && !UE_SERVER` / `bool bIsInstalled = true;`. `-NotInstalled`로 끌 수 있다(230-234줄).
- 그러면 사용자 폴더에 저장한다: `Engine/Source/Runtime/Core/Private/Misc/Paths.cpp:183-191` `FApp::IsInstalled()` / `|| FParse::Param(FCommandLine::Get(), TEXT("SaveToUserDir"))`.
- 사용자 폴더: `Paths.cpp:460-466` `return FPaths::Combine(FPlatformProcess::UserSettingsDir(), FApp::GetProjectName()) + TEXT("/");`. Windows의 `UserSettingsDir`는 로컬 AppData다: `Engine/Source/Runtime/Core/Private/Windows/WindowsPlatformProcess.cpp:1466-1471` `SHGetKnownFolderPath(... FOLDERID_LocalAppData, ...)`.
- Saved: `Paths.cpp:85-87` `FString Result = BaseDir + TEXT("Saved");`, `ProjectSavedDir()`는 485-494줄. `-saveddirsuffix=`는 90줄, `-userdir=`는 1946-1960줄에서 이 값을 바꾼다.
- `ProjectDir()`: `Engine/Source/Runtime/Core/Private/GenericPlatform/GenericPlatformMisc.cpp:1638-1639` `ProjectDir = FString::Printf(TEXT("../../../%s/"), FApp::GetProjectName());`(모놀리식 게임). 실행 파일 폴더(`RankAndFile/Binaries/Win64/`)에서 세 단계 위의 `RankAndFile/`, 즉 `Saved/StagedBuilds/Windows/RankAndFile/`이다. 같은 함수 1569-1574줄은 `GConfig`가 준비되기 전과 후에 캐시를 다시 만든다.
- 작업 디렉터리: `LaunchEngineLoop.cpp:1801-1803` `// Switch into executable's directory.` / `FPlatformProcess::SetCurrentWorkingDirectoryToBaseDir();`. 상대 경로는 스크립트가 있는 곳이 아니라 실행 파일 폴더를 기준으로 풀린다 [판단].
- 쓰기 경로: `Engine/Source/Runtime/Core/Private/Misc/FileHelper.cpp:778` `FileManager->CreateFileWriter( Filename, WriteFlags )`, 817-818줄 `// Always explicitly close to catch errors from flush/close` / `Ar->Close();`, 828줄에서 오류 여부를 돌려준다. `Engine/Source/Runtime/Core/Private/HAL/FileManagerGeneric.cpp:110-117`은 열기에 실패하면 `MakeDirectory(*FPaths::GetPath(Filename), true);`로 상위 폴더를 만들고 다시 연다. pak 계층은 pak 안에 있는 파일만 거부하고 나머지는 아래 계층으로 넘긴다: `Engine/Source/Runtime/PakFile/Private/IPlatformFilePak.cpp:6732-6740` `// No modifications allowed on pak files.` … `return LowerLevel->OpenWrite(Filename, bAppend, bAllowRead);`. Windows 계층은 그대로 `CreateFileW(*FNormalizedFilename(Filename), ..., CREATE_ALWAYS, ...)`를 부른다(`Engine/Source/Runtime/Core/Private/Windows/WindowsPlatformFile.cpp:1644-1646`). 이 경로에 Shipping 조건은 없다.

이 프로젝트에 주는 뜻

- 결과 파일 위치는 스크립트가 정해 절대 경로로 넘긴다: `-RankT11Out=<저장소>\Saved\T11\shipping_results.json` [제안]. `Saved/`는 `.gitignore` 72줄 `Saved/*`로 커밋되지 않는다.
- 실행 파일은 상대 경로를 받지 않는다(`FPaths::IsRelative`이면 인자 오류). 상대 경로가 실행 파일 폴더 기준으로 풀려 엉뚱한 곳에 쓰는 일을 막는다 [판단].
- `ProjectDir()`(스테이징 폴더) 안에 쓸 수도 있지만 패키지 구조에 묶이고, 패키지를 다시 만들 때 함께 지워지거나 남을 수 있다 [추측]. 쓰지 않는다.
- `FFileHelper::SaveStringToFile`이 true를 돌려주면 파일은 이미 닫혔다. 그 뒤에 강제 종료(Q4)해도 결과 파일은 남는다 [판단].

남은 [미확인]

- `ProjectDir()`가 1609-1611줄(프로젝트 파일 경로가 설정된 경우)과 1638-1639줄 가운데 어느 분기를 타는지 [추측]. 두 분기 모두 같은 `RankAndFile/` 폴더를 가리킬 것으로 보이고, 설계안은 이 값을 쓰지 않으므로 T11에는 영향이 없다.
- 실제 패키지에서 절대 경로 쓰기가 되는지는 실행해 보지 않았다. 확인 방법: 1절과 같은 시험 실행.

## 3. Q3 로그

결론: Shipping(과 Test) 구성에서는 `NO_LOGGING`이 1이라 `UE_LOG`, `UE_LOGF`, `UE_LOGFMT`가 Fatal 말고는 컴파일되지 않는다. 켜는 설정은 `TargetRules.bUseLoggingInShipping`이고 기본값은 false다. 이 프로젝트의 `RankAndFile.Target.cs`는 이 값을 정하지 않아 false다. 게다가 이 속성은 고유 빌드 환경(unique build environment)을 요구하는데, 설치형 엔진은 고유 빌드 환경을 받지 않는다. 그래서 T11은 로그 없이 결과 파일과 종료 코드만으로 판정되게 짜야 한다.

근거

- `Engine/Source/Runtime/Core/Public/Misc/Build.h:200-203` `/** If not specified, disable logging in shipping */` / `#define USE_LOGGING_IN_SHIPPING 0`. Shipping 구간 350-352줄 `#define NO_LOGGING !USE_LOGGING_IN_SHIPPING`(Test 구간 322-324줄도 같다).
- `Engine/Source/Runtime/Core/Public/Logging/LogMacros.h:184-194` `#if NO_LOGGING` / `// This will only log Fatal errors`. Fatal이면 `LowLevelFatalError`를 부른다.
- `UE_LOGFMT`: `Engine/Source/Runtime/Core/Public/Logging/StructuredLog.h:551-555`에서 `NO_LOGGING`이면 범주가 `::UE::Logging::Private::LogFatal`로 바뀌고, 그 범주의 컴파일 시점 상세도는 Fatal이다(520줄 `FLogCategory<ELogVerbosity::Fatal, ELogVerbosity::Fatal> LogFatal`). 557-563줄의 `if constexpr` 조건 때문에 Fatal이 아닌 기록은 컴파일되지 않는다 [판단].
- 설정의 이름과 기본값: `Engine/Source/Programs/UnrealBuildTool/Configuration/Rules/TargetRules.cs:1607-1611` `/// Whether to turn on logging for test/shipping builds.` / `[RequiresUniqueBuildEnvironment]` / `public bool bUseLoggingInShipping { get; set; }`(초기값이 없는 C# bool이라 false). 정의로 옮기는 곳은 `Engine/Source/Programs/UnrealBuildTool/Configuration/UEBuildTarget.cs:6459-6466`(`USE_LOGGING_IN_SHIPPING=1` 또는 `=0`).
- 이 프로젝트의 값: `Source/RankAndFile.Target.cs:7-13`에 `bUseLoggingInShipping`이 없다. 그래서 false, `NO_LOGGING`은 1이다.
- 설치형 엔진의 제약: 설치형이면 빌드 환경이 공유(Shared)로 정해진다(`TargetRules.cs:2859-2861` `else if (Unreal.IsEngineInstalled() || LinkType != TargetLinkType.Monolithic)` → `return TargetBuildEnvironment.Shared;`). 공유 환경에서 `[RequiresUniqueBuildEnvironment]` 속성을 바꾸면 빌드가 멈춘다(`UEBuildTarget.cs:1515` `"{0} modifies the values of properties: [ {1} ]. This is not allowed, ..."`). 고유 환경으로 바꾸면 설치형 엔진에서 멈춘다(`Engine/Source/Programs/UnrealBuildTool/Configuration/Rules/RulesAssembly.cs:710-713` `"Targets with a unique build environment cannot be built with an installed engine."`). 설치형인지는 `InstalledBuild.txt`로 판정한다(`Engine/Source/Programs/Shared/EpicGames.Build/Unreal.cs:258`). 이 엔진에는 그 파일이 있다. 검사를 끄는 `bOverrideBuildEnvironment`(`TargetRules.cs:2883`)도 있지만, 미리 빌드된 엔진 바이너리는 그대로 둔 채 정의만 바뀌므로 권하지 않는다 [추측].
- 함께 알아 둘 것: `Build.h:205-207` `#define USE_CHECKS_IN_SHIPPING 0`, Shipping 구간 329-331줄 `#define DO_CHECK USE_CHECKS_IN_SHIPPING`. Shipping에서는 `check()` 안의 식이 평가되지 않는다. 지금 `Source/RankCore`와 `Source/RankGame`에는 `check`, `checkf`, `verify`, `ensure`, `UE_LOG`, `UE_LOGFMT`가 하나도 없다(grep). 그래서 지금은 영향이 없다 [판단].

이 프로젝트에 주는 뜻

- `Target.cs`는 바꾸지 않는다. 바꾸려 해도 설치형 엔진에서는 빌드가 막힌다.
- 결과 파일만으로 판정하려면 아래가 필요하다 [제안].
  1. 결과 파일에 상태(`status`)와 오류 문장(`error`)을 쓴다. 입력을 읽지 못했을 때도 출력 경로에 쓸 수 있으면 결과 파일을 남긴다. 로그가 없으니 실패 이유가 남는 곳은 여기뿐이다.
  2. 빌드 구성(`config`)을 적는다(`UE_BUILD_SHIPPING` 매크로로 정한다). Development 패키지로 잘못 돌린 결과를 T11로 받지 않게 한다.
  3. 실제로 쓴 병과 수치와 규칙 매개변수(`defs`), 대전마다의 입력 행(`ally`, `enemy`)을 적는다. 스크립트가 기록 파일과 같은 입력이었는지 본다(T1과 같은 검사, `Source/RankTests/Private/RankRegressionTest.cpp:57`, `83-86`).
  4. 스크립트는 실행 전에 이전 결과 파일을 지운다. 지난 실행의 파일을 이번 결과로 읽지 않게 한다.
  5. 종료 코드는 보조로만 쓴다. Q4에서 보듯 종료 코드 0은 "게임이 정상 종료됨"일 수도 있다.
- [제안] `Source/RankCore/AGENTS.md`의 결정론 점검표에 "Shipping에서는 `check()` 안의 식이 평가되지 않는다. 부수 효과가 있는 식을 `check`에 넣지 않는다"를 더할지 사람이 정한다(근거 `Build.h:329-331`).

남은 [미확인]: 없다.

## 4. Q4 종료 코드와 일을 할 시점

결론

- 5.8에도 `FPlatformMisc::RequestExitWithStatus(bool Force, uint8 ReturnCode, const TCHAR* CallSite)`가 있다. Windows에서 `Force = true`면 로그를 비운 뒤 `TerminateProcess(GetCurrentProcess(), ReturnCode)`로 끝나므로, 그 값이 프로세스 종료 코드가 된다. `Force = false`면 `PostQuitMessage(ReturnCode)`만 하는데, 메인 루프는 이 값을 쓰지 않는다. 정상 종료의 종료 코드는 엔진 초기화의 결과(보통 0)다 [판단].
- 크래시하면 엔진이 3으로 끝낸다 [판단]. 따라서 종료 코드 0만 보고 통과로 판정하면 안 된다.
- 일을 할 시점: `RankGame` 모듈의 `StartupModule`을 권한다 [제안]. 명령줄과 pak 파일 계층이 이미 준비되어 있고, 게임 창과 사용자 설정을 건드리기 전이다. 대안은 `FCoreDelegates::GetOnPostEngineInit()`이다. 5.8에서는 `OnPostEngineInit` 변수가 폐기 예정으로 바뀌었다.

근거: 종료 코드

- 선언: `Engine/Source/Runtime/Core/Public/GenericPlatform/GenericPlatformMisc.h:1088-1096` `@param ReturnCode This value will be returned from the program (on the platforms where it's possible). Limited to 0-255 to conform with POSIX.`
- Windows 구현: `Engine/Source/Runtime/Core/Private/Windows/WindowsPlatformMisc.cpp:1474-1523`. 1486줄 `RequestEngineExit(TEXT("Win RequestExit"));`, `Force`일 때 1502-1505줄 `GLog->Flush();`, 1515줄 `TerminateProcess(GetCurrentProcess(), ReturnCode);`. `Force`가 아니면 1517-1521줄 `// Tell the platform specific code we want to exit cleanly from the main loop.` / `PostQuitMessage(ReturnCode);`.
- 강제 종료 앞에서 예외를 던지는 설정: 1507-1513줄은 `WindowsPlatform.RaiseExceptionOnFaultTermination`이 참이고 코드가 0이 아니면 `RaiseException(1)`을 부른다. 기본값은 false다(140-144줄 `TEXT("WindowsPlatform.RaiseExceptionOnFaultTermination"),` / `false,`).
- 정상 종료 경로의 종료 코드: `Engine/Source/Runtime/Launch/Private/Launch.cpp:146` `int32 ErrorLevel = EnginePreInit( CmdLine );`, 172줄 `ErrorLevel = EngineInit();`, 190-193줄 메인 루프, 204줄 `return ErrorLevel;`. 이 값이 `LaunchWindows.cpp:277`, `300`을 거쳐 `WinMain`의 반환값이 된다(338-341줄). Runtime 소스 전체에서 `WM_QUIT`을 다루는 곳은 `Engine/Source/Runtime/ApplicationCore/Private/Windows/WindowsConsoleOutputDevice2.cpp:397`과 `Engine/Source/Runtime/ApplicationCore/Private/Windows/WindowsPlatformSplash.cpp:644` 두 곳뿐이다. 그래서 `PostQuitMessage`의 코드는 프로세스 종료 코드로 이어지지 않는다 [판단].
- 엔진 안의 같은 쓰임: 커맨들릿을 빨리 끝낼 때 `LaunchEngineLoop.cpp:4255-4258` `FPlatformMisc::RequestExitWithStatus(true, ErrorLevel);`를 쓴다.
- 크래시: `LaunchWindows.cpp:284-293` `ErrorLevel = 1;` … `GError->HandleError();` … `FPlatformMisc::RequestExit( true, ...)`. `Engine/Source/Runtime/Core/Private/Windows/WindowsErrorOutputDevice.cpp:105-107`의 `HandleError`가 `GIsCriticalError = 1;`로 두고, `WindowsPlatformMisc.cpp:1464-1466`이 `RequestExitWithStatus(Force, GIsCriticalError ? 3 : 0, CallSite);`를 부른다. 그래서 크래시는 3으로 끝난다 [판단: 오류 출력 장치가 `FWindowsErrorOutputDevice`일 때]. `-unattended`면 `LaunchWindows.cpp:211-214`에서 `SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | ...)`로 오류 대화 상자를 끈다.

근거: 시점(Shipping 게임의 시작 순서)

| 순서 | 일 | 위치 |
|---|---|---|
| 1 | 작업 디렉터리를 실행 파일 폴더로 | `LaunchEngineLoop.cpp:1801-1803` |
| 2 | 명령줄 설정 | `LaunchEngineLoop.cpp:1808` `FCommandLine::Set(CmdLine)` |
| 3 | 파일 계층(pak) 설정 | `LaunchEngineLoop.cpp:2134-2135` `LaunchCheckForFileOverride`, 943-959줄 `ConditionallyCreateFileWrapper(TEXT("PakFile"), ...)` → `FPlatformFileManager::Get().SetPlatformFile(*CurrentPlatformFile);` |
| 4 | RHI 초기화 | `LaunchEngineLoop.cpp:3168-3171` `RHIInit(bHasEditorToken);` |
| 5 | 시작 모듈 불러오기(`RankGame`은 `.uproject`에서 `LoadingPhase: Default`) | `LaunchEngineLoop.cpp:3894-3896` `LoadStartupModules()`, 4682줄 `LoadModulesForProject(ELoadingPhase::Default)` → `StartupModule` |
| 6 | 엔진 초기화 | `LaunchEngineLoop.cpp:4826-4829` `GEngine->Init(this);` |
| 6-1 | 사용자 설정 불러오기 | `Engine/Source/Runtime/Engine/Private/GameEngine.cpp:1249` `GetGameUserSettings()->LoadSettings();` |
| 6-2 | 게임 인스턴스 만들기와 `UGameInstance::Init` | `GameEngine.cpp:1256-1267`(클래스는 `UGameMapsSettings`의 `GameInstanceClass`), `Engine/Source/Runtime/Engine/Private/GameInstance.cpp:190`, `201` `Init();` |
| 6-3 | 게임 창 만들기 | `GameEngine.cpp:1307` `GameViewportWindow = CreateGameWindow();` |
| 7 | 엔진 초기화 뒤 콜백 | `LaunchEngineLoop.cpp:4832-4837` `FCoreDelegates::GetOnPostEngineInit().Broadcast();` |
| 8 | 게임 시작 | `LaunchEngineLoop.cpp:4879` `GEngine->Start();` → `GameEngine.cpp:1333-1338` `GameInstance->StartGameInstance();` |

- `StartupModule`은 모듈을 불러온 뒤 부른다(`Engine/Source/Runtime/Core/Public/Modules/ModuleInterface.h:40-49`). 지금 이 프로젝트는 아무것도 하지 않는 `FDefaultGameModuleImpl`을 쓴다(`Engine/Source/Runtime/Core/Public/Modules/ModuleManager.h:892-903`, `Source/RankGame/Private/RankGameModule.cpp:4`). 모놀리식 데스크톱 빌드의 `IMPLEMENT_PRIMARY_GAME_MODULE`은 첫 인자로 받은 클래스를 그대로 모듈로 등록한다(`ModuleManager.h:1100-1111` `IMPLEMENT_GAME_MODULE( ModuleImplClass, ModuleName )`).
- 5.8의 이름: `Engine/Source/Runtime/Core/Public/Misc/CoreDelegates.h:236-241` `static CORE_API FSimpleMulticastDelegate& GetOnPostEngineInit();` / `UE_DEPRECATED(5.8, "OnPostEngineInit has been deprecated. Please use GetOnPostEngineInit() instead.")`. 기억에 있는 `FCoreDelegates::OnPostEngineInit`는 5.8에서 쓰면 폐기 경고가 난다.
- 그 시점의 파일 읽기·쓰기: pak 계층(3번)이 시작 모듈(5번)보다 먼저 설정되므로 `StartupModule`에서도 `FFileHelper`로 읽고 쓸 수 있다 [판단: 순서로 판단했다. 실행해 보지 않았다].
- RHI는 어느 시점이든 이미 초기화된 뒤다(4번이 5번보다 앞). `-nullrhi`를 주면 렌더링 장치를 만들지 않는다: `Engine/Source/Runtime/Core/Public/Misc/App.h:405-406` `FParse::Param(FCommandLine::Get(), TEXT("nullrhi"))` … `!(USE_NULL_RHI || bHasNullRHIOnCommandline)`, `Engine/Source/Runtime/RHI/Private/DynamicRHI.cpp:304-306` `if (!FApp::CanEverRender())` / `InitNullRHI();`. 두 곳 모두 Shipping 조건이 없다. `-nosplash`는 `WindowsPlatformSplash.cpp:706`이 본다.

시점 비교

| 시점 | 파일 입출력 | 게임 창 | 설정 변경 필요 | 판단 |
|---|---|---|---|---|
| `RankGame`의 `StartupModule` | 된다(3번 뒤) [판단] | 만들기 전(6-3번 앞) | 없음. `RankGameModule.cpp`의 모듈 클래스만 바뀐다 | 권장 [제안] |
| `UGameInstance::Init` | 된다 | 만들기 전 | `UGameMapsSettings`의 `GameInstanceClass`에 C++ 게임 인스턴스 클래스를 지정해야 한다. 이 설정은 `Config/DefaultEngine.ini`에 간다(`Engine/Source/Runtime/EngineSettings/Classes/GameMapsSettings.h:100-101` `UCLASS(config=Engine, defaultconfig, MinimalAPI)`) | 런 진행 클래스에 T11이 묶인다. 권하지 않는다 |
| `FCoreDelegates::GetOnPostEngineInit()` | 된다 | 만든 뒤(창이 잠깐 뜬다) | 없음. `StartupModule`에서 등록 | 대안. 엔진이 다 준비된 뒤라 `StartupModule`에서 문제가 나면 이쪽으로 옮긴다 |

이 프로젝트에 주는 뜻

- 0이 아닌 종료 코드를 내려면 반드시 `Force = true`로 부른다. `Force = false`로는 0으로 끝난다 [판단].
- 엔진이 쓰는 코드(1: 실행 중 오류 값, 3: 크래시 뒤 강제 종료)와 겹치지 않는 값을 T11 실패 코드로 쓴다(6.4절).
- 인자가 무시되면 게임이 그대로 떠서 끝나지 않는다. 스크립트는 시간 제한을 두고, 시간을 넘기면 프로세스를 끝내고 실패로 처리한다.

남은 [미확인]

- `StartupModule` 시점에서 대전 20건을 돌리고 강제 종료했을 때 결과 파일과 종료 코드가 기대대로 남는지. 확인 방법: 6.1절 코드를 넣은 Shipping 패키지에서 정상 실행 1번, 일부러 틀린 인자(상대 경로) 1번을 돌려 종료 코드와 결과 파일을 본다.
- `-nullrhi`로 Shipping 패키지가 `StartupModule`까지 가는지. 확인 방법: 위 시험을 `-nullrhi`를 붙여 한 번, 빼고 한 번 돌린다. 안 되면 `-nullrhi`를 빼고 쓴다(README의 2026-11-20 기록대로 창 모드 실행은 된다).
- 스크립트(배치 파일 `start "" /wait`, 또는 Node `child_process.spawnSync`)가 창 프로그램(`WinMain`, `LaunchWindows.cpp:317`)의 종료를 기다리고 종료 코드를 받는지. 엔진 밖 동작이다. 확인 방법: 위 시험에서 스크립트가 받은 코드를 출력한다.
- 스테이징 루트의 `RankAndFile.exe`(부트스트랩)가 인자와 종료 코드를 넘기는지. 소스가 설치형 엔진에 없다. 확인하기 전에는 이것을 쓰지 않고 `RankAndFile/Binaries/Win64/RankAndFile-Win64-Shipping.exe`를 직접 부른다(지금 스테이징 폴더에 이 파일이 있다).

## 5. Q5 입력 파일의 패키징

결론: 비UFS 파일을 스테이징하는 프로젝트 설정의 이름은 `DirectoriesToAlwaysStageAsNonUFS`다(`[/Script/UnrealEd.ProjectPackagingSettings]`, `Config/DefaultGame.ini`). 그런데 경로가 프로젝트 Content 폴더 기준이라 Content 밖의 `Source/RankTests/Fixtures`와 `Data/defs`를 가리키려면 `..`이 든 경로가 필요하다. 그 동작은 확인하지 못했다. 다른 방법(명령줄로 저장소 안의 절대 경로를 넘겨 읽기)은 엔진이 막지 않는다. Shipping의 pak 계층은 pak에 없는 `.csv`를 아래 계층에서 그대로 읽는다. 명령줄 방식을 권한다 [판단].

근거: 스테이징 설정

- `Engine/Source/Developer/DeveloperToolSettings/Classes/Settings/ProjectPackagingSettings.h:178` `UCLASS(MinimalAPI, config=Game, defaultconfig)`. 593-599줄 `* Note: These paths are relative to your project Content directory` / `TArray<FDirectoryPath> DirectoriesToAlwaysStageAsNonUFS;`. pak에 넣는 판은 585-591줄 `DirectoriesToAlwaysStageAsUFS`다.
- 스테이징: `Engine/Source/Programs/AutomationTool/Scripts/CopyBuildToStagingDirectory.Automation.cs:2051-2056` `// Stage any additional UFS and NonUFS paths specified in the project ini files; these dirs are relative to the game content directory`. 설정 구역은 820줄 `"/Script/UnrealEd.ProjectPackagingSettings"`, 원본은 837줄 `DirectoryReference.Combine(ProjectContentRoot, RelativePath)`, 놓을 곳은 843줄 `StagedDirectoryReference.Combine(StageContentRoot, RelativePath)`, 비UFS로 놓는 것은 852줄 `SC.StageFiles(StagedFileType.NonUFS, ...)`.
- 모듈 단위의 다른 방법: `Engine/Source/Programs/UnrealBuildTool/Configuration/Rules/ModuleRules.cs:1421-1424` `/// List of files which this module depends on at runtime. These files will be staged along with the target.` / `public RuntimeDependencyList RuntimeDependencies = new();`. `Add(string inPath, StagedFileType inType)`는 397줄, `Add(string inPath, string inSourcePath, StagedFileType inType = StagedFileType.NonUFS)`는 408줄. 종류는 `Engine/Source/Programs/UnrealBuildTool/System/TargetReceipt.cs:116-137`의 `StagedFileType`(`UFS`, `NonUFS`, `DebugNonUFS`, `SystemNonUFS`)이다.

근거: 명령줄로 저장소 경로를 넘겨 읽기

- 읽기는 `FFileHelper::LoadFileToString` → `IFileManager::Get().CreateFileReader`다(`FileHelper.cpp:242-258`). `RankContent::LoadDefsFiles`(`Source/RankGame/Private/RankContent.cpp:380`, `385`)와 커맨들릿의 입력 읽기(`Source/RankEditor/Private/RankRegressionCommandlet.cpp:599`)가 이 함수를 쓴다.
- Shipping의 pak 계층 읽기: `IPlatformFilePak.cpp:6690-6730`. 느슨한 파일(loose file)을 먼저 보는 6694-6703줄은 `#if !UE_BUILD_SHIPPING` 안이라 Shipping에서는 빠진다. pak 안에서 먼저 찾고(6706-6720줄), 없으면 6723-6726줄 `if (IsNonPakFilenameAllowed(Filename))` / `Result = LowerLevel->OpenRead(Filename, bAllowWrite);`로 아래 계층에서 읽는다.
- pak 밖 읽기를 막는 조건(`IsNonPakFilenameAllowed`, 4572-4640줄):
  - 확장자 검사는 Shipping에서 늘 한다(4576-4590줄, `EXCLUDE_NONPAK_UE_EXTENSIONS`는 `Engine/Source/Runtime/PakFile/Public/IPlatformFilePak.h:60-65`에서 `(!WITH_EDITOR)`). 막는 확장자는 5706-5715줄의 `uasset`, `umap`, `ubulk`, `uexp`, `uptnl`, `ushaderbytecode`, `metalmap`뿐이다. `csv`와 `json`은 없다.
  - `.ini` 제한(4592-4599줄)은 `DISABLE_NONUFS_INI_WHEN_COOKED`일 때만인데, 이 값은 0이다(`IPlatformFilePak.cpp:75`). 어느 쪽이든 `.csv`와는 무관하다.
  - 파일 이름 보안 델리게이트(4630-4636줄 `if (FilenameSecurityDelegate.IsBound())`)는 묶였을 때만 걸린다. `Engine/Source` 안에서 `GetFilenameSecurityDelegate()`를 묶는 코드는 `IPlatformFilePak` 밖에 없다(grep. `Engine/Plugins`는 찾지 않았다).

방법 비교

| | A. 명령줄로 저장소 절대 경로 | B. `DirectoriesToAlwaysStageAsNonUFS` | C. `RuntimeDependencies` |
|---|---|---|---|
| 바꿀 파일 | 없음(스크립트 인자) | `Config/DefaultGame.ini` | `Source/RankGame/RankGame.Build.cs` |
| 읽는 파일 | 에디터 커맨들릿·T1과 같은 저장소 파일 | 패키지를 만들 때 복사한 사본 | 패키지를 만들 때 복사한 사본 |
| 경로 기준 | 스크립트가 정한다 | Content 폴더 기준. Fixtures와 `Data/defs`는 Content 밖이라 `..`이 필요하다 [미확인] | 원본을 프로젝트 기준으로 줄 수 있다. 놓이는 곳은 [미확인] |
| 배포 패키지에 | 들어가지 않는다 | 시험 입력이 들어간다 | Runtime 모듈이라 모든 패키지에 들어간다 |
| 패키지 뒤에 입력·기록이 바뀌면 | 늘 지금 파일을 읽는다 | 낡은 사본과 비교하게 된다 | 같음 |
| 저장소 없는 PC | 돌지 않는다 | 패키지만으로 돈다 | 패키지만으로 돈다 |
| 규칙상 절차 | 없음 | 설정 변경: 루트 `AGENTS.md` 4절의 `tools\package.bat` 확인 | `Source/AGENTS.md` 15줄(의존·빌드 파일 변경은 알린다), 59줄(`tools\package.bat`) |

이 프로젝트에 주는 뜻

- A를 권한다 [판단]. T11이 보려는 것은 빌드 구성에 따른 차이 하나다. 에디터 기록과 바이트까지 같은 입력을, 같은 함수(`RankRegression::ParseInputsCsv`, `RankContent::LoadDefsFiles`)로 읽어야 어긋났을 때 원인을 코어로 좁힐 수 있다(`Source/RankGame/AGENTS.md:61`). 에디터가 없는 다른 PC에서 실행하는 확인은 `docs/design.md` 9.1절의 수동 확인이 맡고, T11의 몫이 아니다.
- A에서는 저장소 루트를 `-RankT11Root=`로 넘기고, 그 아래 상대 경로는 커맨들릿과 같은 상수를 쓴다(6.2절).
- 게임 본편이 Shipping에서 병과 수치를 어디서 읽는지(쿠킹된 DataTable)는 T11이 보지 않는다. 그것은 T8과 `docs/design.md` 14절의 "DataTable 에셋이 쿠킹에 들어가는지" 확인이 맡는다.

남은 [미확인]

- B와 C에서 Content 밖 파일이 스테이징 폴더 어디에 놓이는지(B는 `..`이 든 경로의 처리). A를 쓰면 확인할 필요가 없다. 확인하려면 설정을 바꾼 패키지의 `Manifest_NonUFSFiles_Win64.txt`를 본다(사람의 승인 뒤).

## 6. 설계안 [제안]

코드는 쓰지 않았다. 아래는 문장과 함수 이름만으로 적은 안이다. 이름은 모두 [제안]이다.

### 6.1 어디에 무엇을 두는가 (`RankGame`)

| 파일 | 둘 것 |
|---|---|
| `Source/RankGame/Private/RankGameModule.cpp` | `FDefaultGameModuleImpl`을 상속한 모듈 클래스 `FRankGameModule`. `StartupModule`에서 `#if !WITH_EDITOR` 안에서만 `RankShippingCheck::IsRequested(FCommandLine::Get())`를 본다. 참이면 `RankShippingCheck::Run`을 부르고, 돌려받은 코드로 `FPlatformMisc::RequestExitWithStatus(true, 코드, TEXT("RankShippingCheck"))`를 부른다. `IMPLEMENT_PRIMARY_GAME_MODULE`의 첫 인자만 바뀐다 |
| `Source/RankGame/Private/RankShippingCheck.h`, `.cpp`(새 파일, 다른 모듈이 쓰지 않으므로 `Private/`) | `IsRequested`(`-RankT11`이 있는지), `Run`(인자 읽기 → 병과 수치 읽기 → 입력 읽기 → 대전마다 `Rank::Simulate` → `RankRegression::MakeRecord` → 결과 파일 쓰기 → 종료 코드 돌려주기). 결과를 계산하는 코드는 두지 않는다 |
| `Source/RankGame/Public/RankRegressionData.h`, `Private/RankRegressionData.cpp` | 결과 파일의 글을 만드는 `WriteShippingResultJson`, 이벤트 로그의 글을 만드는 `WriteEventLogCsv`, (선택) 6.6절의 `MakeBattleInput` |

- `Run`은 에디터 빌드에서도 컴파일되게 둔다. 컴파일 오류를 패키지 때가 아니라 에디터 빌드에서 잡는다. 부르는 곳(`StartupModule`의 그 부분)만 `#if !WITH_EDITOR`로 막는다. 그러면 에디터, `tools\run_tests.bat`, RankRegression 커맨들릿에서는 이 경로가 아예 없다.
- `Source/RankGame/AGENTS.md` 58-61줄에 맞는다: 코드가 `RankGame`에 있고, 정해진 인자가 있을 때만 동작하고, `Rank::Simulate`만 부르고, 입력 CSV는 `RankRegression::ParseInputsCsv` 하나로 읽는다.
- `RankGame.Build.cs`의 의존은 늘지 않는다. `FParse`, `FPaths`, `FFileHelper`는 `Core`에 있다.

### 6.2 실행 인자

| 인자 | 뜻 | 없으면 |
|---|---|---|
| `-RankT11` | T11 실행을 켠다 | 게임이 평소대로 뜬다(6.5절) |
| `-RankT11Root=<저장소 루트의 절대 경로>` | 입력 위치. 그 아래 상대 경로는 커맨들릿과 같은 상수 `Source/RankTests/Fixtures/regression_inputs.csv`, `Data/defs/units.csv`, `Data/defs/rules.csv`(`RankRegressionCommandlet.cpp:21`, `591`) | 인자 오류(코드 10) |
| `-RankT11Out=<결과 파일의 절대 경로>` | 결과 파일. 스크립트는 `<저장소>\Saved\T11\shipping_results.json`을 넘긴다 | 인자 오류(코드 10) |
| `-RankT11DumpLog=<대전 id>` | 그 대전의 이벤트 로그를 결과 파일과 같은 폴더의 `events_<id>_shipping.csv`에 쓴다 | 쓰지 않는다 |
| 엔진 인자 `-nullrhi -nosplash -unattended` | 렌더링 장치 없이, 시작 화면 없이, 오류 대화 상자 없이 | `-nullrhi`는 4절 [미확인] |

- 경로 값은 `FParse::Value(..., false)`로 읽고, `FPaths::IsRelative`면 인자 오류로 끝낸다(1절, 2절).
- 저장소 루트를 하나로 받는 이유: 상대 경로를 커맨들릿과 같은 상수로 두면, 두 쪽이 같은 파일을 읽는다는 것이 코드에 드러난다. 이 상수들을 `RankRegressionData.h` 한 곳으로 옮겨 커맨들릿(`RankRegressionCommandlet.cpp:21-23`, `591`)과 T1(`Source/RankTests/Private/RankRegressionTest.cpp:25-26`, `Source/RankTests/Private/RankTestUtils.cpp:22-23`)도 함께 쓰게 하는 것도 [제안]이다. 이 변경은 `RankEditor`와 `RankTests`도 건드린다.
- 스크립트는 저장소 루트를 `package.bat` 25줄과 같은 방식(`%~dp0..`을 `%%~fI`로 펼친다)으로 구해 끝에 `\`가 붙지 않게 넘긴다. `"...\"`처럼 따옴표 앞에 `\`가 오면 Windows의 인자 분리 규칙상 따옴표가 글자로 바뀐다 [추측: 엔진 밖 동작, 첫 실행에서 확인].

### 6.3 출력 파일 형식

JSON, UTF-8(BOM 없음), 줄바꿈 LF, 키 순서 고정. 날짜, 시각, 경로, 실행 환경은 넣지 않는다(`tools/AGENTS.md` 32줄의 기록 파일 규칙과 같게). 쓰기는 `FFileHelper::SaveStringToFile(..., FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)`로 한다(커맨들릿 `RankRegressionCommandlet.cpp:804`와 같다).

맨 위 키

| 키 | 값 | 출처 |
|---|---|---|
| `kind` | `"rank-t11-shipping-result"` 고정. 기록 파일과 헷갈리지 않게 한다 | 상수 |
| `version` | 1 | 상수 |
| `config` | `"Shipping"`, `"Test"`, `"Development"` 가운데 하나 | `UE_BUILD_SHIPPING` 등 빌드 매크로(`Build.h:241`, `269`, `297`, `325`) |
| `status` | `"ok"` 또는 `"error"` | 20건이 모두 돌고 기록 항목을 만들었으면 `ok` |
| `error` | 실패 이유. `ok`면 빈 문자열 | `ParseInputsCsv`, `LoadDefsFiles`가 돌려준 `OutError`, 또는 실패한 대전 id |
| `defs` | 기록 파일의 `defs`와 같은 형식(`units` 배열, `rules` 객체) | `LoadDefsFiles`가 돌려준 행 구조체. 형식은 `RankRegressionData.cpp:301-316` |
| `battles` | 대전 배열(아래) | |

대전마다의 키는 기록 파일의 키와 이름·표기가 같다(`Source/RankGame/Private/RankRegressionData.cpp:323-334`): `id`, `ally`, `enemy`, `outcome`, `tick`, `events`, `hash`, `gate`, `barrier`, `alive`, `hpSum`, `logChecksum`. `outcome`은 `RankRegression::OutcomeName`, `hash`와 `logChecksum`은 `RankRegression::HexU32`(소문자 16진수 8자리)로 적는다. 기록 파일에 있는 `branches`는 넣지 않는다. 갈래 확인 코드(`RankRegressionCommandlet.cpp:212-503`)는 에디터 모듈에 있어 Shipping에 없고, T11의 비교 항목도 아니다.

형식을 만드는 방법 두 가지 [미정: 사람이 고른다]

- (가) 권장 [판단]: `WriteRecordJson`(`RankRegressionData.cpp:294-341`) 안의 "defs 쓰기"와 "대전 하나 쓰기"를 함수로 떼어 내고, `WriteRecordJson`과 새 `WriteShippingResultJson`이 함께 부른다. 키 이름과 표기가 한 코드에서 나온다. 맨 위 키(`kind`, `status` 등)가 달라 이 파일을 `ReadRecordJson`이 기록으로 읽지 않는다. 떼어 낸 뒤에도 기록 파일이 바이트까지 같아야 한다. 커맨들릿의 자체 점검(`RankRegressionCommandlet.cpp:757-781`, CRC32 출력)을 고치기 전과 뒤에 돌려 CRC가 같은지 본다.
- (나) `WriteRecordJson`을 그대로 쓰고 `Commit`을 비우고 `Branches`를 빈 배열로 둔다. 코드가 가장 적다. 하지만 결과 파일이 `ReadRecordJson`으로 기록처럼 읽힌다. "T11의 Shipping 결과로 기록 파일을 만들거나 고치지 않는다"(`Source/RankTests/AGENTS.md:76`)를 실수로 어길 길이 생긴다.

이벤트 로그(`-RankT11DumpLog`): 첫 줄 `tick,type,actor,target,value`, 이어서 로그 한 줄에 한 이벤트. 로그 체크섬이 섞는 다섯 칸과 같은 순서다(`RankRegressionData.cpp:87-99`). 에디터 쪽에서 같은 형식의 파일을 내려면 RankRegression 커맨들릿에 같은 함수를 부르는 인자(예: `-dumplog=<id>`, 기록은 하지 않음)를 더해야 한다. `RankEditor`를 바꾸는 일이라 따로 승인받는다. `Source/RankTests/AGENTS.md` 90줄의 "에디터와 Shipping 패키지의 이벤트 로그를 나란히 놓고"에 필요하다.

### 6.4 종료 코드

| 코드 | 뜻 | 결과 파일 |
|---|---|---|
| 0 | 20건을 모두 돌리고 결과 파일을 썼다. 기록과 같다는 뜻은 아니다(비교는 스크립트) | 있음, `status: ok` |
| 10 | 인자 오류: `-RankT11Root=`나 `-RankT11Out=`이 없거나 상대 경로 | 쓸 수 있으면 `status: error` |
| 11 | 입력 읽기 실패: `Data/defs` 또는 `regression_inputs.csv` | `status: error`, `error`에 `OutError` |
| 12 | 대전 실행 실패: 결과가 `Invalid`이거나 `MakeRecord`가 false(로그 줄 수와 이벤트 수가 다름) | `status: error`, `error`에 대전 id |
| 13 | 결과 파일 쓰기 실패(`SaveStringToFile`이 false) | 없음 |

- 엔진이 쓰는 코드와 겹치지 않게 골랐다. 1은 실행 중 오류 값(`LaunchWindows.cpp:286`), 3은 크래시 뒤의 강제 종료(`WindowsPlatformMisc.cpp:1466`)다. 0은 정상 종료와도 같으므로, 스크립트는 0을 받아도 결과 파일의 `status`와 대전 수를 함께 본다.
- 값은 0~255 안이다(`GenericPlatformMisc.h:1093`).

### 6.5 인자가 없을 때 게임 동작을 바꾸지 않는 방법

- `StartupModule`이 하는 일은 `FParse::Param(FCommandLine::Get(), TEXT("RankT11"))` 하나다. 거짓이면 바로 돌아간다. 델리게이트 등록, 파일 접근, 전역 상태 변경, 정적 초기화를 하지 않는다.
- 에디터 빌드에서는 그 부분이 컴파일되지 않는다(`#if !WITH_EDITOR`). 에디터가 모듈을 시작할 때 `GIsEditor`가 이미 참인지 확인하지 않아도 된다.
- Development, Test, Shipping 패키지에 모두 들어간다. 인자가 없으면 세 구성 모두 동작이 같다.
- 확인 방법(사람): 인자 없이 패키지를 띄워 README의 2026-11-20 기록과 같은지(창 모드로 뜨고, 닫았을 때 종료 코드 0) 본다.
- 플레이어가 `-RankT11`을 주더라도, 지정한 파일을 읽고 결과를 쓰고 끝날 뿐이다 [판단].

### 6.6 기존 `RankRegressionData` 함수의 재사용

| 함수 | T11에서 | 근거 |
|---|---|---|
| `RankRegression::ParseInputsCsv`(`RankRegressionData.h:22`) | 쓴다. 같은 읽기 코드여야 한다 | `Source/RankGame/AGENTS.md:61`, `Source/RankTests/AGENTS.md:74` |
| `RankContent::LoadDefsFiles`(`Source/RankGame/Public/RankContent.h:30-32`) | 쓴다. 커맨들릿(`RankRegressionCommandlet.cpp:591`)과 T1(`RankTestUtils.cpp:22-23`)이 쓰는 함수 | 같은 이유 |
| `Rank::Simulate(Input, true)`(`Source/RankCore/Public/RankBattle.h:19`) | 쓴다. 로그를 켠다 | `Source/RankGame/AGENTS.md:60` |
| `RankRegression::MakeRecord`(`RankRegressionData.h:43`, 로그 체크섬 포함) | 쓴다. 비교 항목을 옮겨 적는 함수가 T1, 커맨들릿, T11에 하나다 | 같은 이유 |
| `RankRegression::OutcomeName`, `HexU32` | 쓴다. 기록 파일과 같은 표기 | 6.3절 |
| `RankRegression::WriteRecordJson` | 6.3절 (가)면 안의 일부를 떼어 함께 쓴다. (나)면 그대로 쓴다 | 6.3절 |
| `RankRegression::ReadRecordJson`, `DiffRecords`, `DiffDefs` | 6.7절 비교를 스크립트에서 하면 쓰지 않는다. 실행 파일 안에서 비교하는 안(6.8절 2번)이면 쓴다 | |
| `FBattleInput`을 채우는 코드 | 지금 커맨들릿(`RankRegressionCommandlet.cpp:624-630`)과 T1(`RankRegressionTest.cpp:88-94`)에 같은 다섯 줄이 따로 있다. `RankRegression::MakeBattleInput(행, 병과 정의, 규칙)`으로 `RankGame`에 하나 두고 셋이 함께 쓰자 [제안] | 같은 이유(입력을 코어 입력으로 바꾸는 코드는 하나) |

### 6.7 `tools\check_shipping_determinism.bat`의 흐름

1. 저장소 루트를 구한다(`package.bat` 25줄과 같은 방식). 실행 파일은 `Saved\StagedBuilds\Windows\RankAndFile\Binaries\Win64\RankAndFile-Win64-Shipping.exe`, 기록 파일은 `Source\RankTests\Fixtures\regression_battles_v1.json`, 결과 파일은 `Saved\T11\shipping_results.json`이다. 부트스트랩 `RankAndFile.exe`(`package.bat` 33줄이 확인하는 파일)는 쓰지 않는다(4절 [미확인]).
2. 실행 파일이 없으면 "패키지가 없다. `tools\package.bat`을 먼저 돌린다"로 실패한다.
3. 기록 파일이 없으면 "기준 확정 전"으로 실패한다(`Source/RankTests/AGENTS.md:32`). 지금은 이 파일이 없으므로 여기서 실패한다.
4. 지난 결과 파일과 이벤트 로그 파일을 지운다.
5. 실행 파일을 `-RankT11 -RankT11Root="<루트>" -RankT11Out="<결과 파일>" -nullrhi -nosplash -unattended`로 띄우고, 시간 제한(예: 120초 [제안]) 안에 끝나기를 기다린다. 시간을 넘기면 프로세스를 끝내고 "시간 초과: 인자가 무시되어 게임이 떴을 수 있다"로 실패한다.
6. 종료 코드가 0이 아니면 6.4절 표의 뜻을 적는다. 결과 파일이 있으면 `error`도 적고 실패한다.
7. 결과 파일이 없으면 실패한다.
8. 비교(비교만 한다. 기록 파일을 만들거나 고치지 않는다, `tools/AGENTS.md:40`):
   1. `kind`, `version`이 맞는지, `config`가 `Shipping`인지, `status`가 `ok`인지.
   2. `defs`가 기록 파일의 `defs`와 같은지. 다르면 "기록이 지금 수치와 다르다"로 실패한다(T1과 같은 판정).
   3. 대전 수가 기록 파일의 대전 수와 같고 20인지(`tools/AGENTS.md:37`).
   4. 대전 id의 집합이 같은지, 대전마다 `ally`, `enemy`가 기록과 같은지.
   5. 대전마다 `outcome`, `tick`, `events`, `hash`, `gate`, `barrier`, `alive`, `hpSum`, `logChecksum`이 정확히 같은지. 다르면 대전 id, 항목, 기대값(기록), 실제값(Shipping)을 적는다.
9. 하나라도 다르면 0이 아닌 코드로 끝난다. 모두 같으면 `T11 20/20 · 명령: tools\check_shipping_determinism.bat · 빌드: Shipping · 커밋: <패키지 커밋>`(`Source/RankTests/AGENTS.md:103`의 형식)을 내고 0으로 끝난다.
10. 덤프 모드: `tools\check_shipping_determinism.bat -dump rb07`이면 5번에 `-RankT11DumpLog=rb07`을 더하고, 만든 `Saved\T11\events_rb07_shipping.csv`의 경로를 알린다.

- 5번과 8번을 배치 파일만으로 짜기는 어렵다(시간 제한, JSON 읽기). Node 내장 모듈만 쓰는 보조 스크립트(예: `tools/shipping_determinism.js`)를 두고 배치 파일이 부르는 안을 권한다 [제안]. 시간 제한은 `child_process.spawnSync`의 시간 제한 옵션, 비교는 `JSON.parse`로 한다 [미확인: Node 동작은 첫 실행에서 확인]. 이 저장소는 이미 Node로 검사를 돌린다(`tools/hand_crosscheck.js`, 루트 `AGENTS.md` 4절). PowerShell(`Start-Process -PassThru`와 `WaitForExit`, `ConvertFrom-Json`)로 하는 안도 있다.
- 패키지 커밋: `tools/AGENTS.md` 41줄은 보고에 "패키지를 만든 커밋"을 적으라고 한다. 지금 `package.bat`은 커밋을 남기지 않는다. `package.bat`이 끝날 때 `git rev-parse HEAD`와 커밋하지 않은 변경이 있는지를 `Saved\StagedBuilds\Windows\` 안의 작은 파일에 남기고, 이 스크립트가 그것을 읽어 적는 안을 낸다 [제안]. `package.bat`을 바꾸는 일이라 따로 승인받는다. 그 전까지는 "패키지 커밋: 기록 없음"으로 적는다.

### 6.8 사람이 정할 것 [미정]

1. 일을 할 시점: `StartupModule`(권장) 또는 `GetOnPostEngineInit()`.
2. 비교를 어디서 하는가: 스크립트(권장. `tools/AGENTS.md:36`의 "스크립트는 그 값을 … 비교한다"와 맞고, 시험받는 빌드와 따로 돈다) 또는 실행 파일 안에서 `ReadRecordJson`·`DiffRecords`·`DiffDefs`로(T1과 같은 비교 코드를 쓴다. 대신 비교 코드도 Shipping으로 돈다).
3. 출력 형식을 만드는 방법: 6.3절 (가)(권장) 또는 (나).
4. 보조 스크립트를 Node로 할지 PowerShell로 할지.
5. 함께 할 리팩터: 경로 상수와 `MakeBattleInput`을 `RankGame`으로 모으기(6.2절, 6.6절), 커맨들릿의 이벤트 로그 쓰기(6.3절), `package.bat`의 커밋 기록(6.7절). 셋 다 `RankEditor`, `RankTests`, `tools/`를 건드린다.

## 7. 수정안

### 7.1 `tools/AGENTS.md:42`

지금:

```
- [미확인] Shipping 실행 파일이 실행 인자를 읽는 방법, 결과 파일을 쓸 수 있는 위치, Shipping에서 로그가 꺼지는지, 끝날 때 종료 코드를 돌려주는 방법. 엔진 소스로 확인하고(`docs/design.md` 14.1절), 확인되면 이 표시를 지운다. 확인하기 전에 추측으로 짜지 않는다.
```

바꾼 뒤(42줄 한 줄을 아래 여섯 줄로):

```
- 엔진 소스로 확인한 것(UE 5.8.4, 근거와 파일:줄은 `docs/review/t11-shipping-investigation-2026-11-21.md`):
  - 실행 인자: Shipping에서도 `FCommandLine::Get()`, `FParse::Param`, `FParse::Value`가 그대로 동작한다. 인자를 거르는 허용 목록(`UE_COMMAND_LINE_USES_ALLOW_LIST`)은 기본값이 0이고 이 프로젝트는 켜지 않았다. 실행 파일은 시작할 때 작업 디렉터리를 실행 파일 폴더로 바꾸므로 경로 인자는 절대 경로로 넘긴다.
  - 결과 파일 위치: Shipping의 `FPaths::ProjectSavedDir()`는 사용자 폴더(로컬 AppData) 아래라 쓰지 않는다. 스크립트가 저장소 `Saved/T11/` 안의 절대 경로를 인자로 넘기고, 실행 파일은 그 경로에 쓴다. 입력 CSV와 `Data/defs`도 패키지에 넣지 않고 저장소의 파일을 절대 경로로 읽는다.
  - 로그: Shipping은 `NO_LOGGING`이라 `UE_LOG`, `UE_LOGFMT`가 Fatal 말고는 컴파일되지 않는다. 켜는 설정(`bUseLoggingInShipping`)은 설치형 엔진에서 쓸 수 없다. 판정은 결과 파일(상태, 오류 문장, 빌드 구성, 수치, 대전 20건)과 종료 코드로만 한다.
  - 종료 코드: `FPlatformMisc::RequestExitWithStatus(true, 코드)`가 Windows에서 그 코드로 프로세스를 끝낸다. `false`로는 종료 코드가 바뀌지 않는다. 엔진은 크래시하면 3으로, 정상 종료면 0으로 끝내므로 종료 코드 0만으로 통과로 보지 않는다. 인자가 무시되면 게임이 그대로 뜨므로 시간 제한을 둔다.
- [미확인] 위 방식을 실제 Shipping 패키지에서 돌려 본 결과: `-nullrhi`로 모듈 시작까지 가는지, 모듈 시작 시점에서 돌고 강제 종료했을 때 결과 파일과 종료 코드가 남는지, 스크립트가 종료 코드를 받는지, 스테이징 루트의 `RankAndFile.exe`(부트스트랩)가 인자와 종료 코드를 넘기는지. T11 코드를 넣은 첫 패키지에서 확인하고, 확인되면 이 표시를 지운다. 확인하기 전에는 `Saved/StagedBuilds/Windows/RankAndFile/Binaries/Win64/RankAndFile-Win64-Shipping.exe`를 직접 부른다.
```

### 7.2 (선택) `docs/design.md:850` 표의 "Shipping 결정성 (11/18)" 행

지금(둘째 칸):

```
T11은 Shipping 실행 파일이 실행 인자와 입력 파일을 읽고 결과 파일을 써야 한다. [미확인] Shipping에서 실행 인자 처리, 파일을 쓸 수 있는 위치, 로그가 꺼지는지, 종료 코드를 돌려주는 방법
```

바꾼 뒤(둘째 칸):

```
T11은 Shipping 실행 파일이 실행 인자와 입력 파일을 읽고 결과 파일을 써야 한다. 11/21 엔진 소스로 확인했다(`docs/review/t11-shipping-investigation-2026-11-21.md`): 실행 인자는 그대로 읽히고, 결과 파일은 인자로 받은 절대 경로에 쓰고, 로그는 꺼져 있고(설치형 엔진에서는 켤 수 없다), 종료 코드는 `RequestExitWithStatus(true, 코드)`로 돌려준다. [미확인] 실제 패키지에서의 시험
```

셋째 칸(확인 방법)은 그대로 둔다. `Source/RankGame/AGENTS.md`의 "Shipping 결정성 확인 (T11)"(58-61줄)은 이 조사와 어긋나지 않아 고칠 곳이 없다.

## 8. 남은 [미확인] 모음

| # | 내용 | 확인 방법 | 막는 것 |
|---|---|---|---|
| 1 | Shipping 패키지에서 T11 인자가 실제로 전달되고 결과 파일이 써지는지 | 6.1절 코드를 넣은 패키지로 시험 실행 | 없음(소스로는 막는 곳이 없다) |
| 2 | `StartupModule` 시점에서 20건을 돌리고 강제 종료했을 때 결과 파일과 종료 코드가 남는지 | 같은 시험. 틀린 인자 1번도 함께 | 안 되면 `GetOnPostEngineInit()`로 옮긴다 |
| 3 | `-nullrhi`로 Shipping 패키지가 모듈 시작까지 가는지 | `-nullrhi`를 붙인 것과 뺀 것을 한 번씩 | 안 되면 `-nullrhi`를 뺀다 |
| 4 | 스크립트(배치 파일 또는 Node)가 창 프로그램의 종료를 기다리고 종료 코드를 받는지 | 시험에서 받은 코드를 출력 | 엔진 밖 동작 |
| 5 | 부트스트랩 `RankAndFile.exe`가 인자와 종료 코드를 넘기는지 | 소스가 없다. 필요하면 시험 | 쓰지 않으면 확인할 필요가 없다 |
| 6 | `FPaths::ProjectDir()`가 어느 분기를 타는지 | 시험 실행에서 값을 확인 | 설계안은 이 값을 쓰지 않는다 |
| 7 | `DirectoriesToAlwaysStageAsNonUFS`, `RuntimeDependencies`로 Content 밖 파일을 놓는 위치 | 사람이 승인하면 설정을 바꾼 패키지의 매니페스트를 본다 | A안을 쓰면 확인할 필요가 없다 |
| 8 | 따옴표 앞 `\`의 인자 분리 규칙 | 첫 실행 | 루트 경로 끝에 `\`를 붙이지 않으면 피한다 |
| 9 | `Engine/Plugins` 안에 파일 이름 보안 델리게이트를 묶는 플러그인이 있는지 | 이 프로젝트는 켠 플러그인이 없다(`.uproject`에 `Plugins` 항목이 없다). 플러그인을 켜면 다시 찾는다 | 지금은 없음 |

## 9. 이 조사에서 실행한 명령

모두 읽기다. 빌드, 실행, git 쓰기는 하지 않았다.

- 저장소: `ls`(최상위, `docs`, `tools`, `Source`, `Saved/StagedBuilds/Windows`), `cat`(`RankAndFile.uproject`, `Config/*.ini`, `RankGame.Build.cs`, `RankGameModule.cpp`, `.gitignore`), `grep`(`Source/RankCore`·`Source/RankGame`의 `check`·`UE_LOG`, `Source/*.Target.cs`·`*.Build.cs`의 `GlobalDefinitions`, 문서의 `T11`), `wc -l`(`regression_inputs.csv`: 21줄, 머리 1줄과 대전 20건), `git status --short`, `git log --oneline -1`, `git check-ignore -v`.
- 엔진: `cat Engine/Build/Build.version`, `ls Engine/Build`(InstalledBuild.txt), `grep`(허용 목록 매크로, `RequestExitWithStatus`, `WM_QUIT`, `nullrhi`, `NOSPLASH`, `GIsCriticalError`, `bUseLoggingInShipping`, `RequiresUniqueEnvironment`, `DirectoriesToAlwaysStageAsNonUFS`, `RuntimeDependencies`, `GetFilenameSecurityDelegate`, `bootstrap`), `find`(부트스트랩 소스), 그리고 위에 인용한 파일의 해당 줄 읽기.

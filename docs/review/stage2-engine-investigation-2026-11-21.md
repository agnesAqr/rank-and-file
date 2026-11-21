# 2단계 엔진 조사: Unreal MCP, GLB 임포트, 재생·UI API (2026-11-21)

목적: 2단계(화면에서 한 판)를 시작하기 전에, 5.8 설치본의 엔진 소스와 `.uplugin` 파일로 다섯 가지(Q1~Q5)를 확인한다. 규칙 파일과 설계 문서의 [미확인]과 사실과 다른 문장을 고치는 안을 낸다. 이 파일은 조사 결과와 수정안만 담는다. 대상 파일은 고치지 않았다.

상태: 검토 중 · AI 제안(2026-11-21). 수정안을 적용할지, 6.1절의 [미정]을 어떻게 할지는 본인이 정한다. 결정 기록의 행은 본인이 말한 뒤에 적는다. 11/22 처리: 7절 수정안 1·3·10은 11/21~22 묶음 1·3에서, 4·6·7은 확인 조사(`docs/review/unconfirmed-check-2026-11-21.md`)의 문장으로, 5·8·9는 11/22 묶음 4에서 반영했고, 2는 적용하지 않았다. 6.1절의 [미정]은 11/21 AI 기능 결정으로 정해졌다.

```
목표: 2단계에 필요한 엔진 사실 5가지(Unreal MCP 플러그인과 Shipping 제외, GLB 임포트와 Skeleton 공유, KayKit 원본 구성, PlayAnimation과 BindWidget, CSV → DataTable의 행 이름)를 5.8.4 설치본의 소스로 확인하고, 2단계 시작 순서와 문서 수정안을 낸다
완료 기준: Q1~Q5마다 결론, 근거(엔진 루트 기준 파일:줄과 짧은 인용), 이 프로젝트에 주는 뜻, 남은 [미확인]이 있다. 7절 수정안마다 파일:줄, 지금 문장, 바꾼 문장이 있다. 이 파일에 줄표 0개, 사용자 폴더가 들어간 절대 경로 0개
검증 명령: 문서만 만들므로 루트 AGENTS.md 4절 표의 명령은 없다. git status --short(이 작업으로 생긴 파일은 이 파일 하나), 이 파일에서 줄표 문자 검색(0건)과 사용자 폴더 이름 검색(0건). Q3의 수치는 Node 내장 모듈로 GLB의 JSON 청크만 읽어 낸 값이다
금지: AGENTS.md들, docs/rules.md, README.md, docs/design.md, docs/decisions.md, .uproject, Config/, Source/, Content/ 고치기. 빌드, 에디터 실행, 임포트, 플러그인 켜기. git 쓰기 명령. hand_v1.json, docs/crosscheck/**의 result.json·report.md 열기. 외부 패키지
산출물: docs/review/stage2-engine-investigation-2026-11-21.md(이 파일)
```

조사 조건

- 엔진: 5.8.4(`Engine/Build/Build.version`: `"PatchVersion": 4`, `"Changelist": 58902052`). 설치 위치는 `%UE_ROOT%`. 아래의 경로는 모두 엔진 루트 기준이다.
- 저장소: 브랜치 `feat/rules-and-rankcore`, 커밋 `099f906` 위의 작업 트리(`docs/decisions.md` 수정 중, 이 조사와 무관).
- Node v24.18.0. 엔진 파일과 GLB는 읽기만 했다. 임시 Node 스크립트 2개(플러그인 의존 계산, GLB JSON 청크 읽기)는 저장소 밖 임시 폴더에 두었고 끝나고 지웠다.
- 외부 문서는 하나만 읽었다: [Epic: Unreal MCP in Unreal Editor](https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor). 가져온 페이지의 제목은 "Unreal Engine 5.9 Documentation"이다. 5.8.4 설치본과 다른 곳이 있다(Q1). 엔진 동작의 근거는 설치본만 쓴다.

## 0. 요약

| 질문 | 결론 | 확인 |
|---|---|---|
| Q1 Unreal MCP | 11/17 결정의 "Unreal MCP"는 `Engine/Plugins/Experimental/ModelContextProtocol`이다. 5.8.4에서는 기본으로 꺼져 있다(본체도 켜야 한다). 런타임 모듈이 있어 그냥 켜면 Shipping에 들어간다. `.uproject`의 플러그인 항목에 `"TargetAllowList": ["Editor"]`를 달면 게임 대상에서 플러그인과 그 의존이 통째로 빠진다. 의존 목록에 `ScriptableToolsFramework`는 없다 | 확인(Shipping 패키지 결과는 [미확인]) |
| Q2 GLB 임포트 | 기본으로 켜진 Interchange가 GLB의 스켈레탈 메시와 애니메이션을 임포트한다. 새 플러그인은 필요 없다. Skeleton 공유 조건은 본 이름과 부모 사슬(루트 본 이름 포함)이다. 다만 MCP 도구 묶음의 스켈레탈 메시 임포트 도구는 FBX 전용이라 GLB를 받지 않는다 | 일부(실제 임포트는 에디터에서만 확인 가능) |
| Q3 KayKit 원본 | 두 파일 모두 관절 41개, 이름·순서·부모가 같다. 애니메이션은 76개가 95개에 모두 들어 있고, 8.2 표의 15개 이름이 해골 파일에 모두 있다. 8.2 표에는 대기와 전진(이동) 동작이 없다. 라이선스는 둘 다 CC0 | 확인(JSON 청크 범위) |
| Q4 C++ API | `PlayAnimation`은 5.8에 있고 단일 노드 모드로 바꾼다(`Source/AGENTS.md` 42줄과 맞다). `BindWidget`은 에디터의 위젯 블루프린트 컴파일러가 검사하고, 런타임은 같은 이름의 속성에 위젯을 넣는다 | 확인 |
| Q5 CSV → DataTable | 5.8의 CSV 임포트는 `ImportKeyField`가 비어 있으면 첫 열을 행 이름으로 쓴다. 그 열의 값은 행 구조체 필드에 들어가지 않는다. `rules.csv`는 첫 열이 `maxTicks`라 이 경로로는 맞지 않는다 | 확인 |

## 1. Q1 Unreal MCP

### 결론

- 11/17 결정 행이 가리키는 플러그인은 `Engine/Plugins/Experimental/ModelContextProtocol/ModelContextProtocol.uplugin`이다. 표시 이름이 "Unreal MCP"이고 실험 기능이다. 5.8.4에서는 `EnabledByDefault`가 false라 본체도 직접 켜야 한다. `docs/design.md` 14.1의 "본체는 기본으로 켜져 있고"는 5.8.4와 다르다.
- "도구 묶음"은 `Engine/Plugins/Experimental/Toolsets/` 아래의 도구 묶음 플러그인들이다(26개와, 그중 21개를 켜는 `AllToolsets` 1개). Epic 문서는 `All Toolsets`를 켜라고 한다. 본체가 직접 내놓는 것은 도구 목록·설명·호출을 잇는 중계 도구이고(`ModelContextProtocolEngine/Public/ModelContextProtocolSettings.h`:44 `tools/list returns only list_toolsets, describe_toolset, and call_tool.`), 편집 도구는 도구 묶음 플러그인이 `ToolsetRegistry`에 등록한다.
- 본체에는 Runtime 모듈 2개가 있어, 대상을 제한하지 않고 켜면 Shipping 실행 파일에 들어간다. `.uproject`의 항목에 `"TargetAllowList": ["Editor"]`를 달면 UBT가 게임 대상에서 그 플러그인을 건너뛰고, 의존 플러그인도 따라 빠진다 [판단: 아래 UBT 소스].
- `ScriptableToolsFramework`는 본체, `AllToolsets`, `EditorToolset`, `UMGToolSet`, `AnimationAssistantToolset`의 의존을 끝까지 따라가도 나오지 않는다.

### 근거: 본체

| 파일:줄 | 인용 | 뜻 |
|---|---|---|
| `Engine/Plugins/Experimental/ModelContextProtocol/ModelContextProtocol.uplugin`:5 | `"FriendlyName": "Unreal MCP",` | 결정 행과 Epic 문서의 이름 |
| 같은 파일:16, 18 | `"IsExperimentalVersion": true,` / `"EnabledByDefault": false,` | 실험 기능, 기본으로 꺼짐 |
| 같은 파일:20~49 | `ModelContextProtocol` `"Type": "Runtime"`, `ModelContextProtocolEngine` `"Type": "Runtime"`, `ModelContextProtocolEditor` `"Type": "Editor"`, `ModelContextProtocolTests`·`ModelContextProtocolEngineTests` `"Type": "UncookedOnly"`, `ModelContextProtocolEditorTests` `"Type": "Editor"` | 모듈 6개. LoadingPhase는 모두 `"Default"` |
| 같은 파일:51~60 | `"Name": "EngineAssetDefinitions"`, `"Name": "ToolsetRegistry"` | 의존 플러그인 2개 |
| `Engine/Source/Programs/UnrealBuildTool/System/Plugins.cs`:117~126 | `if (Descriptor.bEnabledByDefault.Value)` … `return false;` | 엔진 플러그인은 `EnabledByDefault`가 false면 꺼져 있다. 값이 없으면 프로젝트 플러그인일 때만 켜진다(128~131줄) |

본체에 의존하는 다른 플러그인은 `Engine/Plugins` 전체에서 0개다(`"ModelContextProtocol"`을 담은 `.uplugin`은 자기 자신뿐). 다른 플러그인을 따라 켜지는 일도 없다.

5.9 문서와 다른 곳(문서 쪽은 가져온 페이지의 문장)

| 항목 | Epic 문서(5.9) | 5.8.4 설치본 |
|---|---|---|
| 본체 기본값 | "Unreal MCP is enabled by default." | `ModelContextProtocol.uplugin`:18 `"EnabledByDefault": false,` |
| 서버 자동 시작 | "Auto-start is on by default" | `ModelContextProtocolEngine/Public/ModelContextProtocolSettings.h`:42 `bool bAutoStartServer = false;` |
| 쿠킹 빌드용 도구 표시 | `AIRuntimeCallable` 태그 | 5.8.4의 `Engine/Plugins/Experimental`과 `Engine/Source/Runtime`에 `AIRuntimeCallable` 0건 |

### 근거: 도구 묶음과 의존

| 파일:줄 | 인용 | 뜻 |
|---|---|---|
| `Engine/Plugins/Experimental/ToolsetRegistry/ToolsetRegistry.uplugin`:19, 22~25 | `"EnabledByDefault": false,` / `"Type": "Editor",` `"TargetAllowList": ["Editor"]` | 본체가 끌고 오는 등록기. 에디터 전용 모듈 |
| 같은 파일:28~41 | `"PythonScriptPlugin"`, `"EditorScriptingUtilities"`, `"FileSandbox"` | 셋 다 기본으로 꺼져 있다(각 `.uplugin`:13 `"EnabledByDefault": false`) |
| `Engine/Plugins/Experimental/PythonScriptPlugin/PythonScriptPlugin.uplugin`:20~21 | `"Name": "PythonScriptPluginPreload",` `"Type": "Runtime",` | 제한 없이 켜면 Python 미리 읽기 모듈도 게임 대상에 들어간다 |
| `Engine/Plugins/Experimental/Toolsets/AllToolsets/AllToolsets.uplugin`:6, 11 | `"Aggregator plugin that depends on all Toolsets plugins."` / `"EnabledByDefault": false,` | 모듈 없이 도구 묶음 21개를 켜는 플러그인 |
| `Engine/Plugins/Experimental/Toolsets/EditorToolset/EditorToolset.uplugin`:6, 18~24 | `"Toolsets for interacting with the Unreal Editor and core types (Blueprints, Actors, Properties, etc.)` / `"Type": "Editor",` `"TargetAllowList": [ "Editor" ]` | 에셋, 블루프린트, 데이터 에셋, 캡처 도구. `EnabledByDefault` 키가 없어 엔진 플러그인으로서 꺼져 있다 |
| `Engine/Plugins/Experimental/Toolsets/UMGToolSet/UMGToolSet.uplugin`:6, 14, 21~24 | `"AI assistant tools for creating and manipulating UMG widgets via reflection."` / `"EnabledByDefault": false,` | 위젯 블루프린트 도구 |

의존을 끝까지 따라간 결과 [측정: 2026-11-21, `Engine/Plugins` 아래 `.uplugin` 895개를 Node로 읽음, 비활성 표시(`"Enabled": false`)가 아닌 의존만 셈]

| 켜는 것 | 따라 켜지는 플러그인(자신 포함) | 그중 지금 기본으로 켜진 것 | `ScriptableToolsFramework` |
|---|---|---|---|
| `ModelContextProtocol` | 7개: 본체, `EngineAssetDefinitions`, `ToolsetRegistry`, `PythonScriptPlugin`, `ContentBrowserFileDataSource`, `EditorScriptingUtilities`, `FileSandbox` | 1개(`EngineAssetDefinitions`) | 없음 |
| `ModelContextProtocol` + `EditorToolset` + `UMGToolSet` | 9개 | 1개 | 없음 |
| `AllToolsets` | 78개(`GameplayAbilities`, `StateTree`, `PCG`, `GameFeatures`, `GeometryCollectionPlugin` 등 포함) | 19개 | 없음 |
| `AnimationAssistantToolset` | 14개(`ControlRig`, `LevelSequenceEditor`, `SequencerScripting` 등) | 6개 | 없음 |

`ScriptableToolsFramework`를 의존으로 적은 `.uplugin`은 `Engine/Plugins/Editor/ScriptableToolsEditorMode/ScriptableToolsEditorMode.uplugin`과 `Engine/Plugins/MetaHuman/MetaHumanCharacter/MetaHumanCharacter.uplugin` 두 개뿐이고, 위 네 경우 어디에도 들어오지 않는다.

### 근거: Editor 타입 모듈은 Shipping에 들어가지 않는다

| 파일:줄 | 인용 |
|---|---|
| `Engine/Source/Programs/UnrealBuildTool/Configuration/Descriptors/ModuleDescriptor.cs`:785~787 | `case ModuleHostType.Runtime:` … `return TargetType != TargetType.Program;` |
| 같은 파일:798~800 | `case ModuleHostType.Editor:` … `return TargetType == TargetType.Editor;` |
| 같은 파일:792~793 | `case ModuleHostType.UncookedOnly:` `return !bBuildRequiresCookedData;` |
| `Source/RankAndFile.Target.cs`:9(이 저장소) | `Type = TargetType.Game;` |

Editor 타입은 대상이 Editor일 때만, Runtime 타입은 Program이 아닌 모든 대상(Game 포함)에서 컴파일된다. 본체의 Runtime 모듈 2개는 Shipping용으로 미리 빌드된 것이 설치본에 있다: `Engine/Plugins/Experimental/ModelContextProtocol/Intermediate/Build/Win64/x64/UnrealGame/Shipping/ModelContextProtocol/ModelContextProtocol.precompiled`(같은 폴더에 `ModelContextProtocolEngine`도 있다). 그냥 켜면 링크된다 [판단].

### 근거: Shipping에서 빼는 방법

| 파일:줄 | 인용 | 뜻 |
|---|---|---|
| `Engine/Source/Programs/UnrealBuildTool/Configuration/Descriptors/PluginReferenceDescriptor.cs`:67~70 | `list of targets for which the plugin should be enabled (or all targets if blank).` | `.uproject`의 플러그인 항목에 쓸 수 있는 키 |
| 같은 파일:320 | `GetEnumArrayFieldWithDeprecatedFallback<TargetType>("TargetAllowList", "WhitelistTargets")` | 키 이름은 `TargetAllowList` |
| 같은 파일:405~419 | `if (TargetAllowList != null && TargetAllowList.Length > 0 && !TargetAllowList.Contains(Target))` `return false;` | 목록에 없는 대상에서는 꺼진다 |
| `Engine/Source/Programs/UnrealBuildTool/Configuration/UEBuildTarget.cs`:5840~5843 | `Ignoring plugin '{ReferenceName}' … not enabled for target {TargetType}` … `return null;` | 게임 대상에서는 플러그인을 더하지 않는다 |
| 같은 파일:5961~5975 | `foreach (PluginReferenceDescriptor NextReference in Info.Descriptor.Plugins)` … `AddPlugin(NextReference, …)` | 의존은 플러그인을 더한 뒤에만 따라간다. 건너뛴 플러그인의 의존은 따라 켜지지 않는다 |
| `Engine/Source/Runtime/Projects/Private/PluginReferenceDescriptor.cpp`:73, 161 | `if (TargetAllowList.Num() > 0 && !TargetAllowList.Contains(TargetType))` / `TEXT("TargetAllowList")` | 엔진 런타임의 플러그인 관리자도 같은 키를 읽는다 |

`TargetConfigurationDenyList: ["Shipping"]`(같은 파일 393줄)도 있지만, 이것은 Development·Test 게임 빌드에는 남긴다. "MCP는 에디터에서만 쓴다"(루트 `AGENTS.md` 88줄)에는 `TargetAllowList: ["Editor"]`가 맞다 [판단].

켜려면 `.uproject`에 적을 것 [제안, 적지 않았다]. 도구 묶음을 무엇으로 할지는 6.1절 [미정] 1번이다. 아래는 B안(`EditorToolset` + `UMGToolSet`)의 모양이다. A안이면 아래 두 도구 묶음 항목 대신 `AllToolsets` 한 항목을 같은 모양으로 적는다.

```json
"Plugins": [
	{ "Name": "ModelContextProtocol", "Enabled": true, "TargetAllowList": [ "Editor" ] },
	{ "Name": "EditorToolset", "Enabled": true, "TargetAllowList": [ "Editor" ] },
	{ "Name": "UMGToolSet", "Enabled": true, "TargetAllowList": [ "Editor" ] }
]
```

의존 플러그인(`ToolsetRegistry`, `PythonScriptPlugin` 등)은 `.uproject`에 적지 않아도 따라 켜진다(위 `UEBuildTarget.cs`:5961~5975).

### 근거: 서버가 언제 뜨는가

| 파일:줄 | 인용 | 뜻 |
|---|---|---|
| `ModelContextProtocolEngine/Public/ModelContextProtocolSettings.h`:17 | `UCLASS(MinimalAPI, config=EditorPerProjectUserSettings, …)` | 설정은 사용자별 에디터 설정에 저장된다 |
| 같은 파일:38, 42 | `uint32 ServerPortNumber = 8000;` / `bool bAutoStartServer = false;` | 기본 포트 8000, 자동 시작 꺼짐 |
| `ModelContextProtocolEngine/Private/ModelContextProtocolSettings.cpp`:34, 54~58 | `TEXT("ModelContextProtocolStartServer")` / `if (IsRunningCommandlet())` … `return false;` | 실행 인자로 켤 수 있다. 커맨들릿(쿠킹 포함)에서는 설정값으로 자동 시작하지 않는다 |
| `ModelContextProtocolEditor/Private/ModelContextProtocolEditor.cpp`:63~69 | `if (UE::ModelContextProtocol::ShouldAutoStartServer())` … `Module->StartServer(…)` | 자동 시작은 Editor 모듈에서만 부른다 |
| `ModelContextProtocol/Private/ModelContextProtocolModule.cpp`:31~56 | `TEXT("ModelContextProtocol.StartServer")` … `ECVF_Cheat);` | Runtime 모듈에 서버를 켜는 콘솔 명령이 있다 |
| `ModelContextProtocol/Public/ModelContextProtocol.h`:45~46 | `DefaultServerPort = 8000;` / `DefaultServerUrlPath = TEXT("/mcp");` | 주소 `/mcp` |
| `Engine/Source/Runtime/Online/HTTPServer/Private/HttpServerConfig.h`:13 | `FString BindAddress = FString("localhost");` | 기본 바인드 주소 |
| `Engine/Source/Runtime/Core/Public/HAL/IConsoleManager.h`:73~76 | `behave differently in a final release build.` … `ECVF_Cheat = 0x1,` | 치트 명령은 최종 빌드에서 숨는다 |
| `Engine/Source/Runtime/Core/Public/Misc/Build.h`:214~215, 347~348 | `#define ALLOW_CONSOLE_IN_SHIPPING 0` / `#define ALLOW_CONSOLE ALLOW_CONSOLE_IN_SHIPPING` | Shipping에는 콘솔이 기본으로 없다 |

[판단] 대상을 제한하지 않고 켜도, 쿠킹한 게임에서는 서버를 띄우는 코드(자동 시작)가 Editor 모듈에만 있어 저절로 뜨지는 않는다. 그래도 Runtime 모듈과 서버를 켜는 콘솔 명령이 실행 파일에 남는다. 규칙(Shipping 빌드에서는 뺀다)을 지키려면 `TargetAllowList`로 모듈 자체를 빼는 것이 맞다.

### 이 프로젝트에 주는 뜻

- 11/17 결정 행("Unreal MCP 도구 묶음만 켠다")과 루트 `AGENTS.md` 88줄("Unreal MCP와 그 도구 묶음(toolset) 플러그인 하나")은 "본체는 기본으로 켜져 있다"는 전제 위에 있다. 5.8.4에서는 본체도 켜야 하고, 본체와 함께 켜지는 플러그인이 본체만이면 6개, B안(`EditorToolset` + `UMGToolSet`)이면 8개, A안(`AllToolsets`)이면 78개다(이미 기본으로 켜진 것 포함). 결정을 보완할지는 본인이 정한다(6.1절 [미정] 1번).
- 도구 묶음 하나만으로는 2단계 일을 다 못 한다(Q2의 GLB 임포트, 아래 "MCP 도구로 되는 일과 안 되는 일").
- 서버 자동 시작은 5.8.4에서 기본으로 꺼져 있다. `tools\run_tests.bat`은 커맨들릿이 아니라 `-ExecCmds`로 에디터를 띄운다(`tools/run_tests.bat` 19줄). 자동 시작을 켜 두면 테스트용 에디터도 8000번을 잡으려 해, 열려 있는 에디터와 부딪칠 수 있다 [추측]. 자동 시작은 끈 채로 두고, MCP를 쓸 때만 에디터 콘솔의 `ModelContextProtocol.StartServer`나 실행 인자 `-ModelContextProtocolStartServer`로 켜기를 제안한다 [제안].
- 클라이언트 설정을 만드는 콘솔 명령 `ModelContextProtocol.GenerateClientConfig ClaudeCode`(`ModelContextProtocolEngine/Private/ModelContextProtocolEngineModule.cpp`:40~42)는 설치형 엔진에서 프로젝트 폴더에 `.mcp.json`을 쓴다(`ModelContextProtocolEngine/Private/ModelContextProtocolClientConfig.cpp`:30~31 `TEXT(".mcp.json")`, 161~165 `FApp::IsEngineInstalled() ? FPaths::ProjectDir()`). 저장소 루트에 새 파일이 생기므로 커밋할지 무시 목록에 넣을지 정해야 한다(6.1절 [미정] 6번).
- MCP 분석 이벤트는 기본으로 켜져 있다(`ModelContextProtocol/Private/ModelContextProtocolAnalytics.cpp`:15~18 `static bool bEnableAnalytics = true;`). 엔진 분석 경로로 보낸다(`ModelContextProtocolEngineModule.cpp`:125~126). [미확인] 무엇이 어디로 가는지와 에디터의 사용 데이터 설정을 따르는지. 확인 방법: `ModelContextProtocolAnalytics.cpp` 전체와 `EngineAnalyticsProviderProxy.h`를 읽는다.

### MCP 도구로 되는 일과 안 되는 일 (2단계 기준)

| 2단계 일 | 도구 | 근거 | 판정 |
|---|---|---|---|
| 위젯 블루프린트 생성(C++ 부모) | `UMGToolSet`의 `CreateWidgetBlueprint(FolderPath, AssetName, ParentClass)` | `Engine/Plugins/Experimental/Toolsets/UMGToolSet/Source/UMGToolSet/Private/UMGToolSet.h`:286~287 | 된다 |
| 위젯 배치, 이름 맞추기(`BindWidget`) | `AddWidget`, `RenameWidget`, `GetWidgetDescription` | 같은 파일:299~300, 382~383, 441~442 | 된다 |
| 이벤트 그래프 노드 수 확인 | `EditorToolset`의 `BlueprintTools.list_graphs`, `find_nodes` | `…/EditorToolset/Content/Python/editor_toolset/toolsets/blueprint.py`:233, 681 | 된다 |
| 화면 캡처로 확인 | `CaptureAssetImage`, `CaptureEditorImage`, `CaptureViewport` | `…/EditorToolset/Source/EditorToolset/Private/EditorAppToolset.h`:215~216, 222~223, 246 | 된다 |
| GLB 스켈레탈 메시 임포트 | `SkeletalMeshTools.import_file` | `…/toolsets/skeletal_mesh.py`:50~64 `options = unreal.FbxImportUI()` … `factory=unreal.FbxFactory())`, `…/toolsets/asset.py`:12~19 확장자 검사, `Engine/Source/Editor/UnrealEd/Private/Fbx/FbxFactory.cpp`:52~53 `fbx`·`obj`만 | 안 된다. `.glb`는 `ValueError` |
| 애니메이션 블루프린트 생성 | 없음 | 도구 묶음 26개와 `ToolsetRegistry`, 본체의 소스·Python에서 `AnimBlueprint` 0건 | 안 된다 [판단]. `BlueprintTools.create`는 `unreal.BlueprintFactory()`로 만든다(`blueprint.py`:177~195). [미확인] `UAnimInstance` 부모로 부르면 애니메이션 블루프린트가 되는지. 확인 방법: 켠 뒤 시험 폴더에 만들고 `AssetTools.get_asset_class`로 클래스를 본다 |
| 데이터 에셋 연결 | `DataAssetTools` | `…/toolsets/data_asset.py`:11 | 된다 [미확인] 세부 도구는 읽지 않았다 |

### 남은 [미확인]

1. `TargetAllowList: ["Editor"]`로 켠 뒤 Shipping 패키지에 MCP가 정말 없는지. 확인 방법: 6.2절 절차(`tools\package.bat`, 실행, 포트 확인, 스테이징 폴더 검색).
2. 쿠커가 에디터 전용으로 켠 플러그인의 콘텐츠(`ToolsetRegistry`·`EditorToolset`의 `CanContainContent: true`)를 패키지에 넣는지. 확인 방법: 같은 절차에서 스테이징 폴더와 pak 목록을 본다.
3. 사용자별 설정 파일의 실제 위치. 확인 방법: 에디터에서 설정을 바꾼 뒤 `Saved/Config/` 아래를 본다.

## 2. Q2 GLB 임포트

### 결론

- 기본으로 켜진 Interchange 플러그인 3개가 glTF/GLB를 임포트한다. 이 프로젝트의 `.uproject`에 플러그인 항목이 없어도 쓸 수 있다. 번역기는 메시, 머티리얼, 애니메이션을 지원한다. 새 플러그인을 켜지 않고 GLB에서 스켈레탈 메시와 애니메이션을 가져올 수 있다 [판단: 아래 소스. 실제 결과는 에디터에서만 확인 가능].
- 두 메시가 Skeleton 하나를 쓰는 조건(Interchange): 임포트하는 메시의 본마다, 이름이 Skeleton에 있으면 그 본에서 위로 올라가는 부모 사슬의 이름이 같아야 하고, 사슬의 끝(루트)이 함께 끝나야 한다. 없는 본은 조상 중 하나가 Skeleton에 있어야 한다. 맞는 본이 1개 이상이어야 한다. 순서는 조건이 아니다.
- Skeleton을 지정했는데 맞지 않으면 "Incompatible skeleton" 메시지를 내고 지정을 쓰지 않는다. 그때 Skeleton이 새로 생긴다 [추측]. `Content/AGENTS.md` 30줄의 확인(Skeleton 에셋이 둘 이상이면 실패)이 그대로 맞다.
- 임포트 때 애니메이션 블루프린트를 만드는 옵션은 Interchange에 없다.
- IK Retargeter가 든 `IKRig` 플러그인은 기본으로 켜져 있다. 공유가 안 될 때의 대안도 새 플러그인 없이 쓸 수 있다.

### 근거

| 파일:줄 | 인용 | 뜻 |
|---|---|---|
| `Engine/Plugins/Interchange/Runtime/Interchange.uplugin`:13 | `"EnabledByDefault": true,` | Interchange Framework |
| 같은 파일:24~27, 39~43 | `"Name": "GLTFCore",` `"Type": "Runtime",` / `"Name": "InterchangeImport",` | glTF 읽기와 임포트 모듈 |
| `Engine/Plugins/Interchange/Editor/InterchangeEditor.uplugin`:13, 22~49 | `"EnabledByDefault": true,` / 모듈 3개 `"Type": "Editor"` | 에디터 쪽 |
| `Engine/Plugins/Interchange/Assets/InterchangeAssets.uplugin`:13 | `"EnabledByDefault": true,` | 기본 파이프라인 에셋 |
| `Engine/Plugins/Interchange/Runtime/Source/Import/Private/Gltf/InterchangeGltfTranslator.cpp`:492~495 | `TEXT("glb;GL Transmission Format (Binary)")` | `.glb`를 받는다 |
| 같은 파일:473 | `return EInterchangeTranslatorAssetType::Materials \| EInterchangeTranslatorAssetType::Meshes \| EInterchangeTranslatorAssetType::Animations;` | 메시·머티리얼·애니메이션 |
| `…/Import/Public/Gltf/InterchangeGltfTranslator.h`:24 | `glTF translator class support import of texture, material, static mesh, skeletal mesh,` | |
| `…/Import/Private/InterchangeImportModule.cpp`:111, 179 | `RegisterTranslator(UInterchangeGLTFTranslator::StaticClass());` / `SetInterchangeImportEnabled(true);` | 모듈이 뜨면 번역기를 등록하고 Interchange 임포트를 켠다 |
| `Engine/Source/Runtime/Interchange/Engine/Private/InterchangeManager.cpp`:60~64, 2503~2505 | `static bool GInterchangeImportEnable = false;` `TEXT("Interchange.FeatureFlags.Import.Enable"),` / `return CCvarInterchangeImportEnable->GetBool();` | 기본값은 꺼짐이고, 위 모듈이 뜰 때 켠다 |
| `Engine/Source/Developer/AssetTools/Private/AssetTools.cpp`:3567~3569 | `return UInterchangeManager::IsInterchangeImportEnabled() && (SpecifiedFactory == nullptr);` | 팩토리를 지정하지 않은 임포트는 Interchange로 간다 |
| `Engine/Config/BaseEngine.ini`:3785~3786 | `[/Script/InterchangeEngine.InterchangeProjectSettings]` … `Translator="/Script/InterchangeImport.InterchangeGLTFTranslator",Pipelines=("/Interchange/Pipelines/DefaultGLTFAssetsPipeline…` | glTF 기본 파이프라인 |
| `…/Import/Private/Gltf/InterchangeGltfAnimation.cpp`:1148~1151, 742 | `for (int32 AnimationIndex = 0; AnimationIndex < Animations.Num(); AnimationIndex++)` / `SetupNode(TrackNode, TrackNodeUid, GLTFAnimation.Name, …)` | glTF 애니메이션마다 하나씩, 이름은 glTF의 이름 |
| `…/Import/Private/Mesh/InterchangeSkeletonHelper.cpp`:113~131, 162~179 | `// first ensure the parent exists for each bone` … `return (NumOfBoneMatches > 0);` | 공유 조건(이름과 부모 사슬) |
| 같은 파일:425~428, 444~447 | `// verify name of root bone matches` / `if one of the parents doesn't exist, make sure both end.` | 루트 이름이 같아야 하고 사슬이 함께 끝나야 한다 |
| `…/Pipelines/Private/InterchangeGenericSkeletalMeshPipeline.cpp`:305~322 | `IsCompatibleSkeleton(Skeleton` … `"Incompatible skeleton {0} when importing skeletalmesh {1}."` | 지정한 Skeleton이 맞을 때만 쓴다 |
| `…/Import/Private/Mesh/InterchangeSkeletalMeshFactory.cpp`:1265~1268 | `if (bSpecifiedSkeleton && !SkeletonReference->IsCompatibleMesh(SkeletalMesh))` | 생성 단계에서 한 번 더 경고 |
| `…/Pipelines/Public/InterchangeGenericAssetsPipelineSharedSettings.h`:193~203 | `bool bImportOnlyAnimations = false;` / `leaving this as "None" will create a new skeleton.` / `TSoftObjectPtr<USkeleton> Skeleton;` | 임포트 대화상자의 Skeleton 칸 |
| 같은 파일:81~83 | `all static meshes nested in bone hierarchies will be imported as skeletal meshes instead.` / `bool bConvertStaticsInBoneHierarchyToSkeletals = true;` | 본 아래에 붙은 메시(무기, 투구, 망토)를 스켈레탈 메시에 합친다(C++ 기본값) |
| `…/Pipelines/Public/InterchangeGenericAnimationPipeline.h`:67~69 | `bool bImportAnimations = true;` | |
| `…/Pipelines/Public/InterchangeGenericMeshPipeline.h`:73, 204, 236 | `bImportStaticMeshes = true;` / `bImportSkeletalMeshes = true;` / `bCreatePhysicsAsset = true;` | |
| `Engine/Plugins/Animation/IKRig/IKRig.uplugin`:5, 13 | `"FriendlyName": "IK Rig",` / `"EnabledByDefault": true,` | IK Retargeter(`Source/IKRig/Public/Retargeter/IKRetargeter.h`:70 `class UIKRetargeter`) |

Interchange의 파이프라인·임포트·에디터 소스(`Engine/Plugins/Interchange/Runtime/Source/Pipelines`, `…/Import`, `Engine/Plugins/Interchange/Editor/Source`)에서 `AnimBlueprint`는 0건이다.

### 이 프로젝트에 주는 뜻

- 임포트는 새 플러그인 없이 된다. 그러나 MCP의 스켈레탈 메시 도구가 FBX 전용이라, AI가 MCP로 GLB를 임포트할 수 없다(Q1 표). `Content/AGENTS.md` 11줄("MCP로 만들 수 없거나 실패하면 순서와 설정값을 적은 절차로 사람에게 넘긴다")에 따르면 임포트는 사람이 하고 AI는 설정값과 확인을 맡게 된다. 다른 길은 6.1절 [미정] 3번.
- `Knight.glb`는 무기·방패 7개와 투구·망토가 본 아래 별도 메시로 들어 있다(Q3). C++ 기본값(`bConvertStaticsInBoneHierarchyToSkeletals = true`)대로면 이것들이 모두 스켈레탈 메시에 합쳐진다 [추측: 화면은 보지 않았다]. "몸만 가져온다"(`Content/AGENTS.md` 32줄)를 지키려면 이 값을 끄고 `bImportStaticMeshes`도 끄는 설정이 후보다 [제안]. 투구·망토를 함께 버릴지는 6.1절 [미정] 4번.
- 기본 파이프라인 에셋(`DefaultGLTFAssetsPipeline.uasset`)이 위 C++ 기본값을 바꿨을 수 있다 [미확인]. 확인 방법: 임포트 대화상자에서 값을 읽거나, 켠 뒤 MCP `ObjectTools`로 파이프라인 에셋의 속성을 읽는다.
- 애니메이션 블루프린트 자동 생성 옵션이 없으므로 `Content/AGENTS.md` 35줄의 "있으면 끈다"는 Interchange에는 해당이 없다.

### 남은 [미확인]

1. 실제 임포트 결과(Skeleton 1개, AnimSequence 수, 에셋 이름). glTF 애니메이션 이름이 에셋 이름에 그대로 쓰이는지(앞이나 뒤에 붙는 것이 있는지). 확인 방법: 6.3절 절차의 개수 확인.
2. 본 아래 메시를 끈 설정에서 몸 6조각(기사)·9조각(졸개)만 남는지. 확인 방법: 임포트 뒤 메시 섹션 수와 `CaptureAssetImage`.
3. 바인드 포즈(`inverseBindMatrices`)가 두 파일에서 같은지. JSON 청크만 읽어서 비교하지 않았다. 확인 방법: 임포트 뒤 두 메시의 참조 포즈를 Skeleton 편집기에서 보거나, BIN 청크를 읽는 별도 조사.

## 3. Q3 KayKit 원본 확인

[측정: 2026-11-21, Node v24.18.0 내장 `fs`로 GLB 머리 20바이트와 첫 청크(JSON)만 읽음. 파일은 고치지 않았다. SHA-256은 `sha256sum`으로 계산]

### 결론

- 두 파일의 스킨은 각 1개(이름 `Rig`), 관절 41개다. 관절 이름, 순서, 부모가 모두 같다. 다른 곳 0개, 중복 이름 0개. 루트 관절은 둘 다 `root`이고 그 부모는 관절이 아닌 노드 `Rig`다.
- 관절 노드의 로컬 변환(이동, 회전, 크기)도 같다. 이동·크기 차이는 거의 0이고(`toes.l`, `toes.r`에서 최대 4×10⁻⁶. 작업 세션이 다시 계산했다), 회전은 `toes.r` 하나만 쿼터니언 부호가 반대로 저장돼 있다(내적 −1.000000, 같은 회전).
- `Knight.glb`의 애니메이션 76개는 모두 `Skeleton_Minion.glb`의 95개 안에 있다. `docs/design.md` 8.2 표의 15개 이름은 모두 해골 파일에 있고, 그중 3개는 해골 파일에만 있다.
- 8.2 표에는 대기 동작과 대열 전진(이동) 동작이 없다. 해골 파일에 후보가 있지만, 표에 없는 이름은 짐작으로 쓰지 않는다(`Source/RankGame/AGENTS.md` 35줄). 6.1절 [미정] 5번.
- 두 LICENSE의 라이선스는 둘 다 Creative Commons Zero(CC0)다. GLB의 SHA-256은 `docs/decisions.md` 11/20 KayKit 행과 같다.

### 파일 구성

| 항목 | `Adventurers/Knight.glb` | `Skeletons/Skeleton_Minion.glb` |
|---|---|---|
| 파일 크기 | 3,659,532바이트(머리의 전체 길이와 같다) | 4,814,296바이트(같다) |
| SHA-256 | `60428e3a…3445168`(결정 기록과 같다) | `6ffc003f…5f95968`(같다) |
| 만든 도구, 형식 | Khronos glTF Blender I/O v1.7.33, glTF 2.0 | 같다 |
| JSON 청크 | 1,972,652바이트 | 2,441,948바이트 |
| 노드 | 57 | 51 |
| 메시 | 15(스킨 6: 팔 2, 몸, 머리, 다리 2 · 본 아래 고정 9: `handslot.l` 아래 방패 4와 `1H_Sword_Offhand`, `handslot.r` 아래 `1H_Sword`·`2H_Sword`, `head` 아래 `Knight_Helmet`, `chest` 아래 `Knight_Cape`) | 9(모두 스킨: 팔 2, 몸, 망토, 눈, 머리, 턱, 다리 2) |
| 머티리얼 | 1(`knight_texture`) | 2(`skeleton`, `Glow`) |
| 텍스처 / 이미지 | 1 / 1(PNG, 파일 안에 들어 있음) | 1 / 1(PNG, 파일 안) |
| 스킨 / 관절 | 1(`Rig`) / 41 | 1(`Rig`) / 41 |
| 정점당 본 영향 | `JOINTS_1` 없음(최대 4) | 같다 |
| 확장(`extensionsUsed`) | 없음 | 없음 |
| 애니메이션 | 76 | 95 |

관절 41개(부모): `root`(Rig), `hips`(root), `spine`(hips), `chest`(spine), `upperarm.l`(chest), `lowerarm.l`, `wrist.l`, `hand.l`, `handslot.l`(hand.l), `upperarm.r`(chest), `lowerarm.r`, `wrist.r`, `hand.r`, `handslot.r`(hand.r), `head`(chest), `upperleg.l`(hips), `lowerleg.l`, `foot.l`, `toes.l`, `upperleg.r`(hips), `lowerleg.r`, `foot.r`, `toes.r`, `kneeIK.l`(root), `control-toe-roll.l`(root), `control-heel-roll.l`, `control-foot-roll.l`, `heelIK.l`(control-foot-roll.l), `IK-foot.l`(control-foot-roll.l), `IK-toe.l`(control-heel-roll.l), 오른쪽 같은 구성 7개, `elbowIK.l`, `handIK.l`, `elbowIK.r`, `handIK.r`(모두 root). 부모를 적지 않은 것은 바로 앞 관절이 부모다.

### 애니메이션과 8.2 표

| 8.2 표의 이름 | Knight | Skeleton_Minion |
|---|---|---|
| `1H_Melee_Attack_Chop`, `Block_Hit`, `Death_A`, `2H_Melee_Attack_Chop`, `Hit_A`, `Death_B`, `2H_Ranged_Shoot`, `Spellcast_Shoot`, `Hit_B`, `Dualwield_Melee_Attack_Stab`, `1H_Melee_Attack_Slice_Horizontal`, `Cheer` | 있음 | 있음 |
| `Death_C_Skeletons`, `Death_C_Skeletons_Resurrect`(부활), `Spawn_Ground_Skeletons`(등장) | 없음 | 있음 |

해골 파일에만 있는 19개: `1H_Melee_Attack_Jump_Chop`, `Death_C_Pose`, `Death_C_Skeletons`, `Death_C_Skeletons_Resurrect`, `Idle_B`, `Idle_Combat`, `Running_C`, `Skeleton_Inactive_Standing_Pose`, `Skeletons_Awaken_Floor`, `Skeletons_Awaken_Floor_Long`, `Skeletons_Awaken_Standing`, `Skeletons_Inactive_Floor_Pose`, `Spawn_Air`, `Spawn_Ground`, `Spawn_Ground_Skeletons`, `Spellcast_Summon`, `Taunt`, `Taunt_Longer`, `Walking_D_Skeletons`.

8.2 표에 없는 동작의 후보(해골 파일에 있는 이름, 고르지 않았다): 대기 `Idle`, `Idle_B`, `Idle_Combat`, `2H_Melee_Idle`, `Unarmed_Idle` · 이동 `Walking_A`, `Walking_B`, `Walking_C`, `Walking_D_Skeletons`, `Walking_Backwards`, `Running_A`, `Running_B`, `Running_C`.

### 이 프로젝트에 주는 뜻

- 두 파일은 Q2의 공유 조건(이름, 부모 사슬, 루트 이름)을 모두 만족한다. 임포트에서 Skeleton 공유가 실패할 가능성은 낮다 [판단]. 9종 가운데 2종만 확인했다.
- 애니메이션은 해골 파일 하나에서 가져오면 된다는 `Content/AGENTS.md` 31줄이 이 두 파일에서는 맞다.
- `docs/design.md` 8.6절 461줄의 "기사는 방패 4개와 검 2개"는 이 파일에서 방패 4개와 검 3개(`1H_Sword_Offhand`, `1H_Sword`, `2H_Sword`)다(7절 수정안 9).

### 남은 [미확인]

1. 나머지 7종(Barbarian, Rogue, Mage, Rogue_Hooded, Skeleton_Warrior, Skeleton_Rogue, Skeleton_Mage). 아직 받지 않았다. 확인 방법: 받은 뒤 같은 방식으로 JSON 청크를 읽는다.
2. 바인드 포즈(`inverseBindMatrices`)의 비교(Q2 남은 [미확인] 3).

## 4. Q4 C++ API

### 결론

- `USkeletalMeshComponent::PlayAnimation(UAnimationAsset*, bool)`은 5.8 헤더에 있다. 구현은 `SetAnimationMode(EAnimationMode::AnimationSingleNode)`를 먼저 부르고, 모드가 바뀌면 지금의 애니메이션 인스턴스를 지운다. `Source/AGENTS.md` 42줄("이 함수는 메시를 단일 노드 모드로 바꾼다", "애니메이션 블루프린트 모드가 풀린다")과 맞다.
- `meta=(BindWidget)`은 두 곳에서 처리된다. ① 에디터(UMGEditor 모듈)의 위젯 블루프린트 컴파일러가 부모 C++ 클래스의 `BindWidget`·`BindWidgetOptional` 속성마다 같은 이름의 위젯이 있는지, 형이 맞는지 검사한다. 없으면 새로 만든 블루프린트는 경고, 그 밖에는 오류다. ② 런타임(UMG 모듈)은 위젯 트리의 위젯마다 이름이 같은 객체 속성을 찾아 넣는다.

### 근거

| 파일:줄 | 인용 |
|---|---|
| `Engine/Source/Runtime/Engine/Classes/Components/SkeletalMeshComponent.h`:1257~1258 | `UFUNCTION(BlueprintCallable, …, UnsafeDuringActorConstruction = "true"))` / `ENGINE_API void PlayAnimation(class UAnimationAsset* NewAnimToPlay, bool bLooping);` |
| 같은 파일:1251~1255 | `These changes status of animation instance, which is transient data,` … `Please use OverrideAnimationData for construction script.` |
| `Engine/Source/Runtime/Engine/Private/Components/SkeletalMeshComponent.cpp`:4278~4289 | `SetAnimationMode(EAnimationMode::AnimationSingleNode);` `SetAnimation(NewAnimToPlay);` `Play(bLooping);` |
| 같은 파일:4254~4259 | `const bool bNeedChange = AnimationMode != InAnimationMode;` … `ClearAnimScriptInstance();` |
| `Engine/Source/Runtime/UMG/Public/Components/Widget.h`:68~70, 72~74 | `This property requires a widget be bound to it in the designer.` / `BindWidgetOptional,` |
| `Engine/Source/Editor/UMGEditor/Private/WidgetBlueprintEditorUtils.cpp`:1789~1790 | `InProperty->HasMetaData("BindWidget") \|\| InProperty->HasMetaData("BindWidgetOptional");` |
| `Engine/Source/Editor/UMGEditor/Private/WidgetBlueprintCompiler.cpp`:1257~1262 | `// Check that all BindWidget properties are present and of the appropriate type` |
| 같은 파일:1282~1291 | `else if (Blueprint->bIsNewlyCreated)` `MessageLog.Warning(…RequiredWidgetNotBoundError…)` … `MessageLog.Error(…)` |
| `Engine/Source/Runtime/UMG/Private/WidgetBlueprintGeneratedClass.cpp`:238~244, 270~275 | `TMap<FName, FObjectPropertyBase*> ObjectPropertiesMap;` … `// Find property with the same name as the template and assign the new widget to it.` `Prop->SetObjectPropertyValue_InContainer(UserWidget, Widget);` |

### 이 프로젝트에 주는 뜻

- `PlayAnimation`은 액터 생성 스크립트 안에서 부르지 말라는 주석이 있다(헤더 1251~1255줄). 재생기는 `BeginPlay` 이후에 부른다 [판단].
- `BindWidget` 검사는 에디터 컴파일 단계에서 일어난다. MCP로 위젯 블루프린트를 만든 뒤 컴파일 메시지(필수 바인딩 누락 0건)와 이름 목록(`GetWidgetDescription`)으로 "BindWidget 대상 위젯 N개가 모두 있음"(`Content/AGENTS.md` 10줄)을 확인할 수 있다.
- 런타임 연결은 메타데이터가 아니라 이름으로 된다. `BindWidget`을 달지 않은 C++ 객체 속성도 이름이 같으면 위젯이 들어간다 [판단: 238~244줄이 클래스의 모든 객체 속성을 모은다]. C++ 위젯 클래스의 위젯 아닌 객체 속성 이름이 디자이너의 위젯 이름과 겹치지 않게 한다 [제안].

### 남은 [미확인]

없음(동작은 소스로 확인했다. 실제 컴파일 메시지는 2단계에서 본다).

## 5. Q5 CSV → DataTable의 행 이름

### 결론

- 5.8의 CSV 임포트(`UDataTable::CreateTableFromCSVString` → `FDataTableImporterCSV::ReadTable`)는 `ImportKeyField`가 비어 있으면 첫 열을 행 이름으로 쓴다. `ImportKeyField`에 열 이름을 주면 그 열이 행 이름이 된다.
- 행 이름이 된 열의 값은 행 구조체의 필드에 넣지 않는다. 열 머리와 셀 모두 그 열을 건너뛴다.
- 행 이름이 겹치면 그 행은 버리고 문제 목록에 "Duplicate row name"을 더한다.
- `CreateTableFromCSVString`은 `WITH_EDITOR` 밖에 선언돼 있다. 행 구조체를 먼저 정해야 하고 문제 목록을 돌려준다(`docs/design.md` 7.3절 2번의 [미확인] 해소).

### 근거

| 파일:줄 | 인용 |
|---|---|
| `Engine/Source/Runtime/Engine/Private/DataTableCSV.cpp`:151~164 | `int32 KeyColumn = 0;` `if (!DataTable->ImportKeyField.IsEmpty())` `// Search for key column by name` |
| 같은 파일:196 | `FName RowName = DataTableUtils::MakeValidName(Cells[KeyColumn]);` |
| 같은 파일:215~223 | `if (!DataTable->AllowDuplicateRowsOnImport())` … `TEXT("Duplicate row name '%s'.")` |
| 같은 파일:236~242 | `// Now iterate over cells (skipping first cell unless we had an explicit name)` `if (CellIdx == KeyColumn)` `continue;` |
| `Engine/Source/Runtime/Engine/Private/DataTable.cpp`:793~799 | `// Skip first column depending on option` … `if (ColIdx == KeyColumn)` `continue;` |
| `Engine/Source/Runtime/Engine/Classes/Engine/DataTable.h`:131~133 | `If this is empty it uses Name for JSON and the first field found for CSV` / `FString ImportKeyField;` |
| 같은 파일:345~351 | `#endif` 다음 `RowStruct must be defined before calling this function.` `ENGINE_API TArray<FString> CreateTableFromCSVString(const FString& InString);` |
| `Engine/Source/Runtime/Engine/Private/DataTable.cpp`:878~888 | `FDataTableImporterCSV(*this, InString, OutProblems).ReadTable();` … `return OutProblems;` |
| 같은 `DataTable.cpp`:811~817 | `FindFProperty<FProperty>(InRowStruct, PropName);` … `GetPropertyImportNames(*It, TempPropertyImportNames);` |
| `Engine/Source/Runtime/Engine/Private/DataTableUtils.cpp`:447~454, 431~437 | `OutResult.AddUnique(Prop->GetName());` `OutResult.AddUnique(GetPropertyExportName(Prop));` / `return Prop->GetAuthoredName();` |

### 이 프로젝트에 주는 뜻

- `Data/AGENTS.md` 55줄의 "첫 열이 DataTable의 행 이름이 된다"는 5.8 소스와 맞다.
- 지금 코드는 DataTable 임포트를 쓰지 않는다. `RankGame`이 CSV를 직접 읽는다(`Source/RankGame/Private/RankContent.cpp` 189줄 `ParseUnitsCsv`, 257줄 `ParseRulesCsv`). `RankEditor` 임포터(`Content/AGENTS.md` 19줄)를 만들 때 엔진 CSV 임포트를 쓰면 다음이 생긴다 [판단: 위 소스].
  - `units.csv`: 첫 열 `key`가 행 이름이 되고, `FRankUnitRow::Key`(`Source/RankGame/Public/RankContentRows.h` 18줄)에는 값이 들어가지 않는다.
  - `units.csv`의 `magic` 열: 속성 이름은 `bMagic`(같은 파일 43줄)이다. 엔진은 속성 이름과 작성 이름으로만 열을 찾으므로 `magic`은 찾지 못하고 "Cannot find Property for column" 문제를 낸다(`DataTable.cpp`:820~825).
  - `rules.csv`: 첫 열이 `maxTicks`라 600이 행 이름이 되고 `MaxTicks` 필드는 0으로 남는다. `Data/AGENTS.md` 55줄의 "첫 열(`id` 또는 `key`)"에 이 파일은 맞지 않는다.
- [제안] `RankEditor` 임포터는 엔진 CSV 임포트 대신 `RankGame`의 읽기 함수로 행 구조체를 만든 뒤 `UDataTable::AddRow`(`DataTable.h`:315, `WITH_EDITOR` 밖)로 넣는다. "콘텐츠를 읽는 경로는 하나다"(`Source/RankGame/AGENTS.md` 18줄)와 맞고 위 세 문제가 없다. 엔진 CSV 임포트를 쓸지는 본인이 정한다(6.1절 [미정] 8번).

### 남은 [미확인]

1. 열 이름 비교가 대소문자를 가리지 않는지(`hp` → `Hp`). [추측] `FName` 비교라 가리지 않는다. 확인 방법: `FindFProperty`와 `FName` 비교 구현을 읽거나, 2단계 임포터 테스트에서 본다.

## 6. 2단계를 시작할 때의 순서 [제안]

### 6.1 먼저 사람이 정할 것 [미정]

1. 도구 묶음. A안 `AllToolsets` 한 항목(Epic 문서의 방법, 의존까지 78개가 에디터에서 켜진다). B안 `EditorToolset` + `UMGToolSet`(9개, 도구 묶음 플러그인이 둘이다). C안 `EditorToolset` 하나(위젯 블루프린트 도구 `CreateWidgetBlueprint`가 없다). AI 권장은 B안이다: 2단계에 필요한 도구(위젯 블루프린트, 데이터 에셋, 그래프 노드 수, 캡처)가 다 있고 켜지는 것이 가장 적다. B안이면 루트 `AGENTS.md` 88줄의 "도구 묶음(toolset) 플러그인 하나"와 맞지 않으므로 그 문장과 11/17 결정 행을 고쳐야 한다(7절 수정안 10).
2. `.uproject`에 1절의 항목(`TargetAllowList: ["Editor"]`)을 적는 것. `.uproject` 수정은 2단계 작업 안에서 승인을 받고 한다.
3. GLB 임포트를 누가 하는가. ① 사람이 에디터 콘텐츠 브라우저에서 임포트하고, AI는 설정값 표와 MCP 조회로 확인한다(`Content/AGENTS.md` 11줄의 절차). ② `RankEditor`에 GLB 임포트 커맨들릿을 C++로 더한다(`IAssetTools::ImportAssetTasks`, 팩토리 없음 → Interchange). 파이프라인 설정을 코드에서 주려면 Interchange 모듈에 기대야 할 수 있어 `Source/AGENTS.md` 54줄("플러그인에 기대는 코드를 더하지 않는다")과 부딪칠 수 있다 [추측]. ③ GLB를 FBX로 바꿔 MCP 도구로 임포트한다. 바꾸는 도구가 하나 더 들고 변환 품질을 따로 봐야 한다. AI 권장은 ①이다.
4. 기사의 투구·망토: 무기와 함께 버리고 몸 6조각만 가져올지, 투구·망토는 남길지.
5. 8.2 표에 넣을 대기 동작과 대열 전진 동작의 이름(3절의 후보 가운데서).
6. `ModelContextProtocol.GenerateClientConfig`가 만드는 `.mcp.json`(주소만 들어 있다, `ModelContextProtocolClientConfig.cpp`:158 `TEXT("http://127.0.0.1:%u%s")`)을 커밋할지 무시 목록에 넣을지.
7. 애니메이션 블루프린트를 누가 만드는가. 루트 `AGENTS.md` 87줄은 AI가 MCP로 만든다고 하고, `Content/AGENTS.md` 35줄은 "필요해지면 C++ 부모를 지정해 사람이 만든다"고 한다. 5.8.4 도구 묶음에는 애니메이션 블루프린트 도구가 없다(1절 표). 블렌딩이 필요해질 때까지는 `PlayAnimation`만 쓰므로 지금 막히지는 않는다.
8. `RankEditor` 임포터가 엔진 CSV 임포트를 쓸지, `RankGame`의 읽기 함수와 `AddRow`를 쓸지(5절).

### 6.2 MCP를 켜고 빈 Shipping 패키지로 확인 (루트 `AGENTS.md` 88줄의 절차)

1. 켜기 전 기준: `tools\package.bat` → `Saved\StagedBuilds\Windows\RankAndFile.exe` 실행. 지금 상태로 패키지가 만들어지고 실행되는지 본다.
2. `.uproject`에 6.1 1번에서 고른 항목을 `TargetAllowList: ["Editor"]`와 함께 적는다.
3. 에디터를 연다. 플러그인 창에서 켜진 것을 보고, 출력 로그에 `LogModelContextProtocol` 오류가 없는지 본다. 자동 시작은 끈 채로 두고 콘솔에서 `ModelContextProtocol.StartServer`로 서버를 켠다. 콘솔 명령 대신 실행 인자 `-ModelContextProtocolStartServer`로도 켤 수 있다(`ModelContextProtocolSettings.cpp:34`, 11/21 확인). `-ExecCmds`는 명령을 쉼표로 나눈다(`ParseExecCommands.cpp:28`).
4. 에디터를 닫고 `tools\run_tests.bat`을 돌린다. `RankAndFile.*` 실패 0, 실행 수가 켜기 전과 같은지 본다(MCP·도구 묶음의 테스트는 필터 `RankAndFile`에 걸리지 않는다 [추측]).
5. `tools\package.bat` → 패키지 실행. 실행 중에 `netstat -ano | findstr :8000`으로 LISTENING 줄이 0개인지 본다. Shipping에 MCP가 없는지는 켜기 전과 켠 뒤의 `Manifest_*_Win64.txt`, Shipping receipt의 `BuildPlugins`, Shipping 실행 파일의 문자열을 비교해 본다. 실행 중에는 게임 프로세스의 TCP 대기 소켓과 `:8000`을 본다(11/21 고침: 처음 적은 `dir /s /b … | findstr …` 0줄 기준은 맞지 않는다. 의존 셋의 `.uplugin`은 켜기 전부터 pak 목록에 있어 0줄이 나오지 않고, `dir`은 pak 안을 보지 못한다).
6. 하나라도 실패하면 `.uproject`의 항목을 지우고(끄고) 사람에게 알린다.

### 6.3 임포트 순서 (6.1 3번이 ①일 때)

1. `Skeleton_Minion.glb`를 먼저 임포트한다: Skeleton 칸은 비움(새로 만듦), 애니메이션 가져오기 켬, 물리 에셋 끔 [제안], 본 아래 메시를 스켈레탈로 합치기 끔·스태틱 메시 가져오기 끔(졸개 파일에는 본 아래 메시가 없어 영향 없음). 95개 애니메이션이 이 한 번에 들어온다.
2. 생긴 Skeleton 이름을 에디터에서 `SKEL_` 규칙으로 바꾼다(`Content/AGENTS.md` 74줄, 옮기기·이름 바꾸기는 에디터에서, 12줄).
3. `Knight.glb`를 임포트한다: Skeleton 칸에 1에서 생긴 Skeleton, 애니메이션 가져오기 끔, 본 아래 메시 설정은 6.1 4번에 따라.
4. 확인(MCP 조회와 캡처): Skeleton 에셋 1개, 스켈레탈 메시 2개, AnimSequence 95개, 8.2 표의 15개 이름이 모두 있음, 두 메시의 `CaptureAssetImage`. Skeleton이 2개면 공유 실패다. 진행하지 않고 알린다(`Content/AGENTS.md` 30줄).
5. 나머지 7종은 받은 뒤 3과 같은 방법으로 한다(3절 남은 [미확인] 1).

### 6.4 Skeleton 공유가 안 될 때 (대응은 사람이 정한다, `Content/AGENTS.md` 34줄)

- 먼저 원인을 본다: 임포트 메시지의 "Incompatible skeleton"과 어느 본에서 사슬이 갈렸는지. 이 두 파일은 관절이 같아서(3절) 원인은 설정(잘못 지정한 Skeleton, 본 아래 메시가 본으로 바뀜 등)일 가능성이 크다 [추측].
- IK Retargeter: `IKRig` 플러그인이 기본으로 켜져 있어 새 플러그인 없이 쓸 수 있다(2절). 병과별 Skeleton마다 IK Rig를 만들고, 해골 파일의 애니메이션을 대상 Skeleton으로 옮긴 에셋을 만든다.
- 그래도 안 되면 `docs/design.md` 17절의 대응: 병과별 Skeleton에 같은 이름의 애니메이션을 따로 임포트하고 데이터 에셋에서 고른다.

## 7. 수정안

모두 [제안]이다. 적용은 승인 뒤에 한다. 규칙 파일(`AGENTS.md`)은 승인을 받아야 고칠 수 있다(루트 `AGENTS.md` 2절).

### 수정안 1. `Source/AGENTS.md` 50줄 (BindWidget의 [미확인])

지금:

> - [미확인] `BindWidget` 지정자의 동작은 공식 문서보다 포럼 사례로 알려져 있다. 5.8 엔진 헤더에서 확인하고 이 표시를 지운다.

바꾼 뒤:

> - `BindWidget`은 5.8 엔진에서 이렇게 처리된다. 에디터의 위젯 블루프린트 컴파일러가 부모 C++ 클래스의 `BindWidget` 속성마다 같은 이름·같은 형의 위젯이 있는지 검사하고, 없으면 오류(새로 만든 블루프린트는 경고)를 낸다(`Engine/Source/Editor/UMGEditor/Private/WidgetBlueprintCompiler.cpp` 1257~1291줄). `BindWidgetOptional`은 없어도 된다. 런타임은 위젯 트리의 위젯마다 이름이 같은 객체 속성에 위젯을 넣는다(`Engine/Source/Runtime/UMG/Private/WidgetBlueprintGeneratedClass.cpp` 270~275줄). 이름이 연결의 기준이므로 C++ 위젯 클래스의 다른 객체 속성 이름이 디자이너의 위젯 이름과 겹치지 않게 한다.

### 수정안 2. `Source/AGENTS.md` 16줄 (근거 보강, 선택)

지금:

> - Runtime 모듈은 Editor 모듈의 코드를 부르지 않는다. Editor 타입 모듈은 런타임 빌드에 컴파일되지 않는다([Epic: Unreal Engine Modules](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-modules)).

바꾼 뒤:

> - Runtime 모듈은 Editor 모듈의 코드를 부르지 않는다. Editor 타입 모듈은 런타임 빌드에 컴파일되지 않는다([Epic: Unreal Engine Modules](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-modules), 5.8 UBT `Engine/Source/Programs/UnrealBuildTool/Configuration/Descriptors/ModuleDescriptor.cs` 798~800줄: Editor 타입은 대상이 Editor일 때만 컴파일된다).

### 수정안 3. `Data/AGENTS.md` 55줄 (CSV 임포트의 [미확인])

지금:

> - 첫 열(`id` 또는 `key`)이 DataTable의 행 이름이 된다. [미확인] 5.8의 CSV 임포트가 첫 열을 행 이름으로 쓰는지는 엔진 소스로 확인하고 이 표시를 지운다.

바꾼 뒤:

> - 첫 열(`id` 또는 `key`)이 DataTable의 행 이름이 된다. 5.8의 CSV 임포트는 `ImportKeyField`가 비어 있으면 첫 열을 행 이름으로 쓰고, 그 열의 값은 행 구조체의 필드에 넣지 않는다(`Engine/Source/Runtime/Engine/Private/DataTableCSV.cpp` 151~164, 196, 236~242줄). 행 이름이 겹치면 그 행을 버린다(같은 파일 215~223줄). `defs/rules.csv`는 첫 열이 `maxTicks`라 이 규칙에 맞지 않는다. DataTable로 임포트하는 방법은 `RankEditor` 임포터를 만들 때 정한다 [미정].

### 수정안 4. `docs/design.md` 847줄 (14.1 Unreal MCP 행)

지금:

> | Unreal MCP (11/17) | 실험 기능. 본체는 기본으로 켜져 있고 도구 묶음 플러그인은 직접 켠다. 쿠킹한 Shipping 빌드도 MCP 서버를 띄울 수 있다([Epic: Unreal MCP](https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor)). 결정: 도구 묶음만 플러그인 금지의 예외로 켜고, Shipping에서는 뺀다. [미확인] 빼는 설정 위치(플러그인 대상 설정 또는 서버 자동 시작) | 도구 묶음을 켠 뒤 빈 Shipping 패키지 실행. 그 패키지에서 `127.0.0.1:8000`에 서버가 뜨지 않는지 확인 |

바꾼 뒤:

> | Unreal MCP (11/17, 11/21 확인) | 실험 기능. 플러그인 이름은 `ModelContextProtocol`(표시 이름 "Unreal MCP", `Engine/Plugins/Experimental/ModelContextProtocol`). 5.8.4에서는 본체도 기본으로 꺼져 있다(`.uplugin` 18줄 `"EnabledByDefault": false`. Epic 문서(5.9)의 "기본으로 켜져 있다"와 다르다). 도구는 `Engine/Plugins/Experimental/Toolsets/`의 도구 묶음 플러그인이 등록한다. 본체에 Runtime 모듈 2개가 있어 대상을 제한하지 않고 켜면 Shipping에 들어간다. 빼는 곳: `.uproject`의 플러그인 항목에 `"TargetAllowList": ["Editor"]`(UBT `PluginReferenceDescriptor.cs` 405~419줄, `UEBuildTarget.cs` 5840~5843줄. 의존 플러그인도 따라 빠진다). 서버 자동 시작은 5.8.4에서 기본으로 꺼져 있고 Editor 모듈에서만 부른다. 의존에 `ScriptableToolsFramework`는 없다. 상세: `docs/review/stage2-engine-investigation-2026-11-21.md` 1절 | 켠 뒤 빈 Shipping 패키지 실행. 그 패키지에서 `127.0.0.1:8000`에 서버가 뜨지 않는지, 스테이징 폴더에 MCP 파일이 없는지 확인 |

### 수정안 5. `docs/design.md` 849줄 (14.1 에셋 행)

지금:

> | 에셋 | KayKit GLB는 glTF 2.0, 정점당 본 영향 최대 4개라 5.8 glTF 임포터의 영향 수 제한 문제([포럼](…))와 무관하다(8.6절) | GLB 2개 임포트, Skeleton 공유, 애니메이션 재생 |

바꾼 뒤(첫 칸 끝에 덧붙임):

> | 에셋 | KayKit GLB는 glTF 2.0, 정점당 본 영향 최대 4개라 5.8 glTF 임포터의 영향 수 제한 문제([포럼](…))와 무관하다(8.6절). 5.8.4에서 glTF 임포트는 기본으로 켜진 Interchange(`Engine/Plugins/Interchange/Runtime/Interchange.uplugin` 13줄)가 하고, 새 플러그인은 필요 없다. MCP 도구 묶음의 스켈레탈 메시 임포트 도구는 FBX 전용이라 GLB를 받지 않는다(`EditorToolset`의 `skeletal_mesh.py` 50~64줄). [측정: 11/21, JSON 청크] `Knight.glb`와 `Skeleton_Minion.glb`는 `JOINTS_1`이 없고 관절 41개의 이름·순서·부모가 같다 | GLB 2개 임포트, Skeleton 공유, 애니메이션 재생 |

### 수정안 6. `docs/design.md` 434줄 (8.4 BindWidget의 [미확인])

지금(문장 일부):

> [미확인] `BindWidget`은 공식 문서보다 포럼과 커뮤니티 위키에 설명이 있다([포럼](…), [커뮤니티 위키](…)). 5.8 엔진 헤더에서 확인한다.

바꾼 뒤:

> `BindWidget`은 에디터의 위젯 블루프린트 컴파일러가 이름과 형을 검사하고(`Engine/Source/Editor/UMGEditor/Private/WidgetBlueprintCompiler.cpp` 1257~1291줄), 런타임은 이름이 같은 속성에 위젯을 넣는다(`Engine/Source/Runtime/UMG/Private/WidgetBlueprintGeneratedClass.cpp` 270~275줄). 11/21 5.8.4 소스로 확인했다.

### 수정안 7. `docs/design.md` 344줄 (7.3 2번의 [미확인])

지금(문장 일부):

> [미확인] `UDataTable::CreateTableFromCSVString`(5.8 엔진 소스로 확인)을 쓴다.

바꾼 뒤:

> `UDataTable::CreateTableFromCSVString`을 쓴다(5.8.4 `Engine/Source/Runtime/Engine/Classes/Engine/DataTable.h` 351줄, `WITH_EDITOR` 밖. 첫 열이 행 이름이 되고 그 값은 필드에 들어가지 않는다, `DataTableCSV.cpp` 151~164, 236~242줄). [미정] `units.csv`의 `magic` 열은 속성 `bMagic`과 이름이 달라 이 함수로는 찾지 못하고, `rules.csv`는 첫 열이 `maxTicks`다. 이 함수를 쓸지, `RankGame`의 읽기 함수로 행을 만들어 `UDataTable::AddRow`로 넣을지 정한다.

### 수정안 8. `docs/design.md` 454줄 (8.6 Skeleton 공유)

지금:

> - Skeleton을 함께 쓰려면 본의 계층 순서와 이름이 같아야 한다([Epic: Importing Skeletal Meshes Using FBX](…)). [판단] 관절 이름이 같다는 것은 확인됐으므로 가능성이 높다. 계층 순서는 임포트해 봐야 안다.

바꾼 뒤:

> - Skeleton을 함께 쓰려면 본의 계층 순서와 이름이 같아야 한다([Epic: Importing Skeletal Meshes Using FBX](…)). 5.8.4의 Interchange는 본마다 이름과 부모 사슬(루트 본 이름 포함)이 Skeleton과 같은지 본다(`Engine/Plugins/Interchange/Runtime/Source/Import/Private/Mesh/InterchangeSkeletonHelper.cpp` 90~180, 422~447줄). [측정: 11/21, GLB JSON 청크] `Knight.glb`와 `Skeleton_Minion.glb`는 관절 41개의 이름, 순서, 부모, 노드 로컬 변환이 같다. 나머지 7종은 받은 뒤 확인한다.

### 수정안 9. `docs/design.md` 461줄 (8.6 기사의 무기 수)

지금(문장 일부):

> (기사는 방패 4개와 검 2개)

바꾼 뒤:

> (기사는 방패 4개와 검 3개: `1H_Sword_Offhand`, `1H_Sword`, `2H_Sword`. 투구와 망토도 `head`, `chest` 본 아래 별도 메시다)

### 수정안 10. 6.1 1번을 정한 뒤 (규칙 파일, 승인 필요)

6.1 1번의 답에 따라 문장이 달라지므로 B안일 때의 모양만 적는다.

- 루트 `AGENTS.md` 88줄. 지금: "예외는 Unreal MCP와 그 도구 묶음(toolset) 플러그인 하나다(2026-11-17 결정)." → 바꾼 뒤: "예외는 Unreal MCP(`ModelContextProtocol`)와 도구 묶음 플러그인 `EditorToolset`, `UMGToolSet`, 그리고 이들이 의존으로 켜는 플러그인이다(2026-11-17 결정, 11/21 보완). `.uproject`의 항목마다 `"TargetAllowList": ["Editor"]`를 단다."
- `Content/AGENTS.md` 8줄. 지금: "MCP로 하는 일: 위젯 블루프린트와 애니메이션 블루프린트 생성·설정(…), 캐릭터 임포트, 데이터 에셋 연결, 전장 액터·머티리얼 배치." → 바꾼 뒤: "MCP로 하는 일: 위젯 블루프린트 생성·설정(부모는 C++ 클래스, 이벤트 그래프는 비움), 데이터 에셋 연결, 전장 액터·머티리얼 배치, 임포트 결과 확인. 5.8.4 도구 묶음으로 할 수 없는 일(GLB 임포트, 애니메이션 블루프린트 생성)은 11줄의 절차로 사람에게 넘긴다."
- `Content/AGENTS.md` 35줄의 "임포트할 때 애니메이션 블루프린트를 자동으로 만드는 옵션이 있으면 끈다"는 Interchange에 그런 옵션이 없으므로(2절) 지워도 된다. 남겨도 해는 없다.
- `docs/decisions.md`: 11/17 MCP 행을 보완하는 새 행(본체도 켜야 함, 고른 도구 묶음, `TargetAllowList`)은 본인이 정한 뒤 적는다.

## 부록 A. 실행한 명령

모두 읽기 전용이다. 엔진 경로는 엔진 루트 기준으로 적는다.

- `cat Engine/Build/Build.version`
- `find Engine/Plugins -iname "*.uplugin"`에 `grep`(MCP·toolset·interchange·gltf 이름)
- `cat -n`으로 `.uplugin` 읽기: `ModelContextProtocol`, `ToolsetRegistry`, `AllToolsets`, `AIAssistant`, `EditorToolset`, `UMGToolSet`, `AnimationAssistantToolset`, `Interchange`, `InterchangeEditor`, `InterchangeAssets`
- `grep -rl "ScriptableToolsFramework"`, `grep -rl "\"ModelContextProtocol\""`(`Engine/Plugins`의 `.uplugin`)
- `node`(임시 스크립트 1): `Engine/Plugins`의 `.uplugin` 895개를 읽어 `ModelContextProtocol`, `EditorToolset`, `UMGToolSet`, `AnimationAssistantToolset`, `AllToolsets`의 의존을 따라감
- `grep`, `sed -n`, `cat -n`으로 엔진 소스 읽기: 부록 B의 파일들
- `find`로 `Engine/Plugins/Experimental/ModelContextProtocol/Binaries`, `Intermediate` 목록
- `node`(임시 스크립트 2)로 `SourceArt/KayKit/Adventurers/Knight.glb`, `SourceArt/KayKit/Skeletons/Skeleton_Minion.glb`의 JSON 청크 읽기(요약, 비교, 8.2 이름 대조, 관절 로컬 변환 비교)
- `sha256sum`(GLB 2개, LICENSE 2개), `cat`(LICENSE 2개)
- 저장소: `cat -n RankAndFile.uproject`, `Source/*.Target.cs`, `tools/package.bat`, `git rev-parse --short HEAD`, `git status --short`, `git check-ignore -v`, `grep`(`Config/*.ini`, `tools/*.bat`, `Source/RankGame`, `Data/defs/*.csv` 머리 줄)
- Epic 문서 1쪽 가져오기(하위 에이전트, 쪽 제목 5.9)
- `node --version`

## 부록 B. 근거로 인용한 엔진 파일 (66개, 엔진 루트 기준)

설정·버전(2): `Engine/Build/Build.version`, `Engine/Config/BaseEngine.ini`

플러그인 기술자(17): `Engine/Plugins/Experimental/ModelContextProtocol/ModelContextProtocol.uplugin`, `Engine/Plugins/Experimental/ToolsetRegistry/ToolsetRegistry.uplugin`, `Engine/Plugins/Experimental/Toolsets/AllToolsets/AllToolsets.uplugin`, `…/Toolsets/EditorToolset/EditorToolset.uplugin`, `…/Toolsets/UMGToolSet/UMGToolSet.uplugin`, `…/Toolsets/AnimationAssistantToolset/AnimationAssistantToolset.uplugin`, `Engine/Plugins/Experimental/PythonScriptPlugin/PythonScriptPlugin.uplugin`, `Engine/Plugins/Editor/EditorScriptingUtilities/EditorScriptingUtilities.uplugin`, `Engine/Plugins/Developer/Sandbox/FileSandbox/FileSandbox.uplugin`, `Engine/Plugins/Editor/EngineAssetDefinitions/EngineAssetDefinitions.uplugin`, `Engine/Plugins/Editor/ContentBrowser/ContentBrowserFileDataSource/ContentBrowserFileDataSource.uplugin`, `Engine/Plugins/Editor/ScriptableToolsEditorMode/ScriptableToolsEditorMode.uplugin`(문자열 검색만), `Engine/Plugins/MetaHuman/MetaHumanCharacter/MetaHumanCharacter.uplugin`(문자열 검색만), `Engine/Plugins/Interchange/Runtime/Interchange.uplugin`, `Engine/Plugins/Interchange/Editor/InterchangeEditor.uplugin`, `Engine/Plugins/Interchange/Assets/InterchangeAssets.uplugin`, `Engine/Plugins/Animation/IKRig/IKRig.uplugin`

UBT·Projects(6): `Engine/Source/Programs/UnrealBuildTool/Configuration/Descriptors/ModuleDescriptor.cs`, `…/Descriptors/PluginReferenceDescriptor.cs`, `Engine/Source/Programs/UnrealBuildTool/Configuration/UEBuildTarget.cs`, `Engine/Source/Programs/UnrealBuildTool/System/Plugins.cs`, `Engine/Source/Runtime/Projects/Private/PluginReferenceDescriptor.cpp`, `Engine/Plugins/Experimental/ModelContextProtocol/Intermediate/Build/Win64/x64/UnrealGame/Shipping/ModelContextProtocol/ModelContextProtocol.precompiled`(있음만 확인)

MCP와 도구 묶음(14): `Engine/Plugins/Experimental/ModelContextProtocol/Source/ModelContextProtocolEngine/Public/ModelContextProtocolSettings.h`, `…/ModelContextProtocolEngine/Private/ModelContextProtocolSettings.cpp`, `…/ModelContextProtocolEngine/Private/ModelContextProtocolEngineModule.cpp`, `…/ModelContextProtocolEngine/Private/ModelContextProtocolClientConfig.cpp`, `…/ModelContextProtocolEditor/Private/ModelContextProtocolEditor.cpp`, `…/ModelContextProtocol/Private/ModelContextProtocolModule.cpp`, `…/ModelContextProtocol/Private/ModelContextProtocolAnalytics.cpp`, `…/ModelContextProtocol/Public/ModelContextProtocol.h`, `Engine/Plugins/Experimental/Toolsets/EditorToolset/Content/Python/editor_toolset/toolsets/skeletal_mesh.py`, `…/toolsets/asset.py`, `…/toolsets/blueprint.py`, `…/toolsets/data_asset.py`, `…/EditorToolset/Source/EditorToolset/Private/EditorAppToolset.h`, `Engine/Plugins/Experimental/Toolsets/UMGToolSet/Source/UMGToolSet/Private/UMGToolSet.h`

코어·콘솔·HTTP(3): `Engine/Source/Runtime/Core/Public/HAL/IConsoleManager.h`, `Engine/Source/Runtime/Core/Public/Misc/Build.h`, `Engine/Source/Runtime/Online/HTTPServer/Private/HttpServerConfig.h`

임포트(13): `Engine/Source/Editor/UnrealEd/Private/Fbx/FbxFactory.cpp`, `Engine/Source/Developer/AssetTools/Private/AssetTools.cpp`, `Engine/Source/Runtime/Interchange/Engine/Private/InterchangeManager.cpp`, `Engine/Plugins/Interchange/Runtime/Source/Import/Public/Gltf/InterchangeGltfTranslator.h`, `…/Import/Private/Gltf/InterchangeGltfTranslator.cpp`, `…/Import/Private/Gltf/InterchangeGltfAnimation.cpp`, `…/Import/Private/InterchangeImportModule.cpp`, `…/Import/Private/Mesh/InterchangeSkeletonHelper.cpp`, `…/Import/Private/Mesh/InterchangeSkeletalMeshFactory.cpp`, `…/Pipelines/Private/InterchangeGenericSkeletalMeshPipeline.cpp`, `…/Pipelines/Public/InterchangeGenericAssetsPipelineSharedSettings.h`, `…/Pipelines/Public/InterchangeGenericAnimationPipeline.h`, `…/Pipelines/Public/InterchangeGenericMeshPipeline.h`

애니메이션·UMG·DataTable(11): `Engine/Plugins/Animation/IKRig/Source/IKRig/Public/Retargeter/IKRetargeter.h`, `Engine/Source/Runtime/Engine/Classes/Components/SkeletalMeshComponent.h`, `Engine/Source/Runtime/Engine/Private/Components/SkeletalMeshComponent.cpp`, `Engine/Source/Runtime/UMG/Public/Components/Widget.h`, `Engine/Source/Editor/UMGEditor/Private/WidgetBlueprintEditorUtils.cpp`, `Engine/Source/Editor/UMGEditor/Private/WidgetBlueprintCompiler.cpp`, `Engine/Source/Runtime/UMG/Private/WidgetBlueprintGeneratedClass.cpp`, `Engine/Source/Runtime/Engine/Private/DataTableCSV.cpp`, `Engine/Source/Runtime/Engine/Classes/Engine/DataTable.h`, `Engine/Source/Runtime/Engine/Private/DataTable.cpp`, `Engine/Source/Runtime/Engine/Private/DataTableUtils.cpp`

합계: 설정·버전 2 + 플러그인 기술자 17 + UBT·Projects 6 + MCP와 도구 묶음 14 + 코어 3 + 임포트 13 + 애니메이션·UMG·DataTable 11 = 66개. 그중 문자열 검색만 한 2개(`ScriptableToolsEditorMode`, `MetaHumanCharacter`)와 있음만 확인한 1개(`.precompiled`)를 뺀 63개에 줄 번호와 인용을 붙였다.

## 11/21 추가 확인: UE 5.8의 AI 기능 (작업 세션)

본인이 "AI 기능과 관련된 부분들 미확인된 부분들은 정확한 자료 찾아보고 다시 보고해"라고 해서 엔진 소스(5.8.4 설치본)와 공식 문서로 다시 확인했다. 엔진 경로는 `Engine/Plugins/Experimental/` 기준이다.

| 항목 | 확인한 것 | 근거 |
|---|---|---|
| 공식 문서 | Unreal MCP는 실험 기능이다. "Enable the Unreal MCP and All Toolsets plugins", 도구 묶음은 하나씩 켤 수도 있다. Toolset Registry는 자동으로 켜진다. HTTP와 SSE만, 기본 `127.0.0.1:8000`, 인증 없음. 자동 시작은 기본 꺼짐 | [Epic: Unreal MCP in Unreal Editor (5.8)](https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor?application_version=5.8) |
| 의존 플러그인 | 도구 묶음(UMG, Editor, Animation)과 AI Assistant는 모두 `ToolsetRegistry`에 기대고, `ToolsetRegistry`는 `PythonScriptPlugin`, `EditorScriptingUtilities`, `FileSandbox`에 기댄다. 그래서 도구 묶음 하나만 켜도 Python 플러그인이 함께 켜진다 | `ToolsetRegistry/ToolsetRegistry.uplugin:30-38`, `Toolsets/EditorToolset/EditorToolset.uplugin:28`, `Toolsets/UMGToolSet/UMGToolSet.uplugin:29`, `AIAssistant/AIAssistant.uplugin:30-38` |
| UMG ToolSet | C++ 도구(`AICallable`): 위젯 블루프린트 만들기(부모 클래스 지정), 위젯 넣기·옮기기·지우기·이름 바꾸기, 변수로 표시, 이벤트 속성 묶기, 감싸기, 위젯 트리 읽기 | `Toolsets/UMGToolSet/Source/UMGToolSet/Private/UMGToolSet.h:286-506` |
| Editor 도구 묶음 | Python 도구: 속성 읽기·쓰기(`object.set_properties`), 데이터 에셋 만들기, 범용 임포트(`asset.import_asset`, 팩토리를 비우면 엔진이 Interchange 경로를 고른다), 스켈레탈 메시 임포트(`skeletal_mesh.import_file`, FBX 전용), 블루프린트 만들기(`BlueprintFactory`, 일반 블루프린트), 제한된 환경에서 코드 실행(`programmatic`) | `Toolsets/EditorToolset/Content/Python/editor_toolset/toolsets/`의 `object.py:81`, `data_asset.py:16`, `asset.py:22-60`, `skeletal_mesh.py:16-55`, `blueprint.py:177-193`, `programmatic.py:2` |
| 애니메이션 블루프린트 | 만드는 도구가 없다. `AnimBlueprint` 검색 0건. Animation 도구 묶음은 Sequencer와 Control Rig 편집용이다 | `Toolsets/EditorToolset/.../toolsets/*.py` 검색, `Toolsets/AnimationAssistantToolset/Content/Python/animation_toolset/toolsets/`의 파일 이름(sequencer, controlrig, keyframing 등) |
| AI Assistant | 에디터 안의 웹 브라우저 위젯에 Epic 페이지 `https://dev.epicgames.com/community/assistant/embedded`를 띄우고, Toolset Registry로 에디터 도구를 부른다. 5.8 공식 문서에서 이 플러그인 설명은 찾지 못했다 | `AIAssistant/Source/AIAssistant/Private/AIAssistantConfig.cpp:18`, `AIAssistant/Source/AIAssistant/AIAssistant.Build.cs:35, 75`, `AIAssistant/Source/AIAssistant/Private/AIAssistant.cpp:93` |
| MCP 분석 | 세션 시작·끝, 도구 호출(세션 id, 도구 묶음·도구 이름의 해시, 걸린 시간, 성공 여부)을 에디터의 엔진 분석(`FEngineAnalytics`)으로 보낸다. 콘솔 변수 `ModelContextProtocol.EnableAnalytics`(기본 true)로 끈다 | `ModelContextProtocol/Source/ModelContextProtocol/Private/ModelContextProtocolAnalytics.cpp:15-19, 88-114`, `ModelContextProtocol/Source/ModelContextProtocolEngine/Private/EngineAnalyticsProviderProxy.h:13-34` |

### 11/21 다시 조사: 남았던 [미확인]을 모두 확인함

본인이 "미확인 사실이 없게 다시 조사해와"라고 해서 엔진 소스, Epic 공식 FAQ와 추가 약관으로 확인했다. 엔진 경로는 `Engine/` 기준이다.

| 질문 | 확인한 것 | 근거 |
|---|---|---|
| MCP를 켜면 무엇이 함께 켜지나 | MCP 본체도 `ToolsetRegistry`에 의존하고, 그것이 `PythonScriptPlugin`, `EditorScriptingUtilities`, `FileSandbox`에 의존한다. 넷 다 자기 `.uplugin`에서는 기본으로 꺼져 있다. 그러나 `PythonScriptPlugin`, `EditorScriptingUtilities`, `FileSandbox`는 엔진 기본 플러그인(`RigVM`, `PluginUtils`, `ConcertSyncClient`)이 이미 켜고 있어, MCP를 켜서 새로 켜지는 것은 `ToolsetRegistry` 하나다(11/21 측정: 켜기 전 에디터 로그 마운트 246개 → 켠 뒤 250개) | `Plugins/Experimental/ModelContextProtocol/ModelContextProtocol.uplugin:57`, `Plugins/Experimental/ToolsetRegistry/ToolsetRegistry.uplugin:30-38`, `Plugins/Experimental/PythonScriptPlugin/PythonScriptPlugin.uplugin`(EnabledByDefault false), `Plugins/Editor/EditorScriptingUtilities/EditorScriptingUtilities.uplugin`(false), `Plugins/Developer/Sandbox/FileSandbox/FileSandbox.uplugin`(false) |
| GLB를 MCP의 범용 임포트로 가져올 수 있나 | 된다. Interchange 플러그인(기본 켜짐)이 시작할 때 Interchange 임포트를 켜고, 팩토리를 지정하지 않은 임포트 작업은 Interchange로 간다. glTF 번역기는 `.gltf`, `.glb`를 받고 메시, 애니메이션, 머티리얼, 스킨 메시와 관절을 만든다 | `Plugins/Interchange/Runtime/Interchange.uplugin:13`, `Plugins/Interchange/Runtime/Source/Import/Private/InterchangeImportModule.cpp:179`, `Source/Developer/AssetTools/Private/AssetTools.cpp:3567-3569`, `Plugins/Interchange/Runtime/Source/Import/Private/Gltf/InterchangeGltfTranslator.cpp:237-263, 470-494` |
| 임포트할 때 Skeleton을 함께 쓰게 지정할 수 있나 | MCP로는 안 된다. Interchange의 기본 파이프라인에 Skeleton 설정이 있지만, 임포트 작업의 옵션은 Interchange로 넘어가지 않는다(자동 여부, 덮어쓰기, 대상 이름만 넘긴다). 사람이 에디터의 임포트 창에서 Skeleton을 고르거나, 프로젝트의 기본 파이프라인 설정을 바꿔야 한다 | `Plugins/Interchange/Runtime/Source/Pipelines/Public/InterchangeGenericAssetsPipelineSharedSettings.h:203`, `Source/Developer/AssetTools/Private/AssetTools.cpp:3836-3870` |
| 애니메이션 블루프린트를 MCP로 만들 수 있나 | 안 된다. 모든 도구 묶음(AllToolsets가 켜는 21개 포함)과 AI Assistant에 `AnimBlueprint` 0건. 블루프린트 만들기는 일반 `BlueprintFactory`다. 코드 실행 도구는 등록된 도구와 표준 모듈 6개(json, math, datetime, copy, re, time)만 쓸 수 있어 `unreal` 모듈에 손댈 수 없다 | `Plugins/Experimental/Toolsets/` 검색, `Plugins/Experimental/Toolsets/AllToolsets/AllToolsets.uplugin`, `.../EditorToolset/Content/Python/editor_toolset/toolsets/blueprint.py:177-193`, `.../programmatic.py:52-81, 505-532` |
| 위젯의 위치·크기·글꼴을 MCP로 바꿀 수 있나 | 된다. 속성 설정 도구가 JSON을 구조체와 인스턴스 하위 객체까지 넣는다. 위젯의 `Slot`은 `Instanced`, `EditAnywhere` 속성이라 위치·크기를 바꿀 수 있고, 글꼴도 위젯의 구조체 속성이다 | `.../editor_toolset/toolsets/object.py:81-94`, `Plugins/Experimental/ToolsetRegistry/Source/ToolsetRegistry/Private/ToolsetRegistry/ToolsetLibrary.cpp:182-186`, `Source/Runtime/UMG/Public/Components/Widget.h:263-264` |
| AI Assistant는 무엇인가 | Epic Developer Assistant(Epic의 웹 AI 도우미)를 에디터 패널로 띄우는 플러그인이다(5.7부터 플러그인으로 제공). 질문 답변과 코드 생성을 한다. 에디터에서 페이지로 엔진 버전, 언어, 커서 아래 항목과 글자, 창·탭 이름을 보내고, "현재 프로젝트와 사용자 정보", "붙어 있는 편집기 정보"를 돌려주는 도구 2개를 가진다 | `Plugins/Experimental/AIAssistant/Source/AIAssistant/Private/AIAssistantConfig.cpp:18`, `AIAssistantSlateQuerier.cpp:200-344`, `AIAssistantToolset.h:84-92`, [Unreal Engine FAQ: 에픽 디벨로퍼 어시스턴트](https://www.unrealengine.com/en-US/faq#epicdeveloperassistant) |
| AI Assistant의 로그인, 비용, 데이터 | 로그인은 하지 않아도 쓸 수 있다(로그인하지 않은 대화도 저장 기간이 정해져 있다). API 키는 필요 없다. 프롬프트로 학습하지 않고, 상용 모델과 자체 모델을 쓴다. 대화는 요청 처리에만 AI 제공업체에 넘긴다. 기록 보관 기간은 FAQ(로그인하지 않으면 최대 30일)와 추가 약관(로그인하면 최대 6개월, 하지 않으면 세션이 끝난 뒤 곧 지움)이 다르게 적는다. FAQ는 "프롬프트에 입력된 내용만 읽는다"고 하지만 플러그인은 위의 편집기 맥락을 함께 보낸다. 요금은 FAQ와 추가 약관 어디에도 없다. 결과물의 고유한 표현은 사용자가 갖고, 결과물을 AI 학습에 쓰면 안 된다 | [Unreal Engine FAQ](https://www.unrealengine.com/en-US/faq#epicdeveloperassistant), [Developer Assistant Supplemental Terms](https://dev.epicgames.com/community/assistant/supplemental-terms)(최종 개정 2025-06-03) |
| MCP 분석은 언제 보내나 | 에디터의 "사용 데이터 보내기"(`bSendUsageData`)가 켜져 있고 콘솔 변수 `ModelContextProtocol.EnableAnalytics`가 true일 때만 엔진 분석으로 보낸다. 둘 중 하나를 끄면 보내지 않는다 | `Source/Editor/UnrealEd/Private/EditorServer.cpp:1231-1234`, `Source/Runtime/Engine/Private/EngineAnalytics.cpp:107-116`, `Plugins/Experimental/ModelContextProtocol/Source/ModelContextProtocol/Private/ModelContextProtocolAnalytics.cpp:15-19` |
| Shipping에서 빠지나 | `.uproject`의 플러그인 항목에 대상 제한(`TargetAllowList`)을 Editor로 두면 UBT가 게임 대상에서 그 플러그인을 건너뛴다. 공식 문서는 Shipping 빌드도 코드가 서버를 시작하면 MCP를 띄울 수 있다고 적는다. 실제 패키지 확인은 켤 때 규칙(빈 Shipping 패키지로 확인)대로 한다 | `Source/Programs/UnrealBuildTool/Configuration/UEBuildTarget.cs:5840-5843`, [Epic: Unreal MCP in Unreal Editor (5.8)](https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor?application_version=5.8) |

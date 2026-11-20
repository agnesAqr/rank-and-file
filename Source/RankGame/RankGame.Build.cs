using UnrealBuildTool;

// 런 진행, 전투 재생, UI. 의존은 RankCore 한 방향이다(Source/AGENTS.md).
// Json은 회귀 테스트 기록 파일(regression_battles_v1.json)을 쓰고 읽는 데 쓴다. 쓰는 곳과 읽는 곳을 이 모듈 하나에 두려는 것이다(11/20 알림).
public class RankGame : ModuleRules
{
	public RankGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "RankCore" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Json" });
	}
}

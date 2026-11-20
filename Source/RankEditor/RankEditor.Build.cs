using UnrealBuildTool;

// 콘텐츠 임포터, BalanceSim·RankRegression 커맨들릿. Editor 모듈이라 Shipping에 들어가지 않는다(Source/AGENTS.md).
public class RankEditor : ModuleRules
{
	public RankEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "RankCore", "RankGame" });
	}
}

using UnrealBuildTool;

// 전투 규칙 모듈. 엔진 Core 외에는 의존하지 않는다(Source/AGENTS.md).
public class RankCore : ModuleRules
{
	public RankCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core" });
	}
}

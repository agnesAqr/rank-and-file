using UnrealBuildTool;

// 자동화 테스트. Editor 모듈이라 Shipping에 들어가지 않는다(Source/AGENTS.md).
// Json은 손계산 시나리오(T5) 파일 Fixtures/hand_v1.json을 읽는 데만 쓴다(11/20 알림).
public class RankTests : ModuleRules
{
	public RankTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "Json", "RankCore", "RankGame" });
	}
}

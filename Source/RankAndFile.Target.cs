using UnrealBuildTool;

// 게임 대상(Development, Test, Shipping). Runtime 모듈(RankCore, RankGame)만 들어간다.
// .uproject에 적힌 모듈은 타입에 맞춰 UBT가 넣는다. 여기서는 주 게임 모듈만 적는다(엔진 템플릿 TP_Blank와 같은 형식).
public class RankAndFileTarget : TargetRules
{
	public RankAndFileTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("RankGame");
	}
}

using UnrealBuildTool;

// 에디터 대상(Development Editor). .uproject의 Editor 모듈(RankEditor, RankTests)은 이 대상에만 들어간다.
public class RankAndFileEditorTarget : TargetRules
{
	public RankAndFileEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("RankGame");
	}
}

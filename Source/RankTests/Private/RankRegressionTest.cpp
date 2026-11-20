#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RankBattle.h"
#include "RankRegressionData.h"
#include "RankTestUtils.h"

// T1 회귀 테스트(Source/RankTests/AGENTS.md). 고정 입력 20건을 돌려 기준으로 확정된 RankCore가 남긴 기록과 비교한다.
// 기록 파일은 tools\record_regression.bat으로만 만든다. 이 테스트는 기록을 만들거나 고치지 않는다.
// - 기록 파일이 없으면 "기준 확정 전"으로 실패한다. 건너뛰거나 통과로 처리하지 않는다.
// - 기록의 병과 수치·규칙 매개변수가 지금 Data/defs와 다르면 "기록이 지금 수치와 다르다"로 실패한다.
// - 입력은 RankGame의 변환 함수(RankRegression::ParseInputsCsv)로 읽는다. 난수기를 쓰지 않는다.

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags RankTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRankT1RegressionTest, "RankAndFile.Core.T1_Regression", RankTestFlags)

bool FRankT1RegressionTest::RunTest(const FString& Parameters)
{
	const FString RecordPath = RankTests::ProjectFile(TEXT("Source/RankTests/Fixtures/regression_battles_v1.json"));
	const FString InputsPath = RankTests::ProjectFile(TEXT("Source/RankTests/Fixtures/regression_inputs.csv"));

	RankTests::FDefs Defs;
	FString Error;
	if (!RankTests::LoadProjectDefs(Defs, Error))
	{
		AddError(FString::Printf(TEXT("T1: Data/defs를 읽지 못했다: %s"), *Error));
		return false;
	}
	FString InputsText;
	TArray<RankRegression::FBattleInputRow> Inputs;
	if (!FFileHelper::LoadFileToString(InputsText, *InputsPath) || !RankRegression::ParseInputsCsv(InputsText, Inputs, Error))
	{
		AddError(FString::Printf(TEXT("T1: 회귀 입력을 읽지 못했다: %s"), *Error));
		return false;
	}

	FString RecordText;
	if (!FPaths::FileExists(RecordPath) || !FFileHelper::LoadFileToString(RecordText, *RecordPath))
	{
		AddError(TEXT("T1: 기준 확정 전(기록 파일 Source/RankTests/Fixtures/regression_battles_v1.json이 없다. RankCore를 기준으로 확정한 뒤 tools\\record_regression.bat으로 만든다)"));
		return false;
	}
	RankRegression::FRecordFile Record;
	if (!RankRegression::ReadRecordJson(RecordText, Record, Error))
	{
		AddError(FString::Printf(TEXT("T1: 기록 파일을 읽지 못했다: %s"), *Error));
		return false;
	}

	TArray<FString> DefDiffs;
	RankRegression::DiffDefs(Record.Units, Record.Rules, TEXT("기록"), Defs.UnitRows, Defs.RulesRow, TEXT("Data/defs"), DefDiffs);
	if (DefDiffs.Num() > 0)
	{
		for (const FString& Diff : DefDiffs)
		{
			AddError(FString::Printf(TEXT("T1: %s"), *Diff));
		}
		AddError(TEXT("T1: 기록이 지금 수치와 다르다"));
		return false;
	}

	if (Record.Battles.Num() != Inputs.Num())
	{
		AddError(FString::Printf(TEXT("T1: 기록의 대전 수 %d, 입력의 대전 수 %d"), Record.Battles.Num(), Inputs.Num()));
	}

	int32 Passed = 0;
	for (const RankRegression::FBattleInputRow& Row : Inputs)
	{
		const RankRegression::FRecordedBattle* Expected = Record.Battles.FindByPredicate(
			[&Row](const RankRegression::FRecordedBattle& Battle) { return Battle.Id.Equals(Row.Id, ESearchCase::CaseSensitive); });
		if (Expected == nullptr)
		{
			AddError(FString::Printf(TEXT("T1 %s: 기록에 없다"), *Row.Id));
			continue;
		}
		if (Expected->AllyRows != Row.AllyRows || Expected->EnemyRows != Row.EnemyRows)
		{
			AddError(FString::Printf(TEXT("T1 %s: 입력이 기록 때와 다르다. 입력을 바꿨으면 승인을 받아 다시 기록한다"), *Row.Id));
			continue;
		}

		Rank::FBattleInput Input;
		Input.Side[0] = Row.Ally;
		Input.Side[1] = Row.Enemy;
		Input.Rules = Defs.Rules;
		Input.Defs = Defs.Units.GetData();
		Input.NumDefs = Defs.Units.Num();
		const Rank::FBattleResult Result = Rank::Simulate(Input, true);

		RankRegression::FRecord Actual;
		if (!RankRegression::MakeRecord(Result, Actual))
		{
			AddError(FString::Printf(TEXT("T1 %s: 로그 줄 수 %d와 이벤트 수 %d가 다르다"), *Row.Id, Result.Log.Num(), Result.NumEvents));
			continue;
		}
		TArray<FString> Diffs;
		RankRegression::DiffRecords(Expected->Record, Actual, Diffs);
		if (Diffs.Num() > 0)
		{
			AddError(FString::Printf(TEXT("T1 %s: %s"), *Row.Id, *FString::Join(Diffs, TEXT(" · "))));
			continue;
		}
		++Passed;
	}

	AddInfo(FString::Printf(TEXT("T1 회귀 %d/%d (기록 커밋 %s)"), Passed, Inputs.Num(), *Record.Commit));
	const bool bPassed = Inputs.Num() > 0 && Passed == Inputs.Num() && Record.Battles.Num() == Inputs.Num();
	if (!bPassed)
	{
		AddError(FString::Printf(TEXT("T1 회귀 %d/%d"), Passed, Inputs.Num()));
	}
	return bPassed;
}

#endif // WITH_DEV_AUTOMATION_TESTS

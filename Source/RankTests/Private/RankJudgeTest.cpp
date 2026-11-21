#include "Misc/AutomationTest.h"
#include "RankBattle.h"
#include "RankTestUtils.h"

// 플레이어 기준 판정(Rank::Judge)이 입력 오류를 판정하지 않는지 본다(docs/rules.md §5.2, 2026-11-20 결정).
// 기대값의 출처는 §5.2의 문장이다: 입력 오류(§3.6)는 판정하지 않고, 코어 결과 A, B, Draw, Timeout은 판정한다.
// 손계산이 아니라 성질이다. 화면 없이 돈다.

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags RankTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	// 병과 수치에 없는 키. 테스트를 시작할 때 Data/defs에 정말 없는지 확인한다.
	constexpr uint8 UnknownKey = 'X';
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRankJudgeInvalidIsNotJudgedTest, "RankAndFile.Core.Judge_InvalidIsNotJudged", RankTestFlags)

bool FRankJudgeInvalidIsNotJudgedTest::RunTest(const FString& Parameters)
{
	RankTests::FDefs Defs;
	FString Error;
	if (!RankTests::LoadProjectDefs(Defs, Error))
	{
		AddError(FString::Printf(TEXT("Judge: Data/defs를 읽지 못했다: %s"), *Error));
		return false;
	}
	if (Defs.Units.Num() == 0)
	{
		AddError(TEXT("Judge: Data/defs에 병과가 없다"));
		return false;
	}
	if (Rank::FindDef(Defs.Units.GetData(), Defs.Units.Num(), UnknownKey) != nullptr)
	{
		AddError(FString::Printf(TEXT("Judge: 모르는 키로 쓰려던 '%c'가 병과 수치에 있다. 다른 키를 골라야 한다"), static_cast<TCHAR>(UnknownKey)));
		return false;
	}
	const uint8 KnownKey = Defs.Units[0].Key;

	int32 InvalidPassed = 0;
	int32 InvalidTotal = 0;
	int32 JudgedPassed = 0;
	int32 JudgedTotal = 0;

	// bExpectJudged가 false면 입력 오류: 판정하지 않고(bJudged false) 종류는 None이다(§5.2).
	// true면 판정한다(bJudged true). 판정했으면 종류가 있다.
	auto Check = [&](const FString& Case, const Rank::FBattleResult& Result, bool bExpectJudged)
	{
		const Rank::FVerdict Verdict = Rank::Judge(Result);
		FString Why;
		if (bExpectJudged)
		{
			++JudgedTotal;
			if (!Verdict.bJudged)
			{
				Why = TEXT("판정하지 않았다(bJudged false)");
			}
			else if (Verdict.Kind == Rank::EVerdictKind::None)
			{
				Why = TEXT("판정했는데 종류가 None이다");
			}
			else
			{
				++JudgedPassed;
			}
		}
		else
		{
			++InvalidTotal;
			if (Verdict.bJudged)
			{
				Why = TEXT("입력 오류인데 판정했다(bJudged true)");
			}
			else if (Verdict.Kind != Rank::EVerdictKind::None)
			{
				Why = FString::Printf(TEXT("입력 오류인데 종류가 None이 아니다(%d)"), static_cast<int32>(Verdict.Kind));
			}
			else
			{
				++InvalidPassed;
			}
		}
		if (!Why.IsEmpty())
		{
			AddError(FString::Printf(TEXT("Judge %s: %s. 코어 결과 %s, 성문 %d, 결계 %d"),
				*Case, *Why, RankTests::OutcomeName(Result.Outcome), Result.Gate[0], Result.Gate[1]));
		}
	};

	// 1. 입력 오류. 코어 결과의 기본값과, Simulate가 §3.6으로 돌려준 입력 오류 세 가지.
	Check(TEXT("기본값 결과(Invalid)"), Rank::FBattleResult(), false);

	struct FInvalidCase
	{
		const TCHAR* Name;
		uint8 AllyKey;   // 진영 0의 행 0 열 0. 0이면 진영 0이 빈다
		uint8 EnemyKey;  // 진영 1의 행 0 열 0. 0이면 진영 1이 빈다
	};
	const FInvalidCase InvalidCases[] = {
		{ TEXT("아군 진형판이 빔"), 0, KnownKey },
		{ TEXT("해골 진형판이 빔"), KnownKey, 0 },
		{ TEXT("병과 수치에 없는 키"), KnownKey, UnknownKey },
	};
	for (const FInvalidCase& Case : InvalidCases)
	{
		Rank::FBattleInput Input;
		Input.Rules = Defs.Rules;
		Input.Defs = Defs.Units.GetData();
		Input.NumDefs = Defs.Units.Num();
		Input.Side[0].Cells[0][0] = Case.AllyKey;
		Input.Side[1].Cells[0][0] = Case.EnemyKey;
		const Rank::FBattleResult Result = Rank::Simulate(Input, false);
		if (Result.Outcome != Rank::EOutcome::Invalid)
		{
			++InvalidTotal;
			AddError(FString::Printf(TEXT("Judge %s: §3.6 입력 오류가 나와야 하는데 코어 결과가 %s다"),
				Case.Name, RankTests::OutcomeName(Result.Outcome)));
			continue;
		}
		Check(Case.Name, Result, false);
	}

	// 2. 판정하는 코어 결과. 성문과 결계 값은 남음, 0, 음수(§5.1 결과 값은 음수일 수 있다)를 섞어 §5.2 표의 갈래를 모두 지난다.
	const Rank::EOutcome JudgedOutcomes[] = { Rank::EOutcome::A, Rank::EOutcome::B, Rank::EOutcome::Draw, Rank::EOutcome::Timeout };
	const int32 Gates[][Rank::NumSides] = { { 5, 5 }, { 5, 0 }, { 0, 5 }, { 0, 0 }, { -3, -2 } };
	for (const Rank::EOutcome Outcome : JudgedOutcomes)
	{
		for (const int32 (&Gate)[Rank::NumSides] : Gates)
		{
			Rank::FBattleResult Result;
			Result.Outcome = Outcome;
			Result.Gate[0] = Gate[0];
			Result.Gate[1] = Gate[1];
			Check(RankTests::OutcomeName(Outcome), Result, true);
		}
	}

	AddInfo(FString::Printf(TEXT("Judge 입력 오류 판정 없음 %d/%d (종류 None), A·B·Draw·Timeout 판정함 %d/%d"),
		InvalidPassed, InvalidTotal, JudgedPassed, JudgedTotal));
	const bool bPassed = InvalidTotal > 0 && JudgedTotal > 0 && InvalidPassed == InvalidTotal && JudgedPassed == JudgedTotal;
	if (!bPassed)
	{
		AddError(FString::Printf(TEXT("Judge 입력 오류 판정 없음 %d/%d, 판정함 %d/%d"), InvalidPassed, InvalidTotal, JudgedPassed, JudgedTotal));
	}
	return bPassed;
}

#endif // WITH_DEV_AUTOMATION_TESTS

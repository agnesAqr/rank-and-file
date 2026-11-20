#include "Misc/AutomationTest.h"
#include "RankBattle.h"
#include "RankFormationMask.h"
#include "RankPlacement.h"
#include "RankTestUtils.h"

// T2 결정성, T3 미러, T4 배치 판정 등가성(Source/RankTests/AGENTS.md 테스트 목록).
// 기대값의 출처는 성질(같은 입력이면 같은 결과, 대칭이면 무승부)과 칸 단위 판정이다. 화면 없이 돈다.
// 무작위 입력은 입력마다 시드 하나(기준 시드 + 번호)로 만든다. 실패 메시지의 시드 하나로 그 입력만 다시 만들 수 있다.

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags RankTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	// 실패를 모두 세되, 메시지는 앞의 몇 건만 자세히 적는다. 센 수는 줄이지 않는다.
	constexpr int32 MaxDetailedErrors = 20;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRankT2DeterminismTest, "RankAndFile.Core.T2_Determinism", RankTestFlags)

bool FRankT2DeterminismTest::RunTest(const FString& Parameters)
{
	RankTests::FDefs Defs;
	FString Error;
	if (!RankTests::LoadProjectDefs(Defs, Error))
	{
		AddError(FString::Printf(TEXT("T2: Data/defs를 읽지 못했다: %s"), *Error));
		return false;
	}

	constexpr int32 NumBattles = 1000;
	constexpr uint64 BaseSeed = 200000;
	int32 Passed = 0;
	int32 Failed = 0;
	auto Fail = [&](uint64 Seed, int32 Index, const FString& Why)
	{
		++Failed;
		if (Failed <= MaxDetailedErrors)
		{
			AddError(FString::Printf(TEXT("T2 seed=%llu (#%d): %s"), Seed, Index, *Why));
		}
	};

	for (int32 Index = 0; Index < NumBattles; ++Index)
	{
		const uint64 Seed = BaseSeed + static_cast<uint64>(Index);

		// 같은 시드로 입력을 두 번 따로 만든다. 무작위 배치도 결정적이어야 한다.
		Rank::FBattleInput Inputs[2];
		bool bMade = true;
		for (int32 Run = 0; Run < 2; ++Run)
		{
			Rank::FRankRng Rng(Seed);
			Inputs[Run].Rules = Defs.Rules;
			Inputs[Run].Defs = Defs.Units.GetData();
			Inputs[Run].NumDefs = Defs.Units.Num();
			bMade = RankTests::MakeRandomFormationFromRng(Rng, Defs.Units, Inputs[Run].Side[0])
				&& RankTests::MakeRandomFormationFromRng(Rng, Defs.Units, Inputs[Run].Side[1])
				&& bMade;
		}
		if (!bMade)
		{
			Fail(Seed, Index, TEXT("무작위 배치를 100번 안에 만들지 못했다"));
			continue;
		}
		if (!RankTests::SameFormation(Inputs[0].Side[0], Inputs[1].Side[0]) || !RankTests::SameFormation(Inputs[0].Side[1], Inputs[1].Side[1]))
		{
			Fail(Seed, Index, FString::Printf(TEXT("같은 시드인데 배치가 다르다: %s vs %s / %s vs %s"),
				*RankTests::FormationToString(Inputs[0].Side[0]), *RankTests::FormationToString(Inputs[1].Side[0]),
				*RankTests::FormationToString(Inputs[0].Side[1]), *RankTests::FormationToString(Inputs[1].Side[1])));
			continue;
		}

		// 첫 번째는 로그를 켜고, 두 번째는 끈다. 로그는 결과와 해시를 바꾸지 않아야 한다(Source/RankCore/AGENTS.md "공개 형태").
		const Rank::FBattleResult First = Rank::Simulate(Inputs[0], true);
		const Rank::FBattleResult Second = Rank::Simulate(Inputs[1], false);
		const FString Battle = FString::Printf(TEXT("진영 0 %s, 진영 1 %s"),
			*RankTests::FormationToString(Inputs[0].Side[0]), *RankTests::FormationToString(Inputs[0].Side[1]));

		if (First.Outcome == Rank::EOutcome::Invalid)
		{
			Fail(Seed, Index, FString::Printf(TEXT("입력 오류(Invalid)가 나왔다. %s"), *Battle));
			continue;
		}
		if (First.Log.Num() != First.NumEvents || Second.Log.Num() != 0)
		{
			Fail(Seed, Index, FString::Printf(TEXT("로그 줄 수가 다르다: 켬 %d(이벤트 수 %d), 끔 %d. %s"),
				First.Log.Num(), First.NumEvents, Second.Log.Num(), *Battle));
			continue;
		}
		FString Diff;
		if (!RankTests::SameResult(First, Second, Diff))
		{
			Fail(Seed, Index, FString::Printf(TEXT("두 번 돌린 결과가 다르다(%s). %s"), *Diff, *Battle));
			continue;
		}
		++Passed;
	}

	AddInfo(FString::Printf(TEXT("T2 결정성 %d/%d (시드 %llu~%llu, 같은 시드로 입력을 두 번 만들고 로그를 켜고 끄고 비교)"),
		Passed, NumBattles, BaseSeed, BaseSeed + NumBattles - 1));
	if (Passed != NumBattles)
	{
		AddError(FString::Printf(TEXT("T2 결정성 %d/%d. 실패 %d건"), Passed, NumBattles, Failed));
	}
	return Passed == NumBattles;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRankT3MirrorTest, "RankAndFile.Core.T3_Mirror", RankTestFlags)

bool FRankT3MirrorTest::RunTest(const FString& Parameters)
{
	RankTests::FDefs Defs;
	FString Error;
	if (!RankTests::LoadProjectDefs(Defs, Error))
	{
		AddError(FString::Printf(TEXT("T3: Data/defs를 읽지 못했다: %s"), *Error));
		return false;
	}

	constexpr int32 NumBattles = 1000;
	constexpr uint64 BaseSeed = 300000;
	int32 Passed = 0;
	int32 Failed = 0;
	int32 Draws = 0;
	int32 Timeouts = 0;

	for (int32 Index = 0; Index < NumBattles; ++Index)
	{
		const uint64 Seed = BaseSeed + static_cast<uint64>(Index);
		Rank::FRankRng Rng(Seed);
		Rank::FBattleInput Input;
		Input.Rules = Defs.Rules;
		Input.Defs = Defs.Units.GetData();
		Input.NumDefs = Defs.Units.Num();

		FString Why;
		if (!RankTests::MakeRandomFormationFromRng(Rng, Defs.Units, Input.Side[0]))
		{
			Why = TEXT("무작위 배치를 100번 안에 만들지 못했다");
		}
		else
		{
			// §3.3, §9 15번: 같은 배치를 양쪽에 놓는다. 진영 1도 행 0이 최전열이다(§9 6번).
			Input.Side[1] = Input.Side[0];
			const Rank::FBattleResult Result = Rank::Simulate(Input, false);
			if (Result.Outcome == Rank::EOutcome::Draw)
			{
				++Draws;
				++Passed;
				continue;
			}
			if (Result.Outcome == Rank::EOutcome::Timeout)
			{
				++Timeouts;
				++Passed;
				continue;
			}
			Why = FString::Printf(TEXT("무승부가 아니다: %s, 틱 %d, 성문 %d, 결계 %d. 배치 %s"),
				RankTests::OutcomeName(Result.Outcome), Result.Tick, Result.Gate[0], Result.Gate[1],
				*RankTests::FormationToString(Input.Side[0]));
		}
		++Failed;
		if (Failed <= MaxDetailedErrors)
		{
			AddError(FString::Printf(TEXT("T3 seed=%llu (#%d): %s"), Seed, Index, *Why));
		}
	}

	AddInfo(FString::Printf(TEXT("T3 미러 %d/%d (Draw %d, Timeout %d, 시드 %llu~%llu)"),
		Passed, NumBattles, Draws, Timeouts, BaseSeed, BaseSeed + NumBattles - 1));
	if (Passed != NumBattles)
	{
		AddError(FString::Printf(TEXT("T3 미러 %d/%d. 실패 %d건"), Passed, NumBattles, Failed));
	}
	return Passed == NumBattles;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRankT4PlacementEquivalenceTest, "RankAndFile.Core.T4_PlacementEquivalence", RankTestFlags)

bool FRankT4PlacementEquivalenceTest::RunTest(const FString& Parameters)
{
	constexpr int32 NumCases = 10000;
	constexpr uint64 BaseSeed = 400000;
	int32 Passed = 0;
	int32 Failed = 0;
	int32 Placeable = 0;

	Rank::FShape SingleCell;
	SingleCell.NumCells = 1;

	// 진형판 밖도 섞으려고 위치를 양쪽으로 2칸씩 넓혀 뽑는다.
	constexpr int32 Margin = 2;

	for (int32 Index = 0; Index < NumCases; ++Index)
	{
		const uint64 Seed = BaseSeed + static_cast<uint64>(Index);
		Rank::FRankRng Rng(Seed);

		// 1. 점유: 칸마다 3분의 1 확률로 차 있다. 두 판정에 같은 점유를 공개 함수로만 넣는다.
		bool Occupied[Rank::BoardRows][Rank::BoardCols] = {};
		Rank::FFormationMask Mask;
		for (int32 Row = 0; Row < Rank::BoardRows; ++Row)
		{
			for (int32 Col = 0; Col < Rank::BoardCols; ++Col)
			{
				if (Rng.NextBelow(3u) == 0)
				{
					Occupied[Row][Col] = true;
					Mask.Place(SingleCell, Row, Col);
				}
			}
		}

		// 2. 모양, 회전, 위치
		Rank::FShape Shape = RankTests::MakeRandomShape(Rng);
		const uint32 Turns = Rng.NextBelow(4u);
		for (uint32 Turn = 0; Turn < Turns; ++Turn)
		{
			Shape = Rank::RotateShape90(Shape);
		}
		const int32 Row = static_cast<int32>(Rng.NextBelow(static_cast<uint32>(Rank::BoardRows + 2 * Margin))) - Margin;
		const int32 Col = static_cast<int32>(Rng.NextBelow(static_cast<uint32>(Rank::BoardCols + 2 * Margin))) - Margin;

		// 3. 두 판정 비교
		const bool bMask = Mask.CanPlace(Shape, Row, Col);
		const bool bCellwise = Rank::CanPlaceCellwise(Occupied, Shape, Row, Col);
		FString Why;
		if (bMask != bCellwise)
		{
			Why = FString::Printf(TEXT("판정이 다르다: 비트마스크 %d, 칸 단위 %d"), bMask ? 1 : 0, bCellwise ? 1 : 0);
		}
		else if (bMask)
		{
			// 4. 놓을 수 있으면 놓은 뒤의 점유도 비교한다. 비트마스크의 점유는 한 칸 모양의 CanPlace로만 읽는다.
			++Placeable;
			Mask.Place(Shape, Row, Col);
			for (int32 Cell = 0; Cell < Shape.NumCells; ++Cell)
			{
				Occupied[Row + Shape.Cells[Cell].Row][Col + Shape.Cells[Cell].Col] = true;
			}
			for (int32 CheckRow = 0; CheckRow < Rank::BoardRows && Why.IsEmpty(); ++CheckRow)
			{
				for (int32 CheckCol = 0; CheckCol < Rank::BoardCols && Why.IsEmpty(); ++CheckCol)
				{
					if (Mask.CanPlace(SingleCell, CheckRow, CheckCol) == Occupied[CheckRow][CheckCol])
					{
						Why = FString::Printf(TEXT("놓은 뒤 점유가 다르다: 행 %d 열 %d"), CheckRow, CheckCol);
					}
				}
			}
		}

		if (Why.IsEmpty())
		{
			++Passed;
			continue;
		}
		++Failed;
		if (Failed <= MaxDetailedErrors)
		{
			FString ShapeText;
			for (int32 Cell = 0; Cell < Shape.NumCells; ++Cell)
			{
				ShapeText += FString::Printf(TEXT("%s%d:%d"), Cell > 0 ? TEXT(";") : TEXT(""), Shape.Cells[Cell].Row, Shape.Cells[Cell].Col);
			}
			AddError(FString::Printf(TEXT("T4 seed=%llu (#%d): %s. 모양 %s, 위치 행 %d 열 %d"), Seed, Index, *Why, *ShapeText, Row, Col));
		}
	}

	AddInfo(FString::Printf(TEXT("T4 배치 판정 등가성 %d/%d (놓을 수 있음 %d, 없음 %d, 시드 %llu~%llu)"),
		Passed, NumCases, Placeable, NumCases - Placeable, BaseSeed, BaseSeed + NumCases - 1));
	if (Passed != NumCases)
	{
		AddError(FString::Printf(TEXT("T4 배치 판정 등가성 %d/%d. 실패 %d건"), Passed, NumCases, Failed));
	}
	return Passed == NumCases;
}

#endif // WITH_DEV_AUTOMATION_TESTS

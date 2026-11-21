#include "RankTestUtils.h"

#include "Misc/Paths.h"
#include "RankContent.h"
#include "RankRegressionData.h"

namespace RankTests
{
	namespace
	{
		constexpr int32 MaxSquadsPerDeck = 6;
		constexpr int32 MaxDeckCells = 16;
	}

	FString ProjectFile(const TCHAR* RelativePath)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}

	bool LoadProjectDefs(FDefs& OutDefs, FString& OutError)
	{
		return RankContent::LoadDefsFiles(
			ProjectFile(RankRegression::UnitsCsvFile), ProjectFile(RankRegression::RulesCsvFile),
			OutDefs.UnitRows, OutDefs.RulesRow, OutDefs.Units, OutDefs.Rules, OutError);
	}

	Rank::FShape MakeRandomShape(Rank::FRankRng& Rng)
	{
		static const int32 Steps[4][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };

		Rank::FShape Shape;
		const int32 Target = 1 + static_cast<int32>(Rng.NextBelow(static_cast<uint32>(Rank::MaxShapeCells)));
		Shape.Cells[0].Row = 0;
		Shape.Cells[0].Col = 0;
		Shape.NumCells = 1;
		// 이미 있는 칸 하나와 방향 하나를 뽑아 옆 칸을 더한다. 겹치면 다시 뽑는다. 그래서 언제나 이어진 모양이다.
		while (Shape.NumCells < Target)
		{
			const Rank::FShapeCell& From = Shape.Cells[Rng.NextBelow(static_cast<uint32>(Shape.NumCells))];
			const uint32 Dir = Rng.NextBelow(4u);
			const int32 Row = From.Row + Steps[Dir][0];
			const int32 Col = From.Col + Steps[Dir][1];
			bool bTaken = false;
			for (int32 Index = 0; Index < Shape.NumCells; ++Index)
			{
				bTaken = bTaken || (Shape.Cells[Index].Row == Row && Shape.Cells[Index].Col == Col);
			}
			if (!bTaken)
			{
				Shape.Cells[Shape.NumCells].Row = static_cast<int8>(Row);
				Shape.Cells[Shape.NumCells].Col = static_cast<int8>(Col);
				++Shape.NumCells;
			}
		}
		return Rank::NormalizeShape(Shape);
	}

	void MakeRandomDeck(Rank::FRankRng& Rng, const TArray<Rank::FUnitDef>& Defs, TArray<Rank::FSquad>& OutDeck)
	{
		OutDeck.Reset();
		const int32 NumSquads = 1 + static_cast<int32>(Rng.NextBelow(static_cast<uint32>(MaxSquadsPerDeck)));
		int32 TotalCells = 0;
		for (int32 Index = 0; Index < NumSquads; ++Index)
		{
			Rank::FSquad Squad;
			Squad.Shape = MakeRandomShape(Rng);
			Squad.Key = Defs[Rng.NextBelow(static_cast<uint32>(Defs.Num()))].Key;
			if (TotalCells + Squad.Shape.NumCells > MaxDeckCells)
			{
				continue; // 칸 수 합 상한을 넘는 분대는 덱에 넣지 않는다. 난수는 이미 뽑았으므로 수열은 시드로 정해진다
			}
			TotalCells += Squad.Shape.NumCells;
			OutDeck.Add(Squad);
		}
	}

	bool MakeRandomFormationFromRng(Rank::FRankRng& Rng, const TArray<Rank::FUnitDef>& Defs, Rank::FFormation& OutFormation)
	{
		TArray<Rank::FSquad> Deck;
		MakeRandomDeck(Rng, Defs, Deck);
		return Rank::MakeRandomFormation(Deck.GetData(), Deck.Num(), Rng, OutFormation);
	}

	FString FormationToString(const Rank::FFormation& Formation)
	{
		FString Text;
		for (int32 Row = 0; Row < Rank::BoardRows; ++Row)
		{
			if (Row > 0)
			{
				Text.AppendChar(TEXT('/'));
			}
			for (int32 Col = 0; Col < Rank::BoardCols; ++Col)
			{
				const uint8 Key = Formation.Cells[Row][Col];
				Text.AppendChar(Key == 0 ? TEXT('.') : static_cast<TCHAR>(Key));
			}
		}
		return Text;
	}

	bool SameFormation(const Rank::FFormation& A, const Rank::FFormation& B)
	{
		for (int32 Row = 0; Row < Rank::BoardRows; ++Row)
		{
			for (int32 Col = 0; Col < Rank::BoardCols; ++Col)
			{
				if (A.Cells[Row][Col] != B.Cells[Row][Col])
				{
					return false;
				}
			}
		}
		return true;
	}

	const TCHAR* OutcomeName(Rank::EOutcome Outcome)
	{
		return RankRegression::OutcomeName(Outcome); // 이름은 RankGame 한 곳에 둔다
	}

	bool SameResult(const Rank::FBattleResult& A, const Rank::FBattleResult& B, FString& OutDiff)
	{
		auto Diff = [&OutDiff](const TCHAR* Name, int64 ValueA, int64 ValueB) -> bool
		{
			if (ValueA == ValueB)
			{
				return false;
			}
			OutDiff = FString::Printf(TEXT("%s: %lld / %lld"), Name, ValueA, ValueB);
			return true;
		};

		if (Diff(TEXT("결과"), static_cast<int64>(A.Outcome), static_cast<int64>(B.Outcome))
			|| Diff(TEXT("틱"), A.Tick, B.Tick)
			|| Diff(TEXT("해시"), A.Hash, B.Hash)
			|| Diff(TEXT("이벤트 수"), A.NumEvents, B.NumEvents)
			|| Diff(TEXT("유닛 수"), A.NumUnits, B.NumUnits))
		{
			return false;
		}
		for (int32 Side = 0; Side < Rank::NumSides; ++Side)
		{
			if (Diff(Side == 0 ? TEXT("성문") : TEXT("결계"), A.Gate[Side], B.Gate[Side])
				|| Diff(Side == 0 ? TEXT("생존 수(진영 0)") : TEXT("생존 수(진영 1)"), A.Alive[Side], B.Alive[Side])
				|| Diff(Side == 0 ? TEXT("체력 합(진영 0)") : TEXT("체력 합(진영 1)"), A.HpSum[Side], B.HpSum[Side]))
			{
				return false;
			}
		}
		for (int32 Index = 0; Index < A.NumUnits; ++Index)
		{
			const Rank::FUnitResult& UnitA = A.Units[Index];
			const Rank::FUnitResult& UnitB = B.Units[Index];
			if (UnitA.Id != UnitB.Id || UnitA.Hp != UnitB.Hp || UnitA.bAlive != UnitB.bAlive)
			{
				OutDiff = FString::Printf(TEXT("유닛 %d번째: 번호 %d/%d, 체력 %d/%d, 생존 %d/%d"),
					Index, UnitA.Id, UnitB.Id, UnitA.Hp, UnitB.Hp, UnitA.bAlive ? 1 : 0, UnitB.bAlive ? 1 : 0);
				return false;
			}
		}
		return true;
	}
}

#include "RankPlacement.h"

namespace Rank
{
	namespace
	{
		// 입력이 놓을 수 있는 모양인지만 본다. 칸이 이어져 있는지는 콘텐츠 관문 1(tools/validate_content.js)이 본다.
		bool IsValidSquadList(const FSquad* Squads, int32 NumSquads)
		{
			if (NumSquads < 0 || NumSquads > MaxSquadsPerFormation || (NumSquads > 0 && Squads == nullptr))
			{
				return false;
			}
			int32 TotalCells = 0;
			for (int32 Index = 0; Index < NumSquads; ++Index)
			{
				const FSquad& Squad = Squads[Index];
				if (Squad.Key == 0 || Squad.Shape.NumCells < 1 || Squad.Shape.NumCells > MaxShapeCells)
				{
					return false;
				}
				TotalCells += Squad.Shape.NumCells;
			}
			return TotalCells <= BoardRows * BoardCols;
		}
	}

	bool MakeRandomFormation(const FSquad* Squads, int32 NumSquads, FRankRng& Rng, FFormation& OutFormation)
	{
		OutFormation = FFormation();
		if (!IsValidSquadList(Squads, NumSquads))
		{
			return false;
		}

		// §2.1 분대마다 네 방향을 미리 만든다. 0번은 처음 모양을 왼쪽 위로 맞춘 것이고, 1~3번은 90도씩 더 돌린 것이다.
		FShape Rotations[MaxSquadsPerFormation][4];
		for (int32 Index = 0; Index < NumSquads; ++Index)
		{
			Rotations[Index][0] = NormalizeShape(Squads[Index].Shape);
			for (int32 Turn = 1; Turn < 4; ++Turn)
			{
				Rotations[Index][Turn] = RotateShape90(Rotations[Index][Turn - 1]);
			}
		}

		int32 Order[MaxSquadsPerFormation] = {};
		for (int32 Attempt = 0; Attempt < PlacementMaxAttempts; ++Attempt)
		{
			// 1. 분대 순서를 섞는다(피셔-예이츠). 다시 시작할 때마다 처음 순서에서 새로 섞는다.
			for (int32 Index = 0; Index < NumSquads; ++Index)
			{
				Order[Index] = Index;
			}
			for (int32 Index = NumSquads - 1; Index > 0; --Index)
			{
				const int32 Pick = static_cast<int32>(Rng.NextBelow(static_cast<uint32>(Index + 1)));
				const int32 Swap = Order[Index];
				Order[Index] = Order[Pick];
				Order[Pick] = Swap;
			}

			// 2. 분대마다 회전과 위치를 40번까지 뽑아 놓아 본다.
			FFormationMask Mask;
			FFormation Working;
			bool bAllPlaced = true;
			for (int32 Step = 0; Step < NumSquads && bAllPlaced; ++Step)
			{
				const FSquad& Squad = Squads[Order[Step]];
				bool bPlaced = false;
				for (int32 Try = 0; Try < PlacementTriesPerSquad && !bPlaced; ++Try)
				{
					// 난수를 뽑는 순서: 회전, 행, 열
					const uint32 Turn = Rng.NextBelow(4u);
					const int32 Row = static_cast<int32>(Rng.NextBelow(static_cast<uint32>(BoardRows)));
					const int32 Col = static_cast<int32>(Rng.NextBelow(static_cast<uint32>(BoardCols)));
					const FShape& Shape = Rotations[Order[Step]][Turn];
					if (Mask.CanPlace(Shape, Row, Col))
					{
						Mask.Place(Shape, Row, Col);
						for (int32 Cell = 0; Cell < Shape.NumCells; ++Cell)
						{
							Working.Cells[Row + Shape.Cells[Cell].Row][Col + Shape.Cells[Cell].Col] = Squad.Key;
						}
						bPlaced = true;
					}
				}
				bAllPlaced = bPlaced;
			}

			// 3. 다 놓았으면 끝. 하나라도 못 놓았으면 이 배치를 버리고 처음부터 다시 한다.
			if (bAllPlaced)
			{
				OutFormation = Working;
				return true;
			}
		}
		return false;
	}
}

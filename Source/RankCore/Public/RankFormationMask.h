#pragma once

#include "RankTypes.h"

namespace Rank
{
	// §2.1 분대 모양: 1~4칸이 변끼리 붙은 폴리오미노. 칸은 (행, 열) 오프셋이다.
	inline constexpr int32 MaxShapeCells = 4;

	struct FShapeCell
	{
		int8 Row = 0;
		int8 Col = 0;
	};

	struct FShape
	{
		FShapeCell Cells[MaxShapeCells];
		int32 NumCells = 0;
	};

	// 가장 작은 행과 열이 0이 되도록 옮긴다. 놓는 위치(Row, Col)는 이렇게 맞춘 모양의 왼쪽 위다.
	RANKCORE_API FShape NormalizeShape(const FShape& Shape);

	// §2.1 90도 돌린 모양. 네 번 돌리면 처음 모양이 된다. 뒤집기는 없다(§9 10번).
	RANKCORE_API FShape RotateShape90(const FShape& Shape);

	// 배치 판정. 행마다 비트 6개로 점유를 표시하고, 한 행을 AND 한 번으로 검사한다(docs/design.md 6.7).
	struct RANKCORE_API FFormationMask
	{
		uint8 RowBits[BoardRows] = {};

		// 진형판 안이고 다른 분대와 겹치지 않으면 true. 칸 수가 1~4가 아니면 false
		bool CanPlace(const FShape& Shape, int32 Row, int32 Col) const;

		// CanPlace가 true일 때만 부른다. 놓은 칸을 점유로 표시한다.
		void Place(const FShape& Shape, int32 Row, int32 Col);
	};

	// 칸 단위로 단순하게 짠 같은 판정. 배치 판정 등가성 테스트(T4)의 대조군이다. 지우지 않는다. 칸 수가 1~4가 아니면 false.
	RANKCORE_API bool CanPlaceCellwise(const bool (&Occupied)[BoardRows][BoardCols], const FShape& Shape, int32 Row, int32 Col);
}

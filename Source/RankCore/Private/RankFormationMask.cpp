#include "RankFormationMask.h"

namespace Rank
{
	FShape NormalizeShape(const FShape& Shape)
	{
		FShape Out = Shape;
		if (Shape.NumCells <= 0)
		{
			return Out;
		}
		int32 MinRow = Shape.Cells[0].Row;
		int32 MinCol = Shape.Cells[0].Col;
		for (int32 Index = 1; Index < Shape.NumCells; ++Index)
		{
			MinRow = Shape.Cells[Index].Row < MinRow ? Shape.Cells[Index].Row : MinRow;
			MinCol = Shape.Cells[Index].Col < MinCol ? Shape.Cells[Index].Col : MinCol;
		}
		for (int32 Index = 0; Index < Shape.NumCells; ++Index)
		{
			Out.Cells[Index].Row = static_cast<int8>(Shape.Cells[Index].Row - MinRow);
			Out.Cells[Index].Col = static_cast<int8>(Shape.Cells[Index].Col - MinCol);
		}
		return Out;
	}

	FShape RotateShape90(const FShape& Shape)
	{
		// (행, 열) → (열, −행)으로 돌린 뒤 왼쪽 위로 옮긴다.
		FShape Rotated = Shape;
		for (int32 Index = 0; Index < Shape.NumCells; ++Index)
		{
			Rotated.Cells[Index].Row = Shape.Cells[Index].Col;
			Rotated.Cells[Index].Col = static_cast<int8>(-Shape.Cells[Index].Row);
		}
		return NormalizeShape(Rotated);
	}

	bool FFormationMask::CanPlace(const FShape& Shape, int32 Row, int32 Col) const
	{
		uint8 ShapeBits[BoardRows] = {};
		for (int32 Index = 0; Index < Shape.NumCells; ++Index)
		{
			const int32 CellRow = Row + Shape.Cells[Index].Row;
			const int32 CellCol = Col + Shape.Cells[Index].Col;
			if (CellRow < 0 || CellRow >= BoardRows || CellCol < 0 || CellCol >= BoardCols)
			{
				return false; // 진형판 밖
			}
			ShapeBits[CellRow] = static_cast<uint8>(ShapeBits[CellRow] | (1u << CellCol));
		}
		for (int32 BoardRow = 0; BoardRow < BoardRows; ++BoardRow)
		{
			if ((RowBits[BoardRow] & ShapeBits[BoardRow]) != 0)
			{
				return false; // 다른 분대와 겹친다
			}
		}
		return true;
	}

	void FFormationMask::Place(const FShape& Shape, int32 Row, int32 Col)
	{
		for (int32 Index = 0; Index < Shape.NumCells; ++Index)
		{
			const int32 CellRow = Row + Shape.Cells[Index].Row;
			const int32 CellCol = Col + Shape.Cells[Index].Col;
			RowBits[CellRow] = static_cast<uint8>(RowBits[CellRow] | (1u << CellCol));
		}
	}

	bool CanPlaceCellwise(const bool (&Occupied)[BoardRows][BoardCols], const FShape& Shape, int32 Row, int32 Col)
	{
		for (int32 Index = 0; Index < Shape.NumCells; ++Index)
		{
			const int32 CellRow = Row + Shape.Cells[Index].Row;
			const int32 CellCol = Col + Shape.Cells[Index].Col;
			if (CellRow < 0 || CellRow >= BoardRows || CellCol < 0 || CellCol >= BoardCols)
			{
				return false;
			}
			if (Occupied[CellRow][CellCol])
			{
				return false;
			}
		}
		return true;
	}
}

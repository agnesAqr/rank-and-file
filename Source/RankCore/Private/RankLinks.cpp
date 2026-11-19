#include "RankBattle.h"

namespace Rank
{
	namespace
	{
		uint8 CellAt(const FFormation& Formation, int32 Row, int32 Col)
		{
			if (Row < 0 || Row >= BoardRows || Col < 0 || Col >= BoardCols)
			{
				return 0;
			}
			return Formation.Cells[Row][Col];
		}

		// §3.4 a: 그 종류를 주는 병과의 한 겹 크기. 지금 데이터는 종류마다 주는 병과가 하나뿐이다.
		// 둘 이상인 데이터는 Simulate가 입력 오류로 막는다(RankSimulate.cpp ValidateInput).
		int32 GiverAmount(const FUnitDef* Defs, int32 NumDefs, ELinkStat Stat)
		{
			for (int32 Index = 0; Index < NumDefs; ++Index)
			{
				if (Defs[Index].LinkStat == Stat)
				{
					return Defs[Index].LinkAmount;
				}
			}
			return 0;
		}

		int32 Min32(int32 A, int32 B) { return A < B ? A : B; }
		int32 Max32(int32 A, int32 B) { return A > B ? A : B; }
	}

	const FUnitDef* FindDef(const FUnitDef* Defs, int32 NumDefs, uint8 Key)
	{
		if (Defs == nullptr || Key == 0)
		{
			return nullptr;
		}
		for (int32 Index = 0; Index < NumDefs; ++Index)
		{
			if (Defs[Index].Key == Key)
			{
				return &Defs[Index];
			}
		}
		return nullptr;
	}

	FLinkStacks ComputeLinks(const FFormation& Formation, int32 Row, int32 Col, const FUnitDef* Defs, int32 NumDefs)
	{
		FLinkStacks Stacks;
		const uint8 Self = CellAt(Formation, Row, Col);
		if (Self == 0)
		{
			return Stacks;
		}

		// §2.2, §3.3 위·아래·왼쪽·오른쪽 이웃만 본다. 대각선은 맞닿은 것이 아니다.
		// 링크는 병과만 보고 정한다. 진영은 보지 않는다(§9 15번).
		const int32 Offsets[4][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const uint8 Neighbor = CellAt(Formation, Row + Offsets[Index][0], Col + Offsets[Index][1]);
			if (Neighbor == 0 || Neighbor == Self)
			{
				continue; // 빈칸이거나 같은 병과면 링크가 없다
			}
			const FUnitDef* NeighborDef = FindDef(Defs, NumDefs, Neighbor);
			if (NeighborDef == nullptr || NeighborDef->LinkStat == ELinkStat::None)
			{
				continue; // §2.8 해골처럼 링크 효과가 없는 병과는 주지 않는다
			}
			++Stacks.Count[static_cast<int32>(NeighborDef->LinkStat)];
		}
		return Stacks;
	}

	bool ComputeFinalStats(const FFormation& Formation, int32 Row, int32 Col, const FUnitDef* Defs, int32 NumDefs, const FRuleParams& Rules, FUnitStats& OutStats)
	{
		const FUnitDef* Def = FindDef(Defs, NumDefs, CellAt(Formation, Row, Col));
		if (Def == nullptr)
		{
			return false;
		}

		const FLinkStacks Stacks = ComputeLinks(Formation, Row, Col, Defs, NumDefs);
		auto Bonus = [&](ELinkStat Stat) -> int32
		{
			// §3.4 a × min(n, 겹 상한)
			const int32 Count = Min32(Stacks.Count[static_cast<int32>(Stat)], Rules.LinkStackMax);
			return Count == 0 ? 0 : GiverAmount(Defs, NumDefs, Stat) * Count;
		};

		OutStats.Armor = Def->Armor + Bonus(ELinkStat::Armor);
		OutStats.Atk = Def->Atk + Bonus(ELinkStat::Atk);
		OutStats.Range = Min32(Def->Range + Bonus(ELinkStat::Range), Rules.RangeMax);
		OutStats.MaxHp = Def->Hp + Bonus(ELinkStat::Hp);
		OutStats.Interval = Max32(Def->Interval + Bonus(ELinkStat::Interval), Rules.IntervalMin);
		return true;
	}
}

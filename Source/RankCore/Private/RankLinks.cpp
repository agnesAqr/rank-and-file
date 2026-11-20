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
		const int32 Offsets[MaxLinkNeighbors][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };
		for (int32 Index = 0; Index < MaxLinkNeighbors; ++Index)
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

			// §3.4 겹은 주는 병과마다 따로 센다. 이웃은 넷뿐이라 NumGivers는 MaxLinkNeighbors를 넘지 않는다.
			int32 GiverIndex = 0;
			for (; GiverIndex < Stacks.NumGivers; ++GiverIndex)
			{
				if (Stacks.Givers[GiverIndex].Key == Neighbor)
				{
					break;
				}
			}
			if (GiverIndex == Stacks.NumGivers)
			{
				FLinkGiver& NewGiver = Stacks.Givers[Stacks.NumGivers];
				NewGiver.Key = Neighbor;
				NewGiver.Stat = NeighborDef->LinkStat;
				NewGiver.Amount = NeighborDef->LinkAmount;
				++Stacks.NumGivers;
			}
			++Stacks.Givers[GiverIndex].Count;
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
			// §3.4 주는 병과마다 a × min(n, 겹 상한)을 구해 더한다. 사거리 상한과 간격 하한은 아래에서 더한 뒤에 적용한다.
			int32 Sum = 0;
			for (int32 Index = 0; Index < Stacks.NumGivers; ++Index)
			{
				const FLinkGiver& Giver = Stacks.Givers[Index];
				if (Giver.Stat == Stat)
				{
					Sum += Giver.Amount * Min32(Giver.Count, Rules.LinkStackMax);
				}
			}
			return Sum;
		};

		OutStats.Armor = Def->Armor + Bonus(ELinkStat::Armor);
		OutStats.Atk = Def->Atk + Bonus(ELinkStat::Atk);
		OutStats.Range = Min32(Def->Range + Bonus(ELinkStat::Range), Rules.RangeMax);
		OutStats.MaxHp = Def->Hp + Bonus(ELinkStat::Hp);
		OutStats.Interval = Max32(Def->Interval + Bonus(ELinkStat::Interval), Rules.IntervalMin);
		return true;
	}
}

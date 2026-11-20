#include "RankRegressionCommandlet.h"

#include "HAL/FileManager.h"
#include "Logging/StructuredLog.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RankBattle.h"
#include "RankContent.h"
#include "RankRegressionData.h"

DEFINE_LOG_CATEGORY_STATIC(LogRankRegression, Log, All);

// 갈래 확인은 전투 결과를 다시 계산하지 않는다. 쓰는 것은 아래뿐이다.
// - 입력 진형(칸, 열, 행)과 대열 순번(§2.3: 같은 열에서 앞에 선 병사 수)
// - 코어 함수의 결과: Rank::ComputeLinks, Rank::ComputeFinalStats, Rank::Simulate(로그와 결과), Rank::Judge
// - 유닛 번호에서 진영, 행, 열을 되읽기(§3.2)

namespace
{
	const TCHAR* const InputsFile = TEXT("Source/RankTests/Fixtures/regression_inputs.csv");
	const TCHAR* const BranchesFile = TEXT("Source/RankTests/Fixtures/regression_branches.csv");
	const TCHAR* const RecordFile = TEXT("Source/RankTests/Fixtures/regression_battles_v1.json");
	const TCHAR* const TallyFile = TEXT("Saved/RankRegression/branch_tally.csv");

	FString ProjectPath(const TCHAR* Relative)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), Relative));
	}

	struct FBranch
	{
		FString Id;
		FString Clause;
		FString Text;
		FString Static;
		FString Tests;
		FString Battles;
		FString Detect;
		bool bT1 = false;
	};

	struct FUnitFacts
	{
		int32 Id = 0;
		int32 Side = 0;
		int32 Row = 0;
		int32 Col = 0;
		uint8 Key = 0;
		const Rank::FUnitDef* Def = nullptr;
		Rank::FUnitStats Stats;
		Rank::FLinkStacks Links;
		int32 Rank0 = 0;      // 전투 시작 때 대열 순번
		int32 DeathTick = 0;  // 0이면 쓰러지지 않음
		int32 ReviveTick = 0; // 0이면 부활하지 않음
		int32 FirstAttackTick = 0;
	};

	bool IsAttack(const Rank::FBattleEvent& Event)
	{
		return Event.Type == static_cast<uint8>(Rank::EEventType::Damage) || Event.Type == static_cast<uint8>(Rank::EEventType::GateHit);
	}

	struct FBattleFacts
	{
		const Rank::FBattleInput* Input = nullptr;
		const Rank::FBattleResult* Result = nullptr;
		Rank::FVerdict Verdict;
		TArray<FUnitFacts> Units;
		int32 Grid[Rank::NumSides][Rank::BoardRows][Rank::BoardCols];
		int32 ColumnCount[Rank::NumSides][Rank::BoardCols] = {};

		void Build(const Rank::FBattleInput& InInput, const Rank::FBattleResult& InResult)
		{
			Input = &InInput;
			Result = &InResult;
			Verdict = Rank::Judge(InResult);
			for (int32 Side = 0; Side < Rank::NumSides; ++Side)
			{
				const Rank::FFormation& Formation = InInput.Side[Side];
				for (int32 Col = 0; Col < Rank::BoardCols; ++Col)
				{
					int32 InFront = 0;
					for (int32 Row = 0; Row < Rank::BoardRows; ++Row)
					{
						Grid[Side][Row][Col] = -1;
						const uint8 Key = Formation.Cells[Row][Col];
						if (Key == 0)
						{
							continue;
						}
						FUnitFacts Unit;
						Unit.Id = Side * Rank::UnitIdSideFactor + Row * Rank::UnitIdRowFactor + Col;
						Unit.Side = Side;
						Unit.Row = Row;
						Unit.Col = Col;
						Unit.Key = Key;
						Unit.Def = Rank::FindDef(InInput.Defs, InInput.NumDefs, Key);
						Rank::ComputeFinalStats(Formation, Row, Col, InInput.Defs, InInput.NumDefs, InInput.Rules, Unit.Stats);
						Unit.Links = Rank::ComputeLinks(Formation, Row, Col, InInput.Defs, InInput.NumDefs);
						Unit.Rank0 = InFront++;
						Grid[Side][Row][Col] = Units.Add(Unit);
						++ColumnCount[Side][Col];
					}
				}
			}
			for (const Rank::FBattleEvent& Event : InResult.Log)
			{
				if (FUnitFacts* Actor = ById(Event.Actor))
				{
					if (IsAttack(Event) && Actor->FirstAttackTick == 0)
					{
						Actor->FirstAttackTick = Event.Tick;
					}
					if (Event.Type == static_cast<uint8>(Rank::EEventType::Death))
					{
						Actor->DeathTick = Event.Tick;
					}
					if (Event.Type == static_cast<uint8>(Rank::EEventType::Revive))
					{
						Actor->ReviveTick = Event.Tick;
					}
				}
			}
		}

		FUnitFacts* ById(int32 Id)
		{
			const int32 Side = Id / Rank::UnitIdSideFactor;
			const int32 Rest = Id % Rank::UnitIdSideFactor;
			const int32 Row = Rest / Rank::UnitIdRowFactor;
			const int32 Col = Rest % Rank::UnitIdRowFactor;
			if (Id < 0 || Side >= Rank::NumSides || Row >= Rank::BoardRows || Col >= Rank::BoardCols)
			{
				return nullptr;
			}
			const int32 Index = Grid[Side][Row][Col];
			return Index >= 0 ? &Units[Index] : nullptr;
		}

		const FUnitFacts* At(int32 Side, int32 Row, int32 Col) const
		{
			if (Side < 0 || Side >= Rank::NumSides || Row < 0 || Row >= Rank::BoardRows || Col < 0 || Col >= Rank::BoardCols)
			{
				return nullptr;
			}
			const int32 Index = Grid[Side][Row][Col];
			return Index >= 0 ? &Units[Index] : nullptr;
		}

		// 진형판 밖의 열은 빈 열로 본다(§9 8번)
		int32 Count(int32 Side, int32 Col) const
		{
			return (Col >= 0 && Col < Rank::BoardCols) ? ColumnCount[Side][Col] : 0;
		}

		const FUnitFacts* Front(int32 Side, int32 Col) const
		{
			for (int32 Row = 0; Row < Rank::BoardRows; ++Row)
			{
				if (const FUnitFacts* Unit = At(Side, Row, Col))
				{
					return Unit;
				}
			}
			return nullptr;
		}

		const FUnitFacts* Back(int32 Side, int32 Col) const
		{
			for (int32 Row = Rank::BoardRows - 1; Row >= 0; --Row)
			{
				if (const FUnitFacts* Unit = At(Side, Row, Col))
				{
					return Unit;
				}
			}
			return nullptr;
		}

		bool AnyUnit(TFunctionRef<bool(const FUnitFacts&)> Pred) const
		{
			for (const FUnitFacts& Unit : Units)
			{
				if (Pred(Unit))
				{
					return true;
				}
			}
			return false;
		}

		bool AnyEvent(TFunctionRef<bool(const Rank::FBattleEvent&)> Pred) const
		{
			for (const Rank::FBattleEvent& Event : Result->Log)
			{
				if (Pred(Event))
				{
					return true;
				}
			}
			return false;
		}

		bool Is(const FUnitFacts& Unit, Rank::ETarget Target) const
		{
			return Unit.Def != nullptr && Unit.Def->Target == Target;
		}
	};

	// 갈래 하나를 이 대전이 지났는지. bOutKnown이 false면 이 갈래의 확인 방법이 구현되지 않았다.
	bool Detect(const FString& Id, FBattleFacts& F, bool& bOutKnown)
	{
		using namespace Rank;
		bOutKnown = true;
		const FBattleResult& R = *F.Result;
		const int32 E = 1; // 상대 진영 = 1 - Side
		auto Facing = [&](const FUnitFacts& U) { return F.Count(E - U.Side, U.Col); };

		// 배치: 한 열에 2명 이상
		if (Id == TEXT("br01")) { return F.AnyUnit([&](const FUnitFacts& U) { return F.ColumnCount[U.Side][U.Col] >= 2; }); }
		// 배치: 앞 행이 비고 뒤 행에 병사
		if (Id == TEXT("br02")) { return F.AnyUnit([&](const FUnitFacts& U) { return U.Rank0 < U.Row; }); }
		// 배치: 그 종류의 링크를 받은 병사가 있다(ComputeLinks)
		if (Id == TEXT("br03")) { return F.AnyUnit([](const FUnitFacts& U) { return U.Links.Count[static_cast<int32>(ELinkStat::Armor)] > 0; }); }
		if (Id == TEXT("br04")) { return F.AnyUnit([](const FUnitFacts& U) { return U.Links.Count[static_cast<int32>(ELinkStat::Atk)] > 0; }); }
		if (Id == TEXT("br05")) { return F.AnyUnit([](const FUnitFacts& U) { return U.Links.Count[static_cast<int32>(ELinkStat::Range)] > 0; }); }
		if (Id == TEXT("br06")) { return F.AnyUnit([](const FUnitFacts& U) { return U.Links.Count[static_cast<int32>(ELinkStat::Hp)] > 0; }); }
		if (Id == TEXT("br07")) { return F.AnyUnit([](const FUnitFacts& U) { return U.Links.Count[static_cast<int32>(ELinkStat::Interval)] > 0; }); }
		// 배치: 같은 병과가 맞닿음(오른쪽, 아래만 보면 모든 쌍을 한 번씩 본다)
		if (Id == TEXT("br08"))
		{
			return F.AnyUnit([&](const FUnitFacts& U) {
				const FUnitFacts* Right = F.At(U.Side, U.Row, U.Col + 1);
				const FUnitFacts* Down = F.At(U.Side, U.Row + 1, U.Col);
				return (Right && Right->Key == U.Key) || (Down && Down->Key == U.Key); });
		}
		// 배치: 다른 병과가 대각선으로 닿음
		if (Id == TEXT("br09"))
		{
			return F.AnyUnit([&](const FUnitFacts& U) {
				const FUnitFacts* DownRight = F.At(U.Side, U.Row + 1, U.Col + 1);
				const FUnitFacts* DownLeft = F.At(U.Side, U.Row + 1, U.Col - 1);
				return (DownRight && DownRight->Key != U.Key) || (DownLeft && DownLeft->Key != U.Key); });
		}
		// 배치: 한 병과에게서 받은 겹이 겹 상한을 넘는다(ComputeLinks의 겹 상한 적용 전 수)
		if (Id == TEXT("br10"))
		{
			return F.AnyUnit([&](const FUnitFacts& U) {
				for (int32 Index = 0; Index < U.Links.NumGivers; ++Index)
				{
					if (U.Links.Givers[Index].Count > F.Input->Rules.LinkStackMax) { return true; }
				}
				return false; });
		}
		// 배치: 링크 효과가 없는 서로 다른 병과(해골)가 맞닿음
		if (Id == TEXT("br13"))
		{
			auto NoLink = [](const FUnitFacts* U) { return U && U->Def && U->Def->LinkStat == ELinkStat::None; };
			return F.AnyUnit([&](const FUnitFacts& U) {
				const FUnitFacts* Right = F.At(U.Side, U.Row, U.Col + 1);
				const FUnitFacts* Down = F.At(U.Side, U.Row + 1, U.Col);
				return NoLink(&U) && ((NoLink(Right) && Right->Key != U.Key) || (NoLink(Down) && Down->Key != U.Key)); });
		}
		// 배치: 진영 1에 링크를 받은 병사가 있다
		if (Id == TEXT("br14"))
		{
			return F.AnyUnit([](const FUnitFacts& U) {
				if (U.Side != 1) { return false; }
				for (int32 Stat = 0; Stat < static_cast<int32>(ELinkStat::Count); ++Stat)
				{
					if (U.Links.Count[Stat] > 0) { return true; }
				}
				return false; });
		}
		// 로그: 링크를 준 병사가 쓰러진 뒤 받은 병사가 치거나 맞음
		if (Id == TEXT("br15"))
		{
			const int32 Offsets[4][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };
			for (const FUnitFacts& Receiver : F.Units)
			{
				for (int32 Dir = 0; Dir < 4; ++Dir)
				{
					const FUnitFacts* Giver = F.At(Receiver.Side, Receiver.Row + Offsets[Dir][0], Receiver.Col + Offsets[Dir][1]);
					if (Giver == nullptr || Giver->DeathTick == 0)
					{
						continue;
					}
					bool bGave = false;
					for (int32 Index = 0; Index < Receiver.Links.NumGivers; ++Index)
					{
						bGave = bGave || Receiver.Links.Givers[Index].Key == Giver->Key;
					}
					if (bGave && F.AnyEvent([&](const FBattleEvent& Ev) {
						return IsAttack(Ev) && Ev.Tick > Giver->DeathTick && (Ev.Actor == Receiver.Id || Ev.Target == Receiver.Id); }))
					{
						return true;
					}
				}
			}
			return false;
		}
		// 로그: 첫 공격이 간격 번째 틱
		if (Id == TEXT("br16")) { return F.AnyUnit([](const FUnitFacts& U) { return U.FirstAttackTick > 0 && U.FirstAttackTick == U.Stats.Interval; }); }
		// 로그: 이벤트가 없는 틱이 있다
		if (Id == TEXT("br17"))
		{
			int32 TicksWithEvents = 0;
			int32 LastTick = 0;
			for (const FBattleEvent& Ev : R.Log)
			{
				if (Ev.Tick != LastTick) { ++TicksWithEvents; LastTick = Ev.Tick; }
			}
			return TicksWithEvents < R.Tick;
		}
		// 배치: 대열 순번이 사거리 이상
		if (Id == TEXT("br18")) { return F.AnyUnit([](const FUnitFacts& U) { return U.Rank0 >= U.Stats.Range; }); }
		// 배치: 대열 순번 = 사거리 − 1(1 이상)
		if (Id == TEXT("br19")) { return F.AnyUnit([](const FUnitFacts& U) { return U.Rank0 >= 1 && U.Rank0 == U.Stats.Range - 1; }); }
		// 로그: 앞사람이 쓰러진 다음 틱에 당겨진 병사가 침
		if (Id == TEXT("br20"))
		{
			for (const FUnitFacts& Dead : F.Units)
			{
				if (Dead.DeathTick == 0) { continue; }
				for (int32 Row = Dead.Row + 1; Row < BoardRows; ++Row)
				{
					const FUnitFacts* Behind = F.At(Dead.Side, Row, Dead.Col);
					if (Behind == nullptr || (Behind->DeathTick != 0 && Behind->DeathTick <= Dead.DeathTick)) { continue; }
					if (F.AnyEvent([&](const FBattleEvent& Ev) { return IsAttack(Ev) && Ev.Actor == Behind->Id && Ev.Tick == Dead.DeathTick + 1; }))
					{
						return true;
					}
				}
			}
			return false;
		}
		// 배치: 표적 종류와 마주 보는 열의 상태(§4.2)
		if (Id == TEXT("br21")) { return F.AnyUnit([&](const FUnitFacts& U) { return F.Is(U, ETarget::Front) && Facing(U) > 0; }); }
		if (Id == TEXT("br22")) { return F.AnyUnit([&](const FUnitFacts& U) { return F.Is(U, ETarget::Front) && Facing(U) == 0; }); }
		if (Id == TEXT("br23")) { return F.AnyUnit([&](const FUnitFacts& U) { return F.Is(U, ETarget::Back) && Facing(U) >= 2; }); }
		if (Id == TEXT("br24")) { return F.AnyUnit([&](const FUnitFacts& U) { return F.Is(U, ETarget::Back) && Facing(U) == 0; }); }
		if (Id == TEXT("br25")) { return F.AnyUnit([&](const FUnitFacts& U) { return F.Is(U, ETarget::Support) && Facing(U) > 0; }); }
		if (Id == TEXT("br26"))
		{
			return F.AnyUnit([&](const FUnitFacts& U) {
				const int32 S = E - U.Side;
				return F.Is(U, ETarget::Support) && F.Count(S, U.Col) == 0 && F.Count(S, U.Col - 1) > 0 && F.Count(S, U.Col + 1) > 0; });
		}
		if (Id == TEXT("br27"))
		{
			return F.AnyUnit([&](const FUnitFacts& U) {
				const int32 S = E - U.Side;
				if (!F.Is(U, ETarget::Support) || F.Count(S, U.Col) > 0 || F.Count(S, U.Col - 1) > 0 || F.Count(S, U.Col + 1) > 0) { return false; }
				for (int32 Col = 0; Col < BoardCols; ++Col) { if (F.Count(S, Col) > 0) { return true; } }
				return false; });
		}
		if (Id == TEXT("br29"))
		{
			return F.AnyUnit([&](const FUnitFacts& U) {
				const int32 S = E - U.Side;
				return F.Is(U, ETarget::Area) && U.Col >= 1 && U.Col <= BoardCols - 2
					&& F.Count(S, U.Col - 1) > 0 && F.Count(S, U.Col) > 0 && F.Count(S, U.Col + 1) > 0; });
		}
		if (Id == TEXT("br30"))
		{
			return F.AnyUnit([&](const FUnitFacts& U) {
				if (!F.Is(U, ETarget::Area)) { return false; }
				int32 Empty = 0;
				int32 Filled = 0;
				for (int32 Col = U.Col - 1; Col <= U.Col + 1; ++Col)
				{
					if (Col < 0 || Col >= BoardCols) { continue; }
					(F.Count(E - U.Side, Col) > 0 ? Filled : Empty) += 1;
				}
				return Empty > 0 && Filled > 0; });
		}
		if (Id == TEXT("br31"))
		{
			return F.AnyUnit([&](const FUnitFacts& U) {
				const int32 S = E - U.Side;
				return F.Is(U, ETarget::Area) && (U.Col == 0 || U.Col == BoardCols - 1)
					&& F.Count(S, U.Col - 1) == 0 && F.Count(S, U.Col) == 0 && F.Count(S, U.Col + 1) == 0; });
		}
		if (Id == TEXT("br32"))
		{
			return F.AnyUnit([&](const FUnitFacts& U) {
				const int32 S = E - U.Side;
				return F.Is(U, ETarget::Area) && U.Col >= 1 && U.Col <= BoardCols - 2
					&& F.Count(S, U.Col - 1) == 0 && F.Count(S, U.Col) == 0 && F.Count(S, U.Col + 1) == 0; });
		}
		// 배치: 물리 공격자의 공격이 마주 보는 표적의 방어 이하(최소 피해 1이 쓰인다)
		if (Id == TEXT("br33"))
		{
			return F.AnyUnit([&](const FUnitFacts& U) {
				if (U.Def == nullptr || U.Def->bMagic) { return false; }
				const FUnitFacts* Target = nullptr;
				if (F.Is(U, ETarget::Front) || F.Is(U, ETarget::Support)) { Target = F.Front(E - U.Side, U.Col); }
				else if (F.Is(U, ETarget::Back)) { Target = F.Back(E - U.Side, U.Col); }
				return Target != nullptr && U.Stats.Atk - Target->Stats.Armor < 1; });
		}
		// 배치: 마법 공격자가 칠 수 있는 열의 맨 앞에 방어가 있는 병사
		if (Id == TEXT("br34"))
		{
			return F.AnyUnit([&](const FUnitFacts& U) {
				if (U.Def == nullptr || !U.Def->bMagic) { return false; }
				const int32 Reach = F.Is(U, ETarget::Area) ? 1 : 0;
				for (int32 Col = U.Col - Reach; Col <= U.Col + Reach; ++Col)
				{
					const FUnitFacts* Target = F.Front(E - U.Side, Col);
					if (Target != nullptr && Target->Stats.Armor > 0) { return true; }
				}
				return false; });
		}
		// 배치: 범위 공격자가 칠 수 있는 열 가운데 2개 이상에 병사
		if (Id == TEXT("br35"))
		{
			return F.AnyUnit([&](const FUnitFacts& U) {
				if (!F.Is(U, ETarget::Area)) { return false; }
				int32 Filled = 0;
				for (int32 Col = U.Col - 1; Col <= U.Col + 1; ++Col) { Filled += F.Count(E - U.Side, Col) > 0 ? 1 : 0; }
				return Filled >= 2; });
		}
		// 로그: 성문(결계) 타격
		if (Id == TEXT("br36")) { return F.AnyEvent([](const FBattleEvent& Ev) { return Ev.Type == static_cast<uint8>(EEventType::GateHit); }); }
		// 로그: 체력을 이벤트로 따라가며 본다(재생 화면과 같은 방법: 처음 최대 체력, 피해는 빼고, 부활은 값으로 바꾼다)
		if (Id == TEXT("br37") || Id == TEXT("br38"))
		{
			TArray<int32> Hp;
			for (const FUnitFacts& U : F.Units) { Hp.Add(U.Stats.MaxHp); }
			bool bOverkillOnDead = false;
			bool bOverflow = false;
			for (const FBattleEvent& Ev : R.Log)
			{
				FUnitFacts* Target = F.ById(Ev.Target);
				if (Target == nullptr) { continue; }
				int32& TargetHp = Hp[static_cast<int32>(Target - F.Units.GetData())];
				if (Ev.Type == static_cast<uint8>(EEventType::Damage))
				{
					bOverkillOnDead = bOverkillOnDead || TargetHp <= 0;
					bOverflow = bOverflow || (TargetHp > 0 && Ev.Value > TargetHp);
					TargetHp -= Ev.Value;
				}
				else if (Ev.Type == static_cast<uint8>(EEventType::Revive))
				{
					TargetHp = Ev.Value;
				}
			}
			return Id == TEXT("br37") ? bOverkillOnDead : bOverflow;
		}
		// 결과: 성문이나 결계가 음수
		if (Id == TEXT("br39")) { return R.Gate[0] < 0 || R.Gate[1] < 0; }
		// 로그: 부활, 부활 뒤 사망
		if (Id == TEXT("br40")) { return F.AnyUnit([](const FUnitFacts& U) { return U.ReviveTick > 0; }); }
		if (Id == TEXT("br41")) { return F.AnyUnit([](const FUnitFacts& U) { return U.ReviveTick > 0 && U.DeathTick > U.ReviveTick; }); }
		// 로그: 한 틱에 부활과 (다른 병사의) 사망
		if (Id == TEXT("br42"))
		{
			return F.AnyUnit([&](const FUnitFacts& U) {
				return U.ReviveTick > 0 && F.AnyUnit([&](const FUnitFacts& Other) { return Other.Id != U.Id && Other.DeathTick == U.ReviveTick; }); });
		}
		// 로그: 같은 틱 같은 열(같은 진영)의 사망 2건 이상
		if (Id == TEXT("br43"))
		{
			return F.AnyUnit([&](const FUnitFacts& U) {
				return U.DeathTick > 0 && F.AnyUnit([&](const FUnitFacts& Other) {
					return Other.Id != U.Id && Other.Side == U.Side && Other.Col == U.Col && Other.DeathTick == U.DeathTick; }); });
		}
		// 로그: 같은 틱에 서로를 친 두 병사가 모두 쓰러짐
		if (Id == TEXT("br44"))
		{
			for (const FBattleEvent& Ev : R.Log)
			{
				if (Ev.Type != static_cast<uint8>(EEventType::Damage)) { continue; }
				const FUnitFacts* A = F.ById(Ev.Actor);
				const FUnitFacts* B = F.ById(Ev.Target);
				if (A == nullptr || B == nullptr || A->DeathTick != Ev.Tick || B->DeathTick != Ev.Tick) { continue; }
				if (F.AnyEvent([&](const FBattleEvent& Back) {
					return Back.Type == static_cast<uint8>(EEventType::Damage) && Back.Tick == Ev.Tick && Back.Actor == B->Id && Back.Target == A->Id; }))
				{
					return true;
				}
			}
			return false;
		}
		// 결과: 플레이어 기준 판정(§5.2)
		if (Id == TEXT("br45")) { return F.Verdict.Kind == EVerdictKind::Annihilation; }
		if (Id == TEXT("br46")) { return F.Verdict.Kind == EVerdictKind::BarrierBreak; }
		if (Id == TEXT("br47")) { return F.Verdict.Kind == EVerdictKind::Dawn; }
		if (Id == TEXT("br48")) { return R.Outcome == EOutcome::Draw && F.Verdict.Kind == EVerdictKind::GateHeld; }
		if (Id == TEXT("br49")) { return R.Outcome == EOutcome::Draw && F.Verdict.Kind == EVerdictKind::GateFall; }
		if (Id == TEXT("br50")) { return R.Outcome == EOutcome::B && F.Verdict.Kind == EVerdictKind::GateFall; }
		if (Id == TEXT("br51")) { return F.Verdict.Kind == EVerdictKind::Wiped; }
		// 결과: 틱이 최대 틱인데 Timeout이 아님
		if (Id == TEXT("br52")) { return R.Tick == F.Input->Rules.MaxTicks && R.Outcome != EOutcome::Timeout; }
		// 로그 체크섬: 부활 줄과 사망 줄이 모두 있다(값 칸이 체크섬에 들어간다)
		if (Id == TEXT("br55")) { return F.AnyUnit([](const FUnitFacts& U) { return U.ReviveTick > 0; }) && F.AnyUnit([](const FUnitFacts& U) { return U.DeathTick > 0; }); }
		// 이벤트 수, 해시: 모든 대전이 지난다
		if (Id == TEXT("br56") || Id == TEXT("br57")) { return R.Outcome != EOutcome::Invalid; }

		bOutKnown = false;
		return false;
	}

	bool LoadBranches(TArray<FBranch>& OutBranches, FString& OutError)
	{
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *ProjectPath(BranchesFile)))
		{
			OutError = FString::Printf(TEXT("파일을 읽지 못했다: %s"), BranchesFile);
			return false;
		}
		TArray<FString> Lines;
		TArray<int32> LineNumbers;
		RankContent::SplitCsvLines(Text, Lines, LineNumbers);
		const FString Header = TEXT("id,clause,branch,static,tests,battles,detect");
		if (Lines.Num() == 0 || Lines[0] != Header)
		{
			OutError = FString::Printf(TEXT("%s: 첫 줄이 \"%s\"가 아니다"), BranchesFile, *Header);
			return false;
		}
		for (int32 Index = 1; Index < Lines.Num(); ++Index)
		{
			TArray<FString> Fields;
			if (!RankContent::SplitCsvLine(Lines[Index], Fields, OutError) || Fields.Num() != 7)
			{
				OutError = FString::Printf(TEXT("%s %d줄: 칸이 7개여야 한다"), BranchesFile, LineNumbers[Index]);
				return false;
			}
			FBranch Branch;
			Branch.Id = Fields[0];
			Branch.Clause = Fields[1];
			Branch.Text = Fields[2];
			Branch.Static = Fields[3];
			Branch.Tests = Fields[4];
			Branch.Battles = Fields[5];
			Branch.Detect = Fields[6];
			TArray<FString> Tests;
			Branch.Tests.ParseIntoArray(Tests, TEXT(";"), true);
			Branch.bT1 = Tests.Contains(TEXT("T1"));
			OutBranches.Add(MoveTemp(Branch));
		}
		return true;
	}

	FString CsvField(const FString& Text)
	{
		if (!Text.Contains(TEXT(",")) && !Text.Contains(TEXT("\"")))
		{
			return Text;
		}
		return TEXT("\"") + Text.Replace(TEXT("\""), TEXT("\"\"")) + TEXT("\"");
	}
}

URankRegressionCommandlet::URankRegressionCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
	ShowErrorCount = true;
}

int32 URankRegressionCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> ParamMap;
	ParseCommandLine(*Params, Tokens, Switches, ParamMap);
	const bool bWrite = Switches.Contains(TEXT("write"));
	const bool bOverwrite = Switches.Contains(TEXT("overwrite"));
	const FString* CommitParam = ParamMap.Find(TEXT("commit"));
	const FString* OutParam = ParamMap.Find(TEXT("out"));
	const FString OutPath = OutParam ? FPaths::ConvertRelativePathToFull(*OutParam) : ProjectPath(RecordFile);
	// -inputs=<경로>: 입력 수정안을 시험할 때만 쓴다. 고정 입력 파일을 고치지 않고 갈래 집계를 볼 수 있다. 이 인자로는 기록하지 않는다.
	const FString* InputsParam = ParamMap.Find(TEXT("inputs"));
	const FString InputsPath = InputsParam ? FPaths::ConvertRelativePathToFull(*InputsParam) : ProjectPath(InputsFile);
	if (InputsParam && bWrite)
	{
		UE_LOGFMT(LogRankRegression, Error, "-inputs는 수정안 시험용이다. -write와 함께 쓰지 않는다.");
		return 1;
	}

	// 1. 수치, 입력, 갈래 목록
	TArray<FRankUnitRow> UnitRows;
	FRankRulesRow RulesRow;
	TArray<Rank::FUnitDef> Defs;
	Rank::FRuleParams Rules;
	FString Error;
	if (!RankContent::LoadDefsFiles(ProjectPath(TEXT("Data/defs/units.csv")), ProjectPath(TEXT("Data/defs/rules.csv")), UnitRows, RulesRow, Defs, Rules, Error))
	{
		UE_LOGFMT(LogRankRegression, Error, "Data/defs를 읽지 못했다: {0}", Error);
		return 1;
	}
	FString InputsText;
	TArray<RankRegression::FBattleInputRow> Inputs;
	UE_LOGFMT(LogRankRegression, Display, "입력: {0}", InputsPath);
	if (!FFileHelper::LoadFileToString(InputsText, *InputsPath) || !RankRegression::ParseInputsCsv(InputsText, Inputs, Error))
	{
		UE_LOGFMT(LogRankRegression, Error, "회귀 입력을 읽지 못했다: {0}", Error);
		return 1;
	}
	TArray<FBranch> Branches;
	if (!LoadBranches(Branches, Error))
	{
		UE_LOGFMT(LogRankRegression, Error, "{0}", Error);
		return 1;
	}

	// 2. 대전마다 돌리고 갈래를 센다
	bool bProblem = false;
	RankRegression::FRecordFile NewFile;
	NewFile.Commit = CommitParam ? *CommitParam : FString();
	NewFile.Units = UnitRows;
	NewFile.Rules = RulesRow;
	TArray<TArray<FString>> HitBattles;
	HitBattles.SetNum(Branches.Num());
	TArray<bool> Known;
	Known.Init(true, Branches.Num());

	for (const RankRegression::FBattleInputRow& Row : Inputs)
	{
		Rank::FBattleInput Input;
		Input.Side[0] = Row.Ally;
		Input.Side[1] = Row.Enemy;
		Input.Rules = Rules;
		Input.Defs = Defs.GetData();
		Input.NumDefs = Defs.Num();
		const Rank::FBattleResult Result = Rank::Simulate(Input, true);

		RankRegression::FRecordedBattle Battle;
		Battle.Id = Row.Id;
		Battle.AllyRows = Row.AllyRows;
		Battle.EnemyRows = Row.EnemyRows;
		if (Result.Outcome == Rank::EOutcome::Invalid || !RankRegression::MakeRecord(Result, Battle.Record))
		{
			UE_LOGFMT(LogRankRegression, Error, "{0}: 입력 오류(Invalid)다. 회귀 입력은 전투가 돌아야 한다", Row.Id);
			bProblem = true;
			continue;
		}

		FBattleFacts Facts;
		Facts.Build(Input, Result);
		for (int32 Index = 0; Index < Branches.Num(); ++Index)
		{
			bool bKnown = true;
			if (Detect(Branches[Index].Id, Facts, bKnown))
			{
				HitBattles[Index].Add(Row.Id);
				Battle.Branches.Add(Branches[Index].Id);
			}
			Known[Index] = Known[Index] && bKnown;
		}
		NewFile.Battles.Add(MoveTemp(Battle));
	}

	// 3. 갈래 집계 표
	TArray<FString> InputIds;
	for (const RankRegression::FBattleInputRow& Row : Inputs) { InputIds.Add(Row.Id); }
	FString Csv = TEXT("id,clause,tests,static,expected,count,battles,missing,note\n");
	TArray<FString> ZeroT1;
	TArray<FString> UnknownT1;
	UE_LOGFMT(LogRankRegression, Display, "갈래 집계: id | 맡는 테스트 | 지난 대전 수 | 지난 대전 | 목록에 적힌 대전 가운데 지나지 않은 것");
	for (int32 Index = 0; Index < Branches.Num(); ++Index)
	{
		const FBranch& Branch = Branches[Index];
		FString Note;
		if (!Known[Index])
		{
			Note = Branch.bT1 ? TEXT("확인 방법이 구현되지 않았다") : TEXT("T1 대상 아님");
			if (Branch.bT1) { UnknownT1.Add(Branch.Id); }
		}
		// 목록에 적힌 대전: rbXX는 그 대전, "전부"는 모든 대전. "실전"은 특정 대전을 정하지 않으므로 빠짐을 세지 않는다.
		TArray<FString> Expected;
		Branch.Battles.ParseIntoArray(Expected, TEXT(";"), true);
		TArray<FString> Missing;
		for (const FString& Item : Expected)
		{
			if (Item == TEXT("전부"))
			{
				for (const FString& InputId : InputIds) { if (!HitBattles[Index].Contains(InputId)) { Missing.AddUnique(InputId); } }
			}
			else if (Item.StartsWith(TEXT("rb")) && !HitBattles[Index].Contains(Item))
			{
				Missing.AddUnique(Item);
			}
		}
		if (Known[Index] && Branch.bT1 && HitBattles[Index].Num() == 0)
		{
			ZeroT1.Add(Branch.Id);
			Note = TEXT("T1 갈래인데 0건");
		}
		const FString Hits = FString::Join(HitBattles[Index], TEXT(";"));
		const FString MissingText = FString::Join(Missing, TEXT(";"));
		UE_LOGFMT(LogRankRegression, Display, "{0} | {1} | {2} | {3} | {4} {5}",
			Branch.Id, Branch.Tests, Known[Index] ? FString::FromInt(HitBattles[Index].Num()) : FString(TEXT("-")), Hits,
			MissingText.IsEmpty() ? FString(TEXT("-")) : MissingText, Note);
		Csv += FString::Printf(TEXT("%s,%s,%s,%s,%s,%s,%s,%s,%s\n"),
			*CsvField(Branch.Id), *CsvField(Branch.Clause), *CsvField(Branch.Tests), *CsvField(Branch.Static), *CsvField(Branch.Battles),
			Known[Index] ? *FString::FromInt(HitBattles[Index].Num()) : TEXT(""), *CsvField(Hits), *CsvField(MissingText), *CsvField(Note));
	}
	const FString TallyPath = ProjectPath(TallyFile);
	if (!FFileHelper::SaveStringToFile(Csv, *TallyPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOGFMT(LogRankRegression, Error, "갈래 집계 파일을 쓰지 못했다: {0}", TallyPath);
		bProblem = true;
	}
	UE_LOGFMT(LogRankRegression, Display, "갈래 집계 파일: {0}", TallyPath);
	if (UnknownT1.Num() > 0)
	{
		UE_LOGFMT(LogRankRegression, Error, "확인 방법이 구현되지 않은 T1 갈래: {0}", FString::Join(UnknownT1, TEXT(", ")));
		bProblem = true;
	}
	if (ZeroT1.Num() > 0)
	{
		UE_LOGFMT(LogRankRegression, Error, "T1이 맡았는데 지난 대전이 0건인 갈래: {0}. 입력을 고쳐야 기록할 수 있다(사람 승인)", FString::Join(ZeroT1, TEXT(", ")));
		bProblem = true;
	}

	// 4. 지금 기록 파일과 비교
	const bool bRecordExists = IFileManager::Get().FileExists(*OutPath);
	if (bRecordExists)
	{
		FString OldText;
		RankRegression::FRecordFile OldFile;
		if (!FFileHelper::LoadFileToString(OldText, *OutPath) || !RankRegression::ReadRecordJson(OldText, OldFile, Error))
		{
			UE_LOGFMT(LogRankRegression, Error, "지금 기록 파일을 읽지 못했다: {0}", Error);
			bProblem = true;
		}
		else
		{
			UE_LOGFMT(LogRankRegression, Display, "지금 기록 파일과 비교(기록 커밋 {0})", OldFile.Commit);
			TArray<FString> DefDiffs;
			RankRegression::DiffDefs(OldFile.Units, OldFile.Rules, TEXT("기록"), UnitRows, RulesRow, TEXT("지금"), DefDiffs);
			for (const FString& Diff : DefDiffs) { UE_LOGFMT(LogRankRegression, Display, "  수치 {0}", Diff); }
			for (const RankRegression::FRecordedBattle& New : NewFile.Battles)
			{
				const RankRegression::FRecordedBattle* Old = OldFile.Battles.FindByPredicate([&New](const RankRegression::FRecordedBattle& B) { return B.Id == New.Id; });
				if (Old == nullptr)
				{
					UE_LOGFMT(LogRankRegression, Display, "  {0}: 기록에 없다", New.Id);
					continue;
				}
				TArray<FString> Diffs;
				RankRegression::DiffRecords(Old->Record, New.Record, Diffs);
				UE_LOGFMT(LogRankRegression, Display, "  {0}: {1}", New.Id, Diffs.Num() == 0 ? FString(TEXT("같음")) : FString::Join(Diffs, TEXT(" · ")));
			}
		}
	}
	else
	{
		UE_LOGFMT(LogRankRegression, Display, "기록 파일이 없다: {0}", OutPath);
	}

	// 5. 기록 형식 자체 점검(파일은 쓰지 않는다): 글로 만들고 다시 읽어 같은지, 다시 글로 만들면 같은 글인지 본다.
	// 글의 CRC32를 로그에 낸다. 같은 커밋에서 두 번 돌려 CRC가 같으면 바이트까지 같은 파일이 나온다(tools/AGENTS.md).
	{
		const FString Text = RankRegression::WriteRecordJson(NewFile);
		RankRegression::FRecordFile RoundTrip;
		bool bSame = RankRegression::ReadRecordJson(Text, RoundTrip, Error) && RankRegression::WriteRecordJson(RoundTrip) == Text
			&& RoundTrip.Battles.Num() == NewFile.Battles.Num();
		for (int32 Index = 0; bSame && Index < NewFile.Battles.Num(); ++Index)
		{
			TArray<FString> Diffs;
			RankRegression::DiffRecords(NewFile.Battles[Index].Record, RoundTrip.Battles[Index].Record, Diffs);
			bSame = Diffs.Num() == 0 && NewFile.Battles[Index].Branches == RoundTrip.Battles[Index].Branches;
		}
		const FTCHARToUTF8 Utf8(*Text);
		const uint32 Crc = FCrc::MemCrc32(Utf8.Get(), Utf8.Length());
		if (bSame)
		{
			UE_LOGFMT(LogRankRegression, Display, "기록 형식 자체 점검: 통과(대전 {0}건, UTF-8 {1}바이트, CRC32 {2})", NewFile.Battles.Num(), Utf8.Length(), RankRegression::HexU32(Crc));
		}
		else
		{
			UE_LOGFMT(LogRankRegression, Error, "기록 형식 자체 점검: 실패. 쓰고 다시 읽은 내용이 다르다({0})", Error);
			bProblem = true;
		}
	}

	// 6. 쓰기
	if (!bWrite)
	{
		UE_LOGFMT(LogRankRegression, Display, "집계만 했다. 기록 파일은 쓰지 않았다(-write 없음).");
		return bProblem ? 1 : 0;
	}
	if (bProblem)
	{
		UE_LOGFMT(LogRankRegression, Error, "위 문제 때문에 기록 파일을 쓰지 않았다.");
		return 1;
	}
	if (NewFile.Commit.IsEmpty())
	{
		UE_LOGFMT(LogRankRegression, Error, "-commit=<커밋>이 없어 기록 파일을 쓰지 않았다.");
		return 1;
	}
	if (bRecordExists && !bOverwrite)
	{
		UE_LOGFMT(LogRankRegression, Error, "기록 파일이 이미 있다. 위 비교표를 사람에게 보이고 승인을 받은 뒤 -overwrite로 다시 실행한다.");
		return 1;
	}
	if (!FFileHelper::SaveStringToFile(RankRegression::WriteRecordJson(NewFile), *OutPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOGFMT(LogRankRegression, Error, "기록 파일을 쓰지 못했다: {0}", OutPath);
		return 1;
	}
	UE_LOGFMT(LogRankRegression, Display, "기록 파일을 썼다: {0} (대전 {1}건, 커밋 {2})", OutPath, NewFile.Battles.Num(), NewFile.Commit);
	return 0;
}

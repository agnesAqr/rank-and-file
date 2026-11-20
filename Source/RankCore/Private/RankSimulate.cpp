#include "RankBattle.h"
#include "RankHash.h"

namespace Rank
{
	namespace
	{
		struct FUnit
		{
			int32 Id = 0;
			int32 Side = 0;
			int32 Col = 0;
			int32 Hp = 0;
			int32 Atk = 0;
			int32 Armor = 0;
			int32 Interval = 0;
			int32 Range = 0;
			int32 Cd = 0;       // 대기 시간
			int32 Revive = 0;   // 남은 부활 값. 0이면 부활하지 않는다
			ETarget Target = ETarget::Front;
			bool bMagic = false;
		};

		// §4.1 ① 의도: 누가 누구를 치는가
		struct FIntent
		{
			int32 Attacker = 0;    // Units의 인덱스
			int32 TargetUnit = -1; // Units의 인덱스. -1이면 성문(결계)
			int32 TargetSide = 0;
			int32 Damage = 0;
		};

		struct FState
		{
			TArray<FUnit> Units;
			TArray<int32> Lines[NumSides][BoardCols]; // 대열: Units의 인덱스, 앞 → 뒤 (§2.3)
			int32 Gate[NumSides] = {};
		};

		bool InBoard(int32 Col)
		{
			return Col >= 0 && Col < BoardCols;
		}

		bool HasUnit(const FFormation& Formation)
		{
			for (int32 Row = 0; Row < BoardRows; ++Row)
			{
				for (int32 Col = 0; Col < BoardCols; ++Col)
				{
					if (Formation.Cells[Row][Col] != 0)
					{
						return true;
					}
				}
			}
			return false;
		}

		// §3.6 시작 조건. 진형판 크기는 규칙 상수(BoardCols, BoardRows)라 입력으로 받지 않는다(§7, §9 18번).
		// 같은 링크 종류를 두 병과 이상이 주는 데이터도 받는다. 겹은 병과마다 따로 센다(§3.4, §9 17번).
		bool ValidateInput(const FBattleInput& Input)
		{
			if (Input.Defs == nullptr || Input.NumDefs <= 0)
			{
				return false;
			}
			for (int32 Side = 0; Side < NumSides; ++Side)
			{
				const FFormation& Formation = Input.Side[Side];
				if (!HasUnit(Formation))
				{
					return false; // §3.6 한쪽 진영이 비었다
				}
				for (int32 Row = 0; Row < BoardRows; ++Row)
				{
					for (int32 Col = 0; Col < BoardCols; ++Col)
					{
						const uint8 Key = Formation.Cells[Row][Col];
						if (Key != 0 && FindDef(Input.Defs, Input.NumDefs, Key) == nullptr)
						{
							return false; // §3.6 병과 수치에 없는 키
						}
					}
				}
			}
			return true;
		}

		// §3.1 진영 0 → 1, 행 0 → 3, 열 0 → 5 순서로 유닛을 만들고 자기 열의 대열 맨 뒤에 붙인다.
		void BuildUnits(const FBattleInput& Input, FState& State)
		{
			State.Units.Reserve(NumSides * BoardRows * BoardCols);
			for (int32 Side = 0; Side < NumSides; ++Side)
			{
				const FFormation& Formation = Input.Side[Side];
				for (int32 Row = 0; Row < BoardRows; ++Row)
				{
					for (int32 Col = 0; Col < BoardCols; ++Col)
					{
						const uint8 Key = Formation.Cells[Row][Col];
						if (Key == 0)
						{
							continue;
						}
						const FUnitDef* Def = FindDef(Input.Defs, Input.NumDefs, Key);
						FUnitStats Stats;
						ComputeFinalStats(Formation, Row, Col, Input.Defs, Input.NumDefs, Input.Rules, Stats);

						FUnit Unit;
						Unit.Id = Side * UnitIdSideFactor + Row * UnitIdRowFactor + Col; // §3.2
						Unit.Side = Side;
						Unit.Col = Col;
						Unit.Hp = Stats.MaxHp;          // §3.4 전투는 최대 체력으로 시작한다
						Unit.Atk = Stats.Atk;
						Unit.Armor = Stats.Armor;
						Unit.Interval = Stats.Interval;
						Unit.Range = Stats.Range;
						Unit.Cd = Stats.Interval;       // §3.5 대기 시간의 처음 값은 간격
						Unit.Revive = Def->Revive;
						Unit.Target = Def->Target;
						Unit.bMagic = Def->bMagic;

						State.Units.Add(Unit);
						State.Lines[Side][Col].Add(State.Units.Num() - 1);
					}
				}
			}
		}

		// §4.3 피해
		int32 DamageTo(const FUnit& Attacker, const FUnit& Target)
		{
			if (Attacker.bMagic)
			{
				return Attacker.Atk; // 마법은 방어를 무시한다
			}
			const int32 Raw = Attacker.Atk - Target.Armor;
			return Raw > 1 ? Raw : 1; // max(1, 공격 − 방어)
		}

		void AddUnitIntent(const FState& State, int32 AttackerIndex, int32 TargetIndex, TArray<FIntent>& Intents)
		{
			FIntent Intent;
			Intent.Attacker = AttackerIndex;
			Intent.TargetUnit = TargetIndex;
			Intent.TargetSide = State.Units[TargetIndex].Side;
			Intent.Damage = DamageTo(State.Units[AttackerIndex], State.Units[TargetIndex]);
			Intents.Add(Intent);
		}

		void AddGateIntent(const FState& State, int32 AttackerIndex, const FRuleParams& Rules, TArray<FIntent>& Intents)
		{
			FIntent Intent;
			Intent.Attacker = AttackerIndex;
			Intent.TargetUnit = -1;
			Intent.TargetSide = 1 - State.Units[AttackerIndex].Side;
			Intent.Damage = Rules.GateDamage; // §4.3 성문과 결계는 성문 피해 값
			Intents.Add(Intent);
		}

		// §4.2 표적. 틱이 시작될 때의 대열만 본다(①에서는 체력과 대열을 바꾸지 않는다).
		void AddTargets(const FState& State, int32 AttackerIndex, const FRuleParams& Rules, TArray<FIntent>& Intents)
		{
			const FUnit& Attacker = State.Units[AttackerIndex];
			const int32 Enemy = 1 - Attacker.Side;
			const TArray<int32>& Facing = State.Lines[Enemy][Attacker.Col]; // 마주 보는 열: 열 번호가 같은 열

			switch (Attacker.Target)
			{
			case ETarget::Front:
				if (Facing.Num() > 0)
				{
					AddUnitIntent(State, AttackerIndex, Facing[0], Intents);
				}
				else
				{
					AddGateIntent(State, AttackerIndex, Rules, Intents);
				}
				break;

			case ETarget::Back:
				if (Facing.Num() > 0)
				{
					AddUnitIntent(State, AttackerIndex, Facing[Facing.Num() - 1], Intents);
				}
				else
				{
					AddGateIntent(State, AttackerIndex, Rules, Intents);
				}
				break;

			case ETarget::Support:
			{
				if (Facing.Num() > 0)
				{
					AddUnitIntent(State, AttackerIndex, Facing[0], Intents);
					break;
				}
				// 거리 1, 2, …, 5 순서로 넓혀 가며, 같은 거리에서는 열 번호가 작은 쪽을 먼저 본다.
				for (int32 Distance = 1; Distance < BoardCols; ++Distance)
				{
					const int32 Candidates[2] = { Attacker.Col - Distance, Attacker.Col + Distance };
					for (int32 Index = 0; Index < 2; ++Index)
					{
						const int32 Col = Candidates[Index];
						if (InBoard(Col) && State.Lines[Enemy][Col].Num() > 0)
						{
							AddUnitIntent(State, AttackerIndex, State.Lines[Enemy][Col][0], Intents);
							return;
						}
					}
				}
				AddGateIntent(State, AttackerIndex, Rules, Intents);
				break;
			}

			case ETarget::Area:
			{
				// c−1, c, c+1 순서로 진형판 안에 있고 비어 있지 않은 열의 맨 앞을 모두 친다.
				// 진형판 밖의 열은 빈 열로 본다. 세 열이 모두 비면 성문을 한 번 친다(§9 5번, 8번).
				bool bHitAny = false;
				for (int32 Col = Attacker.Col - 1; Col <= Attacker.Col + 1; ++Col)
				{
					if (InBoard(Col) && State.Lines[Enemy][Col].Num() > 0)
					{
						AddUnitIntent(State, AttackerIndex, State.Lines[Enemy][Col][0], Intents);
						bHitAny = true;
					}
				}
				if (!bHitAny)
				{
					AddGateIntent(State, AttackerIndex, Rules, Intents);
				}
				break;
			}
			}
		}

		// §4.1 ① 진영 0 → 1, 열 0 → 5, 대열 앞 → 뒤 순서로 의도를 모은다.
		void CollectIntents(FState& State, const FRuleParams& Rules, TArray<FIntent>& Intents)
		{
			for (int32 Side = 0; Side < NumSides; ++Side)
			{
				for (int32 Col = 0; Col < BoardCols; ++Col)
				{
					const TArray<int32>& Line = State.Lines[Side][Col];
					for (int32 Rank = 0; Rank < Line.Num(); ++Rank)
					{
						const int32 UnitIndex = Line[Rank];
						FUnit& Unit = State.Units[UnitIndex];
						if (Unit.Cd > 0)
						{
							--Unit.Cd;
						}
						if (Unit.Cd > 0)
						{
							continue;
						}
						if (Rank >= Unit.Range)
						{
							continue; // §2.4 사거리 밖: 대기 시간을 0에 둔 채 기다린다
						}
						AddTargets(State, UnitIndex, Rules, Intents);
						Unit.Cd = Unit.Interval;
					}
				}
			}
		}

		void Emit(FBattleResult& Result, bool bRecordLog, int32 Tick, EEventType Type, int32 Actor, int32 Target, int32 Value)
		{
			++Result.NumEvents;
			if (bRecordLog)
			{
				FBattleEvent Event;
				Event.Tick = Tick;
				Event.Type = static_cast<uint8>(Type);
				Event.Actor = Actor;
				Event.Target = Target;
				Event.Value = Value;
				Result.Log.Add(Event);
			}
		}

		// §4.1 ③ 모은 순서대로 적용한다. 표적은 바꾸지 않고, 이미 쓰러진 표적에도 그대로 적용한다(§9 2번).
		// 피해는 계산한 값 그대로 기록하고, 체력과 성문은 0 아래로도 내려간다(§9 11번, 12번).
		void ApplyIntents(FState& State, const TArray<FIntent>& Intents, int32 Tick, uint32& Hash, FBattleResult& Result, bool bRecordLog)
		{
			for (int32 Index = 0; Index < Intents.Num(); ++Index)
			{
				const FIntent& Intent = Intents[Index];
				const int32 AttackerId = State.Units[Intent.Attacker].Id;
				if (Intent.TargetUnit < 0)
				{
					State.Gate[Intent.TargetSide] -= Intent.Damage;
					// §6 성문 타격: 틱, 1, 공격자 번호, 999, 피해
					HashMix(Hash, Tick);
					HashMix(Hash, static_cast<int32>(EEventType::GateHit));
					HashMix(Hash, AttackerId);
					HashMix(Hash, GateTargetId);
					HashMix(Hash, Intent.Damage);
					Emit(Result, bRecordLog, Tick, EEventType::GateHit, AttackerId, GateTargetId, Intent.Damage);
				}
				else
				{
					FUnit& Target = State.Units[Intent.TargetUnit];
					Target.Hp -= Intent.Damage;
					// §6 피해: 틱, 2, 공격자 번호, 표적 번호, 피해
					HashMix(Hash, Tick);
					HashMix(Hash, static_cast<int32>(EEventType::Damage));
					HashMix(Hash, AttackerId);
					HashMix(Hash, Target.Id);
					HashMix(Hash, Intent.Damage);
					Emit(Result, bRecordLog, Tick, EEventType::Damage, AttackerId, Target.Id, Intent.Damage);
				}
			}
		}

		// §4.1 ④ 진영 0 → 1, 열 0 → 5, 대열 앞 → 뒤 순서로 쓰러진 병사를 정리한다.
		void ResolveDeaths(FState& State, int32 Tick, uint32& Hash, FBattleResult& Result, bool bRecordLog)
		{
			for (int32 Side = 0; Side < NumSides; ++Side)
			{
				for (int32 Col = 0; Col < BoardCols; ++Col)
				{
					TArray<int32>& Line = State.Lines[Side][Col];
					int32 Rank = 0;
					while (Rank < Line.Num())
					{
						FUnit& Unit = State.Units[Line[Rank]];
						if (Unit.Hp > 0)
						{
							++Rank;
							continue;
						}
						if (Unit.Revive > 0)
						{
							// §2.8 해골 졸개의 첫 쓰러짐. 자리와 대기 시간은 그대로다(§9 3번).
							Unit.Hp = Unit.Revive;
							Unit.Revive = 0;
							// §6 부활: 해시에는 틱, 3, 유닛 번호. 로그 값은 부활한 뒤의 체력(§9 16번)
							HashMix(Hash, Tick);
							HashMix(Hash, static_cast<int32>(EEventType::Revive));
							HashMix(Hash, Unit.Id);
							Emit(Result, bRecordLog, Tick, EEventType::Revive, Unit.Id, Unit.Id, Unit.Hp);
							++Rank;
						}
						else
						{
							// §6 사망: 해시에는 틱, 4, 유닛 번호. 로그 값은 0
							HashMix(Hash, Tick);
							HashMix(Hash, static_cast<int32>(EEventType::Death));
							HashMix(Hash, Unit.Id);
							Emit(Result, bRecordLog, Tick, EEventType::Death, Unit.Id, Unit.Id, 0);
							// 대열에서 빼면 뒤의 병사가 한 칸씩 당겨진다. 대기 시간은 바뀌지 않는다(§9 7번).
							// RemoveAt은 남은 원소의 순서를 지킨다(Epic: Array Containers).
							Line.RemoveAt(Rank);
						}
					}
				}
			}
		}

		// §5.1 성문(결계)이 0 이하이거나 모든 열이 비면 그 진영이 진 것이다.
		bool SideLost(const FState& State, int32 Side)
		{
			if (State.Gate[Side] <= 0)
			{
				return true;
			}
			for (int32 Col = 0; Col < BoardCols; ++Col)
			{
				if (State.Lines[Side][Col].Num() > 0)
				{
					return false;
				}
			}
			return true;
		}
	}

	FBattleResult Simulate(const FBattleInput& Input, bool bRecordLog)
	{
		FBattleResult Result;
		if (!ValidateInput(Input))
		{
			Result.Outcome = EOutcome::Invalid; // §3.6 틱, 해시, 이벤트가 없다
			return Result;
		}

		FState State;
		BuildUnits(Input, State);
		for (int32 Side = 0; Side < NumSides; ++Side)
		{
			State.Gate[Side] = Input.Rules.GateHp;
		}

		uint32 Hash = FnvOffsetBasis;
		TArray<FIntent> Intents;
		Intents.Reserve(State.Units.Num() * 3);

		EOutcome Outcome = EOutcome::Timeout;
		int32 EndTick = Input.Rules.MaxTicks; // §5.1 600틱까지 끝나지 않으면 Timeout, 틱은 최대 틱으로 적는다

		for (int32 Tick = 1; Tick <= Input.Rules.MaxTicks; ++Tick)
		{
			Intents.Reset();
			CollectIntents(State, Input.Rules, Intents); // ①
			if (Intents.Num() == 0)
			{
				continue; // ② 의도가 없는 틱은 ③~⑤를 건너뛴다
			}
			ApplyIntents(State, Intents, Tick, Hash, Result, bRecordLog); // ③
			ResolveDeaths(State, Tick, Hash, Result, bRecordLog);         // ④

			const bool bLost0 = SideLost(State, 0); // ⑤
			const bool bLost1 = SideLost(State, 1);
			if (bLost0 || bLost1)
			{
				Outcome = (bLost0 && bLost1) ? EOutcome::Draw : (bLost1 ? EOutcome::A : EOutcome::B);
				EndTick = Tick;
				break;
			}
		}

		// §6 종료: 마지막 틱, 결과 코드. 로그와 이벤트 수에는 넣지 않는다(§9 13번).
		HashMix(Hash, EndTick);
		HashMix(Hash, static_cast<int32>(Outcome));

		Result.Outcome = Outcome;
		Result.Tick = EndTick;
		Result.Hash = Hash;
		for (int32 Side = 0; Side < NumSides; ++Side)
		{
			Result.Gate[Side] = State.Gate[Side];
			for (int32 Col = 0; Col < BoardCols; ++Col)
			{
				const TArray<int32>& Line = State.Lines[Side][Col];
				Result.Alive[Side] += Line.Num();
				for (int32 Rank = 0; Rank < Line.Num(); ++Rank)
				{
					Result.HpSum[Side] += State.Units[Line[Rank]].Hp;
				}
			}
		}
		return Result;
	}
}

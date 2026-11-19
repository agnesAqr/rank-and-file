#include "RankBattle.h"

namespace Rank
{
	// §5.2 플레이어 기준 판정
	FVerdict Judge(const FBattleResult& Result)
	{
		FVerdict Verdict;
		const int32 PlayerGate = Result.Gate[0];
		const int32 Barrier = Result.Gate[1];

		switch (Result.Outcome)
		{
		case EOutcome::A:
			Verdict.bWin = true;
			Verdict.Kind = Barrier <= 0 ? EVerdictKind::BarrierBreak : EVerdictKind::Annihilation;
			break;
		case EOutcome::Timeout:
			Verdict.bWin = true;
			Verdict.Kind = EVerdictKind::Dawn;
			break;
		case EOutcome::Draw:
			// 성문이 남아 있으면 승리(성문 사수), 아니면 패배(성문 붕괴)
			Verdict.bWin = PlayerGate > 0;
			Verdict.Kind = Verdict.bWin ? EVerdictKind::GateHeld : EVerdictKind::GateFall;
			break;
		case EOutcome::B:
			Verdict.bWin = false;
			Verdict.Kind = PlayerGate <= 0 ? EVerdictKind::GateFall : EVerdictKind::Wiped;
			break;
		case EOutcome::Invalid:
		default:
			Verdict.bWin = false;
			Verdict.Kind = EVerdictKind::None;
			break;
		}
		return Verdict;
	}
}

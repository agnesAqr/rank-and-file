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
			Verdict.bJudged = true;
			Verdict.bWin = true;
			Verdict.Kind = Barrier <= 0 ? EVerdictKind::BarrierBreak : EVerdictKind::Annihilation;
			break;
		case EOutcome::Timeout:
			Verdict.bJudged = true;
			Verdict.bWin = true;
			Verdict.Kind = EVerdictKind::Dawn;
			break;
		case EOutcome::Draw:
			// 성문이 남아 있으면 승리(성문 사수), 아니면 패배(성문 붕괴)
			Verdict.bJudged = true;
			Verdict.bWin = PlayerGate > 0;
			Verdict.Kind = Verdict.bWin ? EVerdictKind::GateHeld : EVerdictKind::GateFall;
			break;
		case EOutcome::B:
			Verdict.bJudged = true;
			Verdict.bWin = false;
			Verdict.Kind = PlayerGate <= 0 ? EVerdictKind::GateFall : EVerdictKind::Wiped;
			break;
		case EOutcome::Invalid:
		default:
			// §5.2 입력 오류(§3.6)는 전투를 계산하지 않았으므로 판정하지 않는다. 승리도 패배도 아니다.
			// bJudged가 false면 bWin을 쓰지 않는다. 게임은 패배로 세지 않는다(§5.2, 2026-11-20 결정).
			Verdict.bJudged = false;
			Verdict.bWin = false;
			Verdict.Kind = EVerdictKind::None;
			break;
		}
		return Verdict;
	}
}

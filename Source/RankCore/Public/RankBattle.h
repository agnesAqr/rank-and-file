#pragma once

#include "RankTypes.h"

namespace Rank
{
	// 병과 키로 병과 수치를 찾는다. 없으면 nullptr.
	RANKCORE_API const FUnitDef* FindDef(const FUnitDef* Defs, int32 NumDefs, uint8 Key);

	// §3.3 (Row, Col) 칸의 병사가 받는 링크 겹 수. 빈칸이면 모두 0이다.
	// 배치 화면의 링크 표시도 이 함수를 쓴다(Source/RankGame/AGENTS.md).
	RANKCORE_API FLinkStacks ComputeLinks(const FFormation& Formation, int32 Row, int32 Col, const FUnitDef* Defs, int32 NumDefs);

	// §3.4 (Row, Col) 칸의 병사의 최종 능력치. 빈칸이거나 모르는 병과 키면 false.
	RANKCORE_API bool ComputeFinalStats(const FFormation& Formation, int32 Row, int32 Col, const FUnitDef* Defs, int32 NumDefs, const FRuleParams& Rules, FUnitStats& OutStats);

	// 전투 한 판. 입력만으로 결과가 정해진다. 전역 상태를 읽거나 쓰지 않는다.
	// bRecordLog는 로그를 채울지만 정하고, 결과와 해시는 바꾸지 않는다.
	RANKCORE_API FBattleResult Simulate(const FBattleInput& Input, bool bRecordLog);

	// §5.2 코어 결과를 플레이어 기준 승패와 종류로 바꾼다.
	RANKCORE_API FVerdict Judge(const FBattleResult& Result);
}

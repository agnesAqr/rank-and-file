#pragma once

#include "RankFormationMask.h"
#include "RankRandom.h"

namespace Rank
{
	// 분대 하나. 모든 칸이 같은 병과다(Data/AGENTS.md, squads.csv의 class 열).
	struct FSquad
	{
		FShape Shape;
		uint8 Key = 0; // 병과 키(ASCII)
	};

	// 무작위 배치 방식의 횟수. 11/17 확정 · 본인(docs/design.md 9.3, Source/RankCore/AGENTS.md "공개 형태").
	inline constexpr int32 PlacementTriesPerSquad = 40;
	inline constexpr int32 PlacementMaxAttempts = 100;

	// 진형판 한 장에 놓을 수 있는 분대 수의 상한. 분대는 1칸 이상이라 칸 수를 넘지 못한다.
	inline constexpr int32 MaxSquadsPerFormation = BoardRows * BoardCols;

	// 무작위 배치(11/17 확정 방식).
	// 1. 분대 순서를 섞는다(피셔-예이츠 섞기).
	// 2. 그 순서대로 분대마다 회전(0~3번 돌림)과 위치(행 0~3, 열 0~5. 돌린 모양의 왼쪽 위)를 뽑아 놓아 본다. 40번까지 시도한다.
	// 3. 한 분대라도 40번 안에 못 놓으면 그 배치를 버리고 1번부터 다시 한다(순서도 다시 섞는다). 첫 시도를 포함해 최대 100번이다.
	// 한 번 놓아 볼 때 난수를 뽑는 순서는 회전, 행, 열이다.
	// 모든 분대를 놓으면 true다. 100번 안에 못 만들면 false이고 OutFormation은 빈 진형이다. 분대를 빼고 넘어가지 않는다.
	// 입력이 잘못되면(분대 모양이 1~4칸이 아님, 병과 키가 0, 분대 수가 음수이거나 24 초과, 칸 수 합이 24 초과) 난수를 쓰지 않고 바로 false다.
	// 분대가 0개면 빈 진형과 true를 돌려준다.
	RANKCORE_API bool MakeRandomFormation(const FSquad* Squads, int32 NumSquads, FRankRng& Rng, FFormation& OutFormation);
}

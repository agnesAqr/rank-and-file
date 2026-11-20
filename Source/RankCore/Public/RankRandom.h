#pragma once

#include "CoreMinimal.h"

namespace Rank
{
	// 정수 난수기. 무작위 배치(RankPlacement.h)와 BalanceSim이 함께 쓰는 하나뿐인 난수기다(Source/RankCore/AGENTS.md).
	// Simulate 안에서는 쓰지 않는다. 엔진 난수 함수를 부르지 않는다.
	//
	// 방식은 SplitMix64다. 공개 참조 구현 https://prng.di.unimi.it/splitmix64.c (Sebastiano Vigna, 2015, 퍼블릭 도메인)의
	// next()와 같은 상수와 시프트를 쓴다. 참조 구현의 주석: "The state can be seeded with any value."
	// 더하기, 곱하기, XOR, 시프트만 쓰고 부호 없는 64비트 정수의 넘침을 그대로 쓰므로,
	// 같은 시드면 어느 PC, 어느 빌드 구성에서도 같은 수열이 나온다. 상태는 이 구조체의 State 하나뿐이다.
	struct RANKCORE_API FRankRng
	{
		uint64 State = 0;

		FRankRng() = default;
		explicit FRankRng(uint64 InSeed)
			: State(InSeed)
		{
		}

		// 다음 64비트 값
		uint64 NextU64();

		// [0, Bound) 범위의 고른 정수. Bound가 0이면 0을 돌려주고 상태를 바꾸지 않는다.
		// 나머지 연산의 치우침을 없애려고, 2^32를 Bound로 나눈 나머지보다 작은 값은 버리고 다시 뽑는다(거부 샘플링).
		uint32 NextBelow(uint32 Bound);
	};
}

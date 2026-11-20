#include "RankRandom.h"

namespace Rank
{
	uint64 FRankRng::NextU64()
	{
		// SplitMix64 next(). 상수와 시프트는 https://prng.di.unimi.it/splitmix64.c 와 같다.
		State += 0x9E3779B97F4A7C15ull;
		uint64 Z = State;
		Z = (Z ^ (Z >> 30)) * 0xBF58476D1CE4E5B9ull;
		Z = (Z ^ (Z >> 27)) * 0x94D049BB133111EBull;
		return Z ^ (Z >> 31);
	}

	uint32 FRankRng::NextBelow(uint32 Bound)
	{
		if (Bound == 0)
		{
			return 0;
		}
		// (0 - Bound) % Bound = 2^32 mod Bound. Threshold 이상인 값은 2^32 - Threshold개이고 Bound로 나누어떨어진다.
		const uint32 Threshold = (0u - Bound) % Bound;
		for (;;)
		{
			const uint32 Value = static_cast<uint32>(NextU64() >> 32); // 위쪽 32비트를 쓴다
			if (Value >= Threshold)
			{
				return Value % Bound;
			}
		}
	}
}

#pragma once

#include "CoreMinimal.h"

namespace Rank
{
	// §6 FNV-1a 32비트. 정수 하나는 하위 바이트부터 4바이트를 차례로 XOR하고 곱한다.
	// uint32 곱셈이 32비트를 넘친 부분은 버려진다(부호 없는 정수의 산술).
	// 회귀 테스트의 로그 체크섬(Source/RankTests/AGENTS.md)도 같은 섞기를 쓴다. 그래서 Public에 둔다(11/20). 같은 계산을 복사하지 않는다(Source/AGENTS.md).
	inline constexpr uint32 FnvOffsetBasis = 0x811c9dc5u;
	inline constexpr uint32 FnvPrime = 0x01000193u;

	inline void HashMix(uint32& Hash, int32 Value)
	{
		const uint32 Bits = static_cast<uint32>(Value);
		for (int32 Byte = 0; Byte < 4; ++Byte)
		{
			Hash ^= (Bits >> (8 * Byte)) & 0xFFu;
			Hash *= FnvPrime;
		}
	}
}

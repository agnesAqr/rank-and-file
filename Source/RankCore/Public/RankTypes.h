#pragma once

#include "CoreMinimal.h"

// 전투 코어의 입력과 출력. 기준은 docs/rules.md(11/18 확정)다.
// 이 모듈에는 float, double, UObject, 엔진 난수가 없다(Source/RankCore/AGENTS.md 결정론 점검표).
namespace Rank
{
	// §1 진형판은 진영마다 6열 × 4행이다. §3.2 유닛 번호 식이 이 크기에 묶여 있어 데이터가 아니라 규칙이 고정한 값이다.
	inline constexpr int32 BoardCols = 6;
	inline constexpr int32 BoardRows = 4;
	inline constexpr int32 NumSides = 2;

	// §3.2 유닛 번호 = 진영 × 100 + 행 × 6 + 열
	inline constexpr int32 UnitIdSideFactor = 100;
	inline constexpr int32 UnitIdRowFactor = 6;

	// §6 성문 타격 이벤트의 표적 번호
	inline constexpr int32 GateTargetId = 999;

	// §2.5, §4.2 표적 종류
	enum class ETarget : uint8
	{
		Front,   // 맨 앞
		Back,    // 맨 뒤
		Support, // 지원
		Area,    // 범위
	};

	// §2.2, §7 링크로 주는 것
	enum class ELinkStat : uint8
	{
		None,
		Armor,
		Atk,
		Range,
		Hp,
		Interval,
		Count,
	};

	// §5.1 코어 결과. 값은 §6의 결과 코드다. Invalid는 §3.6 입력 오류다.
	enum class EOutcome : uint8
	{
		Invalid = 0,
		A = 1,
		B = 2,
		Draw = 3,
		Timeout = 4,
	};

	// §6 이벤트 종류 값. 종료는 로그에 넣지 않으므로 종류 값이 없다.
	enum class EEventType : uint8
	{
		GateHit = 1,
		Damage = 2,
		Revive = 3,
		Death = 4,
	};

	// §7 병과 수치. 값은 Data/defs/units.csv에서 온다.
	struct FUnitDef
	{
		uint8 Key = 0;
		int32 Hp = 0;
		int32 Atk = 0;
		int32 Armor = 0;
		int32 Interval = 0;
		int32 Range = 0;
		ETarget Target = ETarget::Front;
		bool bMagic = false;
		int32 Revive = 0;
		ELinkStat LinkStat = ELinkStat::None;
		int32 LinkAmount = 0;
	};

	// §7 규칙 매개변수. 값은 Data/defs/rules.csv에서 온다. 코드에 기본값을 두지 않는다.
	struct FRuleParams
	{
		int32 Cols = 0;
		int32 Rows = 0;
		int32 MaxTicks = 0;
		int32 GateHp = 0;
		int32 GateDamage = 0;
		int32 LinkStackMax = 0;
		int32 RangeMax = 0;
		int32 IntervalMin = 0;
	};

	// 한 진영의 진형. 칸마다 병과 키(ASCII), 빈칸은 0. 행 0이 최전열이다(§1, §9 6번).
	struct FFormation
	{
		uint8 Cells[BoardRows][BoardCols] = {};
	};

	struct FBattleInput
	{
		FFormation Side[NumSides];
		FRuleParams Rules;
		const FUnitDef* Defs = nullptr;
		int32 NumDefs = 0;
	};

	// §6 로그 한 줄: 틱, 종류, 행위자, 표적, 값
	struct FBattleEvent
	{
		int32 Tick = 0;
		uint8 Type = 0;
		int32 Actor = 0;
		int32 Target = 0;
		int32 Value = 0;
	};

	struct FBattleResult
	{
		EOutcome Outcome = EOutcome::Invalid;
		int32 Tick = 0;
		uint32 Hash = 0;
		int32 NumEvents = 0;          // §6 종류 1~4인 이벤트의 수
		int32 Gate[NumSides] = {};    // [0] 플레이어 성문, [1] 해골 결계. 음수일 수 있다(§4.1 ③)
		int32 Alive[NumSides] = {};
		int32 HpSum[NumSides] = {};
		TArray<FBattleEvent> Log;     // bRecordLog가 true일 때만 채운다
	};

	// §3.3 종류별로 받은 겹 수. 인덱스는 ELinkStat이다.
	struct FLinkStacks
	{
		int32 Count[static_cast<int32>(ELinkStat::Count)] = {};
	};

	// §3.4 링크를 더한 최종 능력치
	struct FUnitStats
	{
		int32 MaxHp = 0;
		int32 Atk = 0;
		int32 Armor = 0;
		int32 Interval = 0;
		int32 Range = 0;
	};

	// §5.2 플레이어 기준 판정
	enum class EVerdictKind : uint8
	{
		None,          // 입력 오류
		Annihilation,  // 섬멸
		BarrierBreak,  // 결계 파괴
		Dawn,          // 새벽
		GateHeld,      // 성문 사수
		GateFall,      // 성문 붕괴
		Wiped,         // 전멸
	};

	struct FVerdict
	{
		bool bWin = false;
		EVerdictKind Kind = EVerdictKind::None;
	};
}

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "RankContentRows.generated.h"

// 콘텐츠의 행 구조체. 콘텐츠를 읽는 경로는 하나다: CSV → 행 구조체 → 코어 구조체(Source/RankGame/AGENTS.md).
// 행 구조체와 변환 함수(RankContent.h)는 이 모듈에 하나만 둔다. 임포터, 시뮬레이터, 테스트가 같은 것을 쓴다.
// 필드는 CSV의 열을 그대로 옮긴다. 수치의 기본값은 0이고, 값은 언제나 CSV에서 온다.

// Data/defs/units.csv 한 줄. 열: key,name,hp,atk,armor,interval,range,target,magic,revive,linkStat,linkAmount(Data/AGENTS.md "defs")
USTRUCT()
struct RANKGAME_API FRankUnitRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY()
	FString Key;

	UPROPERTY()
	FString Name;

	UPROPERTY()
	int32 Hp = 0;

	UPROPERTY()
	int32 Atk = 0;

	UPROPERTY()
	int32 Armor = 0;

	UPROPERTY()
	int32 Interval = 0;

	UPROPERTY()
	int32 Range = 0;

	// Front, Back, Support, Area
	UPROPERTY()
	FString Target;

	UPROPERTY()
	bool bMagic = false;

	UPROPERTY()
	int32 Revive = 0;

	// None, Armor, Atk, Range, Hp, Interval
	UPROPERTY()
	FString LinkStat;

	UPROPERTY()
	int32 LinkAmount = 0;
};

// Data/defs/rules.csv의 데이터 한 줄. 열: maxTicks,gateHp,gateDamage,linkStackMax,rangeMax,intervalMin
USTRUCT()
struct RANKGAME_API FRankRulesRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY()
	int32 MaxTicks = 0;

	UPROPERTY()
	int32 GateHp = 0;

	UPROPERTY()
	int32 GateDamage = 0;

	UPROPERTY()
	int32 LinkStackMax = 0;

	UPROPERTY()
	int32 RangeMax = 0;

	UPROPERTY()
	int32 IntervalMin = 0;
};

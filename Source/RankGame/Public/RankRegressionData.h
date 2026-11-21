#pragma once

#include "CoreMinimal.h"
#include "RankContentRows.h"
#include "RankTypes.h"

// 회귀 테스트(T1)와 Shipping 결정성(T11)이 함께 쓰는 입력과 비교 항목.
// 입력 CSV를 코어 입력으로 바꾸는 함수는 이 모듈에 하나만 둔다. RankRegression 커맨들릿, T1, T11이 같은 함수를 쓴다(Source/RankGame/AGENTS.md).
// 전투 결과는 계산하지 않는다. 비교 항목은 Rank::Simulate의 결과에서 옮겨 적기만 한다.
namespace RankRegression
{
	// 저장소 루트 기준 상대 경로. RankRegression 커맨들릿, 자동화 테스트, T11이 같은 파일을 읽는다는 것을 이 한 곳에 둔다.
	// 에디터와 테스트는 FPaths::ProjectDir()에 붙여 쓴다.
	inline constexpr TCHAR InputsCsvFile[] = TEXT("Source/RankTests/Fixtures/regression_inputs.csv");
	inline constexpr TCHAR RecordJsonFile[] = TEXT("Source/RankTests/Fixtures/regression_battles_v1.json");
	inline constexpr TCHAR UnitsCsvFile[] = TEXT("Data/defs/units.csv");
	inline constexpr TCHAR RulesCsvFile[] = TEXT("Data/defs/rules.csv");

	// Source/RankTests/Fixtures/regression_inputs.csv 한 줄. 열: id,ally_row0..3,enemy_row0..3
	struct FBattleInputRow
	{
		FString Id;
		TArray<FString> AllyRows;
		TArray<FString> EnemyRows;
		Rank::FFormation Ally;
		Rank::FFormation Enemy;
	};

	RANKGAME_API bool ParseInputsCsv(const FString& Text, TArray<FBattleInputRow>& OutRows, FString& OutError);

	// 입력 한 줄을 코어 입력으로 바꾼다. 진영 0은 ally, 진영 1은 enemy다. 커맨들릿, T1, T11이 이 함수 하나를 쓴다.
	// 돌려준 입력의 Defs는 Defs 배열의 원소를 가리킨다. 입력을 쓰는 동안 Defs를 바꾸거나 없애지 않는다.
	RANKGAME_API Rank::FBattleInput MakeBattleInput(const FBattleInputRow& Row, const TArray<Rank::FUnitDef>& Defs, const Rank::FRuleParams& Rules);
	// 임시 배열을 넘기면 돌려준 입력의 Defs가 곧바로 사라진 메모리를 가리키므로 막는다.
	Rank::FBattleInput MakeBattleInput(const FBattleInputRow& Row, TArray<Rank::FUnitDef>&& Defs, const Rank::FRuleParams& Rules) = delete;

	// 비교 항목(Source/RankTests/AGENTS.md T1): 결과, 틱, 이벤트 수, 해시, 성문 값, 생존 수, 체력 합, 로그 체크섬
	struct FRecord
	{
		Rank::EOutcome Outcome = Rank::EOutcome::Invalid;
		int32 Tick = 0;
		int32 NumEvents = 0;
		uint32 Hash = 0;
		int32 Gate = 0;      // 진영 0 성문
		int32 Barrier = 0;   // 진영 1 결계
		int32 Alive[Rank::NumSides] = {};
		int32 HpSum[Rank::NumSides] = {};
		uint32 LogChecksum = 0;
	};

	// 로그 체크섬: 로그의 모든 줄의 다섯 칸(틱, 종류, 행위자, 표적, 값)을 순서대로 §6과 같은 FNV-1a 방식으로 섞은 값.
	// 규칙의 해시(§6)와는 따로, 테스트 쪽에서만 쓴다(Source/RankTests/AGENTS.md "회귀 테스트 입력").
	RANKGAME_API uint32 ComputeLogChecksum(const TArray<Rank::FBattleEvent>& Log);

	// Result는 로그를 켜고(bRecordLog = true) 돌린 결과여야 한다. 로그가 이벤트 수와 다르면 false.
	RANKGAME_API bool MakeRecord(const Rank::FBattleResult& Result, FRecord& OutRecord);

	// 두 기록의 항목을 순서대로 비교해 다른 것을 모두 적는다. 같으면 빈 배열이다.
	RANKGAME_API void DiffRecords(const FRecord& Expected, const FRecord& Actual, TArray<FString>& OutDiffs);

	RANKGAME_API const TCHAR* OutcomeName(Rank::EOutcome Outcome);
	RANKGAME_API bool ParseOutcome(const FString& Text, Rank::EOutcome& OutOutcome);

	// 32비트 값을 소문자 16진수 8자리로(예: 7385f6cb). 기록 파일의 해시와 로그 체크섬 표기다.
	RANKGAME_API FString HexU32(uint32 Value);
	RANKGAME_API bool ParseHexU32(const FString& Text, uint32& OutValue);

	// 기록 파일 Source/RankTests/Fixtures/regression_battles_v1.json의 내용(Source/RankEditor/AGENTS.md "RankRegression").
	// 쓰는 쪽은 RankRegression 커맨들릿, 읽는 쪽은 커맨들릿(덮어쓰기 전 비교)과 T1이다. 형식은 이 두 함수에만 있다.
	struct FRecordedBattle
	{
		FString Id;
		TArray<FString> AllyRows;
		TArray<FString> EnemyRows;
		FRecord Record;
		TArray<FString> Branches; // 실제로 지난 갈래 id(regression_branches.csv 순서)
	};

	struct FRecordFile
	{
		int32 Version = 1;
		FString Commit;                 // 기록한 커밋
		TArray<FRankUnitRow> Units;     // 기록에 쓴 병과 수치(units.csv와 같은 열 이름과 값)
		FRankRulesRow Rules;            // 기록에 쓴 규칙 매개변수(rules.csv와 같은 열 이름과 값)
		TArray<FRecordedBattle> Battles;
	};

	// 같은 내용이면 바이트까지 같은 글을 낸다. 키 순서가 고정이고, 날짜·시각·실행 환경을 넣지 않는다. 줄바꿈은 LF다(tools/AGENTS.md).
	RANKGAME_API FString WriteRecordJson(const FRecordFile& File);
	RANKGAME_API bool ReadRecordJson(const FString& Text, FRecordFile& OutFile, FString& OutError);

	// 병과 수치와 규칙 매개변수를 열마다 비교한다. 다른 곳을 "K.hp: <A라벨> 40, <B라벨> 41"처럼 모두 적는다.
	RANKGAME_API void DiffDefs(const TArray<FRankUnitRow>& UnitsA, const FRankRulesRow& RulesA, const TCHAR* LabelA,
		const TArray<FRankUnitRow>& UnitsB, const FRankRulesRow& RulesB, const TCHAR* LabelB, TArray<FString>& OutDiffs);
}

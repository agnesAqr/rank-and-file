#pragma once

#include "CoreMinimal.h"
#include "RankContentRows.h"
#include "RankTypes.h"

// CSV 글을 행 구조체로, 행 구조체를 코어 구조체(RankCore)로 바꾼다. 전투 결과는 계산하지 않는다.
// CSV 형식은 Data/AGENTS.md "CSV 공통 규칙"이다: UTF-8(BOM은 있어도 된다), 첫 줄은 열 이름, 쉼표 구분,
// 쉼표나 큰따옴표가 든 값은 큰따옴표로 감싸고 안의 큰따옴표는 두 번 쓴다, 줄바꿈은 CRLF와 LF, 참과 거짓은 TRUE와 FALSE.
// 실패하면 false를 돌려주고 OutError에 줄 번호와 열 이름을 적는다. 값을 짐작으로 채우지 않는다.
namespace RankContent
{
	// CSV 글을 줄로 나눈다. 줄 끝의 CR을 지우고 빈 줄은 건너뛴다. OutLineNumbers는 1부터 센 원래 줄 번호다.
	RANKGAME_API void SplitCsvLines(const FString& Text, TArray<FString>& OutLines, TArray<int32>& OutLineNumbers);

	// CSV 한 줄을 칸으로 나눈다. 큰따옴표가 닫히지 않으면 false.
	RANKGAME_API bool SplitCsvLine(const FString& Line, TArray<FString>& OutFields, FString& OutError);

	// Data/defs/units.csv. 열 이름과 순서가 정확히 같아야 한다. 병과 키가 겹치면(대소문자를 가린다) 실패.
	RANKGAME_API bool ParseUnitsCsv(const FString& Text, TArray<FRankUnitRow>& OutRows, FString& OutError);

	// Data/defs/rules.csv. 데이터는 한 줄이어야 한다. maxTicks나 gateHp가 0 이하면 실패.
	RANKGAME_API bool ParseRulesCsv(const FString& Text, FRankRulesRow& OutRow, FString& OutError);

	// 병과 키는 ASCII 한 글자다. target과 linkStat은 Data/AGENTS.md "defs"의 어휘만 받는다.
	RANKGAME_API bool ToCoreUnitDef(const FRankUnitRow& Row, Rank::FUnitDef& OutDef, FString& OutError);
	RANKGAME_API void ToCoreRules(const FRankRulesRow& Row, Rank::FRuleParams& OutRules);

	// 파일을 읽어 위의 함수로 바꾼다. 행 구조체와 코어 구조체를 함께 돌려준다.
	RANKGAME_API bool LoadDefsFiles(const FString& UnitsCsvPath, const FString& RulesCsvPath,
		TArray<FRankUnitRow>& OutUnitRows, FRankRulesRow& OutRulesRow,
		TArray<Rank::FUnitDef>& OutDefs, Rank::FRuleParams& OutRules, FString& OutError);

	// 진형 문자열 4행(행 0이 최전열, 행마다 6자, '.'은 빈칸)을 코어 진형으로 바꾼다.
	// 형식은 Data/AGENTS.md의 해골 진형 row0~row3과 같다. 병과 수치에 있는 키인지는 보지 않는다(Simulate가 §3.6으로 판정한다).
	RANKGAME_API bool ParseFormationRows(TConstArrayView<FString> Rows, Rank::FFormation& OutFormation, FString& OutError);
}

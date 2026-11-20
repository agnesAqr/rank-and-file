#pragma once

#include "CoreMinimal.h"
#include "RankContentRows.h"
#include "RankPlacement.h"
#include "RankTypes.h"

// T2~T5가 함께 쓰는 도우미. 전투 결과는 계산하지 않는다. 계산은 Rank::Simulate만 한다.
namespace RankTests
{
	// Data/defs의 병과 수치와 규칙 매개변수. RankGame의 변환 함수(RankContent.h)로 읽는다.
	struct FDefs
	{
		TArray<FRankUnitRow> UnitRows;
		FRankRulesRow RulesRow;
		TArray<Rank::FUnitDef> Units;
		Rank::FRuleParams Rules;
	};

	bool LoadProjectDefs(FDefs& OutDefs, FString& OutError);

	// 프로젝트 폴더 기준 경로
	FString ProjectFile(const TCHAR* RelativePath);

	// 무작위 분대 모양: 1~4칸, 변끼리 이어진 모양. 시드를 고정한 정수 난수기만 쓴다(Source/RankTests/AGENTS.md "무작위 입력").
	Rank::FShape MakeRandomShape(Rank::FRankRng& Rng);

	// 무작위 덱: 분대 1~6개, 칸 수 합 16 이하, 분대마다 병과 키는 Defs 9종 가운데 하나.
	// 칸 수 합을 16 이하로 두는 것은 무작위 배치가 100번 안에 거의 언제나 성공하게 하려는 것이다. 실패하면 테스트가 실패로 적는다.
	void MakeRandomDeck(Rank::FRankRng& Rng, const TArray<Rank::FUnitDef>& Defs, TArray<Rank::FSquad>& OutDeck);

	// 시드 하나로 진형 하나: 덱을 만들고 Rank::MakeRandomFormation으로 놓는다.
	bool MakeRandomFormationFromRng(Rank::FRankRng& Rng, const TArray<Rank::FUnitDef>& Defs, Rank::FFormation& OutFormation);

	// 진형을 "행0/행1/행2/행3" 문자열로(실패 메시지용)
	FString FormationToString(const Rank::FFormation& Formation);

	bool SameFormation(const Rank::FFormation& A, const Rank::FFormation& B);

	// 로그를 뺀 결과의 모든 항목을 비교한다. 다르면 첫 번째로 다른 항목을 OutDiff에 적는다.
	bool SameResult(const Rank::FBattleResult& A, const Rank::FBattleResult& B, FString& OutDiff);

	const TCHAR* OutcomeName(Rank::EOutcome Outcome);
}

#pragma once

#include "Commandlets/Commandlet.h"
#include "RankRegressionCommandlet.generated.h"

// -run=RankRegression (Source/RankEditor/AGENTS.md "RankRegression", tools/AGENTS.md "record_regression.bat")
//
// 회귀 입력 20건(Source/RankTests/Fixtures/regression_inputs.csv)을 Rank::Simulate로 돌린다.
// - 갈래 집계: 대전마다 regression_branches.csv의 갈래를 지났는지 detect 열의 방법으로 확인해 표로 낸다.
//   표는 로그와 Saved/RankRegression/branch_tally.csv에 쓴다.
// - 지금 기록 파일이 있으면 대전마다 비교표를 낸다.
// - -write가 있을 때만 기록 파일을 쓴다. 아래 가운데 하나라도 있으면 쓰지 않는다:
//   T1이 맡은 갈래 가운데 0건이 있음, 확인 방법이 구현되지 않은 T1 갈래가 있음, -commit이 없음,
//   기록 파일이 이미 있는데 -overwrite가 없음.
// 인자: -write, -overwrite, -commit=<커밋>, -out=<경로>(기본은 위 기록 파일),
//       -inputs=<경로>(입력 수정안 시험용. 갈래 집계만 하고 -write와 함께 쓸 수 없다)
UCLASS()
class URankRegressionCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	URankRegressionCommandlet();

	virtual int32 Main(const FString& Params) override;
};

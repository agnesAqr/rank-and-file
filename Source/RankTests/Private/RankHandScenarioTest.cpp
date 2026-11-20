#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "RankBattle.h"
#include "RankContent.h"
#include "RankRegressionData.h"
#include "RankTestUtils.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// T5 손계산 시나리오(Source/RankTests/AGENTS.md "손계산 시나리오 (T5)").
// 기대값은 AI가 docs/rules.md만 보고 계산하고, 교차 검증에서 일치했거나 사람이 판정한 값이다(docs/crosscheck/README.md).
// 이 테스트는 기대값을 만들거나 고치지 않는다.
// - 파일의 병과 수치와 규칙 매개변수가 지금 Data/defs와 다르면 "수치가 바뀌어 다시 계산해야 한다"로 실패한다.
// - 확인 상태가 "교차 확인(모델, 날짜)", "본인 판정(날짜)", "본인 확인(날짜)"가 아닌 시나리오는 기대값과 비교하지 않고 "확인 전"으로 실패한다.
//   교차 검증에서 일치하거나 사람이 판정하기 전의 AI 계산값은 기대값으로 쓰지 않는다(루트 AGENTS.md 5절). 건너뛰지도 않는다.
// - 확인된 시나리오는 expected에 적힌 항목만 비교한다: outcome, verdict, endTick, gate, barrier, units(id, alive, hp).

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags RankTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	const TCHAR* const HandFile = TEXT("Source/RankTests/Fixtures/hand_v1.json");

	// JSON의 수는 실수로 읽힌다. 정수가 아니면 형식 오류로 본다(엔진의 정수 변환은 반올림한다: Runtime/Json/Private/Dom/JsonValue.cpp TryConvertNumber).
	bool GetInt(const FJsonObject& Object, const TCHAR* Field, int32& OutValue)
	{
		double Number = 0.0;
		if (!Object.TryGetNumberField(Field, Number))
		{
			return false;
		}
		if (Number < static_cast<double>(MIN_int32) || Number > static_cast<double>(MAX_int32))
		{
			return false;
		}
		const int32 Value = static_cast<int32>(Number);
		if (static_cast<double>(Value) != Number)
		{
			return false;
		}
		OutValue = Value;
		return true;
	}

	const TCHAR* VerdictName(Rank::EVerdictKind Kind)
	{
		switch (Kind)
		{
		case Rank::EVerdictKind::Annihilation: return TEXT("섬멸");
		case Rank::EVerdictKind::BarrierBreak: return TEXT("결계 파괴");
		case Rank::EVerdictKind::Dawn: return TEXT("새벽");
		case Rank::EVerdictKind::GateHeld: return TEXT("성문 사수");
		case Rank::EVerdictKind::GateFall: return TEXT("성문 붕괴");
		case Rank::EVerdictKind::Wiped: return TEXT("전멸");
		case Rank::EVerdictKind::None:
		default: return TEXT("없음");
		}
	}

	// 상태가 "머리말(내용)" 꼴인지 본다. 내용은 비어 있을 수 없다.
	bool HasStatusForm(const FString& Status, const FString& Prefix)
	{
		return Status.StartsWith(Prefix, ESearchCase::CaseSensitive)
			&& Status.EndsWith(TEXT(")"), ESearchCase::CaseSensitive)
			&& Status.Len() > Prefix.Len() + 1;
	}

	// 확인된 상태는 셋이다(Source/RankTests/AGENTS.md "손계산 시나리오 (T5)").
	// 교차 확인(모델, 날짜): 교차 검증에서 비교한 항목이 전부 일치했다. tools/hand_crosscheck.js apply만 적는다.
	// 본인 판정(날짜): 달랐던 시나리오를 사람이 기존 기대값이 맞다고 판정했다. 본인 확인(날짜): 사람이 계산 과정을 직접 확인했다.
	bool IsConfirmed(const FString& Status)
	{
		const FString CrossChecked = TEXT("교차 확인(");
		const FString Ruled = TEXT("본인 판정(");
		const FString Confirmed = TEXT("본인 확인(");
		return HasStatusForm(Status, CrossChecked) || HasStatusForm(Status, Ruled) || HasStatusForm(Status, Confirmed);
	}

	// 손계산 파일의 병과 하나를 행 구조체로 옮긴다. 열 이름은 units.csv와 같다.
	bool ReadUnitRow(const FJsonObject& Object, FRankUnitRow& OutRow, FString& OutError)
	{
		const bool bOk = Object.TryGetStringField(TEXT("key"), OutRow.Key)
			&& Object.TryGetStringField(TEXT("name"), OutRow.Name)
			&& GetInt(Object, TEXT("hp"), OutRow.Hp)
			&& GetInt(Object, TEXT("atk"), OutRow.Atk)
			&& GetInt(Object, TEXT("armor"), OutRow.Armor)
			&& GetInt(Object, TEXT("interval"), OutRow.Interval)
			&& GetInt(Object, TEXT("range"), OutRow.Range)
			&& Object.TryGetStringField(TEXT("target"), OutRow.Target)
			&& Object.TryGetBoolField(TEXT("magic"), OutRow.bMagic)
			&& GetInt(Object, TEXT("revive"), OutRow.Revive)
			&& Object.TryGetStringField(TEXT("linkStat"), OutRow.LinkStat)
			&& GetInt(Object, TEXT("linkAmount"), OutRow.LinkAmount);
		if (!bOk)
		{
			OutError = TEXT("defs.units의 항목에 units.csv의 열이 빠졌거나 형식이 다르다");
		}
		return bOk;
	}

	bool ReadRows(const FJsonObject& Scenario, const TCHAR* Field, TArray<FString>& OutRows)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!Scenario.TryGetArrayField(Field, Values))
		{
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			FString Row;
			if (!Value.IsValid() || !Value->TryGetString(Row))
			{
				return false;
			}
			OutRows.Add(Row);
		}
		return true;
	}

	// 확인된 시나리오의 expected를 결과와 비교한다. 다른 곳과 형식 오류를 OutDiffs에 적는다.
	void CompareExpected(const FJsonObject& Expected, const Rank::FBattleResult& Result, TArray<FString>& OutDiffs)
	{
		FString Text;
		if (Expected.HasField(TEXT("outcome")))
		{
			if (!Expected.TryGetStringField(TEXT("outcome"), Text))
			{
				OutDiffs.Add(TEXT("expected.outcome: 문자열이 아니다"));
			}
			else if (!Text.Equals(RankTests::OutcomeName(Result.Outcome), ESearchCase::CaseSensitive))
			{
				OutDiffs.Add(FString::Printf(TEXT("outcome: 기대 %s, 실제 %s"), *Text, RankTests::OutcomeName(Result.Outcome)));
			}
		}
		if (Expected.HasField(TEXT("verdict")))
		{
			const Rank::FVerdict Verdict = Rank::Judge(Result);
			if (!Expected.TryGetStringField(TEXT("verdict"), Text))
			{
				OutDiffs.Add(TEXT("expected.verdict: 문자열이 아니다"));
			}
			else if (!Text.Equals(VerdictName(Verdict.Kind), ESearchCase::CaseSensitive))
			{
				OutDiffs.Add(FString::Printf(TEXT("verdict: 기대 %s, 실제 %s"), *Text, VerdictName(Verdict.Kind)));
			}
		}

		auto CompareInt = [&](const TCHAR* Field, int32 Actual)
		{
			if (!Expected.HasField(Field))
			{
				return;
			}
			int32 Value = 0;
			if (!GetInt(Expected, Field, Value))
			{
				OutDiffs.Add(FString::Printf(TEXT("expected.%s: 정수가 아니다"), Field));
			}
			else if (Value != Actual)
			{
				OutDiffs.Add(FString::Printf(TEXT("%s: 기대 %d, 실제 %d"), Field, Value, Actual));
			}
		};
		CompareInt(TEXT("endTick"), Result.Tick);
		CompareInt(TEXT("gate"), Result.Gate[0]);
		CompareInt(TEXT("barrier"), Result.Gate[1]);

		if (Expected.HasField(TEXT("units")))
		{
			const TArray<TSharedPtr<FJsonValue>>* Units = nullptr;
			if (!Expected.TryGetArrayField(TEXT("units"), Units))
			{
				OutDiffs.Add(TEXT("expected.units: 배열이 아니다"));
				return;
			}
			for (const TSharedPtr<FJsonValue>& Value : *Units)
			{
				const TSharedPtr<FJsonObject>* Unit = nullptr;
				int32 Id = 0;
				if (!Value.IsValid() || !Value->TryGetObject(Unit) || !GetInt(**Unit, TEXT("id"), Id))
				{
					OutDiffs.Add(TEXT("expected.units: id가 없는 항목이 있다"));
					continue;
				}
				const Rank::FUnitResult* Actual = nullptr;
				for (int32 Index = 0; Index < Result.NumUnits; ++Index)
				{
					if (Result.Units[Index].Id == Id)
					{
						Actual = &Result.Units[Index];
						break;
					}
				}
				if (Actual == nullptr)
				{
					OutDiffs.Add(FString::Printf(TEXT("유닛 %d: 결과에 없다"), Id));
					continue;
				}
				bool bAlive = false;
				if ((*Unit)->HasField(TEXT("alive")))
				{
					if (!(*Unit)->TryGetBoolField(TEXT("alive"), bAlive))
					{
						OutDiffs.Add(FString::Printf(TEXT("유닛 %d alive: 참거짓이 아니다"), Id));
					}
					else if (bAlive != Actual->bAlive)
					{
						OutDiffs.Add(FString::Printf(TEXT("유닛 %d alive: 기대 %s, 실제 %s"), Id, bAlive ? TEXT("true") : TEXT("false"), Actual->bAlive ? TEXT("true") : TEXT("false")));
					}
				}
				if ((*Unit)->HasField(TEXT("hp")))
				{
					int32 Hp = 0;
					if (!GetInt(**Unit, TEXT("hp"), Hp))
					{
						OutDiffs.Add(FString::Printf(TEXT("유닛 %d hp: 정수가 아니다"), Id));
					}
					else if (Hp != Actual->Hp)
					{
						OutDiffs.Add(FString::Printf(TEXT("유닛 %d hp: 기대 %d, 실제 %d"), Id, Hp, Actual->Hp));
					}
				}
			}
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRankT5HandScenarioTest, "RankAndFile.Core.T5_HandScenarios", RankTestFlags)

bool FRankT5HandScenarioTest::RunTest(const FString& Parameters)
{
	// 1. 파일 읽기
	const FString Path = RankTests::ProjectFile(HandFile);
	FString JsonText;
	if (!FFileHelper::LoadFileToString(JsonText, *Path))
	{
		AddError(FString::Printf(TEXT("T5: 손계산 파일이 없다(%s). 확인 전"), HandFile));
		return false;
	}
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(JsonText), Root) || !Root.IsValid())
	{
		AddError(FString::Printf(TEXT("T5: %s가 JSON으로 읽히지 않는다"), HandFile));
		return false;
	}

	// 2. 수치 비교
	RankTests::FDefs Defs;
	FString Error;
	if (!RankTests::LoadProjectDefs(Defs, Error))
	{
		AddError(FString::Printf(TEXT("T5: Data/defs를 읽지 못했다: %s"), *Error));
		return false;
	}
	const TSharedPtr<FJsonObject>* DefsObject = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* HandUnitValues = nullptr;
	const TSharedPtr<FJsonObject>* HandRulesObject = nullptr;
	if (!Root->TryGetObjectField(TEXT("defs"), DefsObject)
		|| !(*DefsObject)->TryGetArrayField(TEXT("units"), HandUnitValues)
		|| !(*DefsObject)->TryGetObjectField(TEXT("rules"), HandRulesObject))
	{
		AddError(TEXT("T5: 손계산 파일에 defs.units나 defs.rules가 없다"));
		return false;
	}
	TArray<FRankUnitRow> HandUnits;
	for (const TSharedPtr<FJsonValue>& Value : *HandUnitValues)
	{
		const TSharedPtr<FJsonObject>* UnitObject = nullptr;
		FRankUnitRow Row;
		if (!Value.IsValid() || !Value->TryGetObject(UnitObject) || !ReadUnitRow(**UnitObject, Row, Error))
		{
			AddError(FString::Printf(TEXT("T5: %s"), Error.IsEmpty() ? TEXT("defs.units 형식 오류") : *Error));
			return false;
		}
		HandUnits.Add(Row);
	}
	FRankRulesRow HandRules;
	const FJsonObject& Rules = **HandRulesObject;
	if (!GetInt(Rules, TEXT("maxTicks"), HandRules.MaxTicks) || !GetInt(Rules, TEXT("gateHp"), HandRules.GateHp)
		|| !GetInt(Rules, TEXT("gateDamage"), HandRules.GateDamage) || !GetInt(Rules, TEXT("linkStackMax"), HandRules.LinkStackMax)
		|| !GetInt(Rules, TEXT("rangeMax"), HandRules.RangeMax) || !GetInt(Rules, TEXT("intervalMin"), HandRules.IntervalMin))
	{
		AddError(TEXT("T5: defs.rules에 rules.csv의 열이 빠졌거나 정수가 아니다"));
		return false;
	}
	TArray<FString> DefDiffs;
	RankRegression::DiffDefs(HandUnits, HandRules, TEXT("손계산"), Defs.UnitRows, Defs.RulesRow, TEXT("Data/defs"), DefDiffs);
	if (DefDiffs.Num() > 0)
	{
		for (const FString& Diff : DefDiffs)
		{
			AddError(FString::Printf(TEXT("T5: %s"), *Diff));
		}
		AddError(TEXT("T5: 수치가 바뀌어 다시 계산해야 한다"));
		return false;
	}

	// 3. 시나리오
	const TArray<TSharedPtr<FJsonValue>>* Scenarios = nullptr;
	if (!Root->TryGetArrayField(TEXT("scenarios"), Scenarios) || Scenarios->Num() == 0)
	{
		AddError(TEXT("T5: 시나리오가 없다"));
		return false;
	}

	int32 Passed = 0;
	int32 Unconfirmed = 0;
	int32 Mismatched = 0;
	int32 Malformed = 0;
	for (int32 Index = 0; Index < Scenarios->Num(); ++Index)
	{
		const TSharedPtr<FJsonObject>* ScenarioPtr = nullptr;
		if (!(*Scenarios)[Index].IsValid() || !(*Scenarios)[Index]->TryGetObject(ScenarioPtr))
		{
			++Malformed;
			AddError(FString::Printf(TEXT("T5 #%d: 시나리오가 객체가 아니다"), Index));
			continue;
		}
		const FJsonObject& Scenario = **ScenarioPtr;
		FString Id = FString::Printf(TEXT("#%d"), Index);
		Scenario.TryGetStringField(TEXT("id"), Id);

		// 입력을 만들고 전투를 돌린다. 확인 전이어도 입력이 읽히는지, 돌다가 멈추지 않는지는 본다. 결과는 비교하지 않는다.
		TArray<FString> AllyRows;
		TArray<FString> EnemyRows;
		Rank::FBattleInput Input;
		Input.Rules = Defs.Rules;
		Input.Defs = Defs.Units.GetData();
		Input.NumDefs = Defs.Units.Num();
		if (!ReadRows(Scenario, TEXT("ally"), AllyRows) || !ReadRows(Scenario, TEXT("enemy"), EnemyRows)
			|| !RankContent::ParseFormationRows(AllyRows, Input.Side[0], Error)
			|| !RankContent::ParseFormationRows(EnemyRows, Input.Side[1], Error))
		{
			++Malformed;
			AddError(FString::Printf(TEXT("T5 %s: 진형을 읽지 못했다(%s)"), *Id, *Error));
			continue;
		}
		const Rank::FBattleResult Result = Rank::Simulate(Input, true);

		FString Status;
		Scenario.TryGetStringField(TEXT("status"), Status);
		if (!IsConfirmed(Status))
		{
			++Unconfirmed;
			AddError(FString::Printf(TEXT("T5 %s: 확인 전(상태 \"%s\")"), *Id, *Status));
			continue;
		}

		const TSharedPtr<FJsonObject>* Expected = nullptr;
		if (!Scenario.TryGetObjectField(TEXT("expected"), Expected) || (*Expected)->Values.Num() == 0)
		{
			++Unconfirmed;
			AddError(FString::Printf(TEXT("T5 %s: 기대값이 없다. 확인 전"), *Id));
			continue;
		}

		TArray<FString> Diffs;
		CompareExpected(**Expected, Result, Diffs);
		if (Diffs.Num() == 0)
		{
			++Passed;
			continue;
		}

		// 어긋나면 다른 곳을 모두 적고, 계산 과정과 틱 단위로 대조할 수 있도록 이벤트 로그를 한 줄에 하나씩 남긴다(Source/RankCore/AGENTS.md).
		++Mismatched;
		for (const FString& Diff : Diffs)
		{
			AddError(FString::Printf(TEXT("T5 %s: %s"), *Id, *Diff));
		}
		AddInfo(FString::Printf(TEXT("T5 %s 이벤트 로그(틱, 종류, 행위자, 표적, 값) %d줄"), *Id, Result.Log.Num()));
		for (const Rank::FBattleEvent& Event : Result.Log)
		{
			AddInfo(FString::Printf(TEXT("T5 %s | %d, %d, %d, %d, %d"), *Id, Event.Tick, Event.Type, Event.Actor, Event.Target, Event.Value));
		}
	}

	const int32 Total = Scenarios->Num();
	AddInfo(FString::Printf(TEXT("T5 손계산 시나리오 %d/%d (확인 전 %d, 어긋남 %d, 형식 오류 %d)"),
		Passed, Total, Unconfirmed, Mismatched, Malformed));
	if (Passed != Total)
	{
		AddError(FString::Printf(TEXT("T5 손계산 시나리오 %d/%d"), Passed, Total));
	}
	return Passed == Total;
}

#endif // WITH_DEV_AUTOMATION_TESTS

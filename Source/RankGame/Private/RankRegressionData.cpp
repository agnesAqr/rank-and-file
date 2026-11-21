#include "RankRegressionData.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "RankContent.h"
#include "RankHash.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace RankRegression
{
	namespace
	{
		const TCHAR* const InputColumns[] = {
			TEXT("id"),
			TEXT("ally_row0"), TEXT("ally_row1"), TEXT("ally_row2"), TEXT("ally_row3"),
			TEXT("enemy_row0"), TEXT("enemy_row1"), TEXT("enemy_row2"), TEXT("enemy_row3") };
	}

	bool ParseInputsCsv(const FString& Text, TArray<FBattleInputRow>& OutRows, FString& OutError)
	{
		OutRows.Reset();
		constexpr int32 NumColumns = UE_ARRAY_COUNT(InputColumns);
		static_assert(NumColumns == 1 + 2 * Rank::BoardRows, "regression_inputs.csv has 4 rows per side");

		TArray<FString> Lines;
		TArray<int32> LineNumbers;
		RankContent::SplitCsvLines(Text, Lines, LineNumbers);
		if (Lines.Num() == 0)
		{
			OutError = TEXT("regression_inputs.csv: 비어 있다");
			return false;
		}

		TArray<FString> Fields;
		if (!RankContent::SplitCsvLine(Lines[0], Fields, OutError))
		{
			OutError = FString::Printf(TEXT("regression_inputs.csv 1줄: %s"), *OutError);
			return false;
		}
		bool bHeader = Fields.Num() == NumColumns;
		for (int32 Index = 0; bHeader && Index < NumColumns; ++Index)
		{
			bHeader = Fields[Index].Equals(InputColumns[Index], ESearchCase::CaseSensitive);
		}
		if (!bHeader)
		{
			OutError = FString::Printf(TEXT("regression_inputs.csv 1줄: 열 이름이 다르다(\"%s\")"), *Lines[0]);
			return false;
		}

		for (int32 LineIndex = 1; LineIndex < Lines.Num(); ++LineIndex)
		{
			const int32 LineNumber = LineNumbers[LineIndex];
			if (!RankContent::SplitCsvLine(Lines[LineIndex], Fields, OutError) || Fields.Num() != NumColumns)
			{
				OutError = FString::Printf(TEXT("regression_inputs.csv %d줄: 칸이 %d개여야 한다"), LineNumber, NumColumns);
				return false;
			}
			FBattleInputRow Row;
			Row.Id = Fields[0];
			for (int32 BoardRow = 0; BoardRow < Rank::BoardRows; ++BoardRow)
			{
				Row.AllyRows.Add(Fields[1 + BoardRow]);
				Row.EnemyRows.Add(Fields[1 + Rank::BoardRows + BoardRow]);
			}
			FString FormationError;
			if (!RankContent::ParseFormationRows(Row.AllyRows, Row.Ally, FormationError)
				|| !RankContent::ParseFormationRows(Row.EnemyRows, Row.Enemy, FormationError))
			{
				OutError = FString::Printf(TEXT("regression_inputs.csv %d줄(%s): %s"), LineNumber, *Row.Id, *FormationError);
				return false;
			}
			for (const FBattleInputRow& Existing : OutRows)
			{
				if (Existing.Id.Equals(Row.Id, ESearchCase::CaseSensitive))
				{
					OutError = FString::Printf(TEXT("regression_inputs.csv %d줄: id가 겹친다(%s)"), LineNumber, *Row.Id);
					return false;
				}
			}
			OutRows.Add(MoveTemp(Row));
		}
		return true;
	}

	Rank::FBattleInput MakeBattleInput(const FBattleInputRow& Row, const TArray<Rank::FUnitDef>& Defs, const Rank::FRuleParams& Rules)
	{
		Rank::FBattleInput Input;
		Input.Side[0] = Row.Ally;
		Input.Side[1] = Row.Enemy;
		Input.Rules = Rules;
		Input.Defs = Defs.GetData();
		Input.NumDefs = Defs.Num();
		return Input;
	}

	uint32 ComputeLogChecksum(const TArray<Rank::FBattleEvent>& Log)
	{
		uint32 Checksum = Rank::FnvOffsetBasis;
		for (const Rank::FBattleEvent& Event : Log)
		{
			Rank::HashMix(Checksum, Event.Tick);
			Rank::HashMix(Checksum, static_cast<int32>(Event.Type));
			Rank::HashMix(Checksum, Event.Actor);
			Rank::HashMix(Checksum, Event.Target);
			Rank::HashMix(Checksum, Event.Value);
		}
		return Checksum;
	}

	bool MakeRecord(const Rank::FBattleResult& Result, FRecord& OutRecord)
	{
		if (Result.Log.Num() != Result.NumEvents)
		{
			return false;
		}
		FRecord Record;
		Record.Outcome = Result.Outcome;
		Record.Tick = Result.Tick;
		Record.NumEvents = Result.NumEvents;
		Record.Hash = Result.Hash;
		Record.Gate = Result.Gate[0];
		Record.Barrier = Result.Gate[1];
		for (int32 Side = 0; Side < Rank::NumSides; ++Side)
		{
			Record.Alive[Side] = Result.Alive[Side];
			Record.HpSum[Side] = Result.HpSum[Side];
		}
		Record.LogChecksum = ComputeLogChecksum(Result.Log);
		OutRecord = Record;
		return true;
	}

	void DiffRecords(const FRecord& Expected, const FRecord& Actual, TArray<FString>& OutDiffs)
	{
		OutDiffs.Reset();
		if (Expected.Outcome != Actual.Outcome)
		{
			OutDiffs.Add(FString::Printf(TEXT("결과: 기록 %s, 지금 %s"), OutcomeName(Expected.Outcome), OutcomeName(Actual.Outcome)));
		}
		auto DiffInt = [&OutDiffs](const TCHAR* Name, int32 A, int32 B)
		{
			if (A != B)
			{
				OutDiffs.Add(FString::Printf(TEXT("%s: 기록 %d, 지금 %d"), Name, A, B));
			}
		};
		DiffInt(TEXT("틱"), Expected.Tick, Actual.Tick);
		DiffInt(TEXT("이벤트 수"), Expected.NumEvents, Actual.NumEvents);
		if (Expected.Hash != Actual.Hash)
		{
			OutDiffs.Add(FString::Printf(TEXT("해시: 기록 %s, 지금 %s"), *HexU32(Expected.Hash), *HexU32(Actual.Hash)));
		}
		DiffInt(TEXT("성문"), Expected.Gate, Actual.Gate);
		DiffInt(TEXT("결계"), Expected.Barrier, Actual.Barrier);
		DiffInt(TEXT("생존 수(진영 0)"), Expected.Alive[0], Actual.Alive[0]);
		DiffInt(TEXT("생존 수(진영 1)"), Expected.Alive[1], Actual.Alive[1]);
		DiffInt(TEXT("체력 합(진영 0)"), Expected.HpSum[0], Actual.HpSum[0]);
		DiffInt(TEXT("체력 합(진영 1)"), Expected.HpSum[1], Actual.HpSum[1]);
		if (Expected.LogChecksum != Actual.LogChecksum)
		{
			OutDiffs.Add(FString::Printf(TEXT("로그 체크섬: 기록 %s, 지금 %s"), *HexU32(Expected.LogChecksum), *HexU32(Actual.LogChecksum)));
		}
	}

	const TCHAR* OutcomeName(Rank::EOutcome Outcome)
	{
		switch (Outcome)
		{
		case Rank::EOutcome::A: return TEXT("A");
		case Rank::EOutcome::B: return TEXT("B");
		case Rank::EOutcome::Draw: return TEXT("Draw");
		case Rank::EOutcome::Timeout: return TEXT("Timeout");
		case Rank::EOutcome::Invalid:
		default: return TEXT("Invalid");
		}
	}

	bool ParseOutcome(const FString& Text, Rank::EOutcome& OutOutcome)
	{
		const Rank::EOutcome All[] = { Rank::EOutcome::Invalid, Rank::EOutcome::A, Rank::EOutcome::B, Rank::EOutcome::Draw, Rank::EOutcome::Timeout };
		for (const Rank::EOutcome Outcome : All)
		{
			if (Text.Equals(OutcomeName(Outcome), ESearchCase::CaseSensitive))
			{
				OutOutcome = Outcome;
				return true;
			}
		}
		return false;
	}

	FString HexU32(uint32 Value)
	{
		return FString::Printf(TEXT("%08x"), Value);
	}

	bool ParseHexU32(const FString& Text, uint32& OutValue)
	{
		if (Text.Len() != 8)
		{
			return false;
		}
		uint32 Value = 0;
		for (int32 Index = 0; Index < Text.Len(); ++Index)
		{
			const TCHAR Char = Text[Index];
			uint32 Digit = 0;
			if (Char >= TEXT('0') && Char <= TEXT('9')) { Digit = Char - TEXT('0'); }
			else if (Char >= TEXT('a') && Char <= TEXT('f')) { Digit = 10 + (Char - TEXT('a')); }
			else { return false; }
			Value = (Value << 4) | Digit;
		}
		OutValue = Value;
		return true;
	}

	namespace
	{
		FString JsonString(const FString& Text)
		{
			FString Out = TEXT("\"");
			for (const TCHAR Char : Text)
			{
				if (Char == TEXT('"')) { Out += TEXT("\\\""); }
				else if (Char == TEXT('\\')) { Out += TEXT("\\\\"); }
				else if (Char < 0x20) { Out += FString::Printf(TEXT("\\u%04x"), static_cast<uint32>(Char)); }
				else { Out.AppendChar(Char); }
			}
			Out += TEXT("\"");
			return Out;
		}

		FString JsonStringArray(const TArray<FString>& Items)
		{
			FString Out = TEXT("[");
			for (int32 Index = 0; Index < Items.Num(); ++Index)
			{
				Out += (Index > 0 ? TEXT(", ") : TEXT("")) + JsonString(Items[Index]);
			}
			return Out + TEXT("]");
		}

		bool GetInt(const FJsonObject& Object, const TCHAR* Field, int32& OutValue)
		{
			double Number = 0.0;
			if (!Object.TryGetNumberField(Field, Number) || Number < MIN_int32 || Number > MAX_int32)
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

		bool GetStringArray(const FJsonObject& Object, const TCHAR* Field, TArray<FString>& OutItems)
		{
			const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
			if (!Object.TryGetArrayField(Field, Values))
			{
				return false;
			}
			for (const TSharedPtr<FJsonValue>& Value : *Values)
			{
				FString Item;
				if (!Value.IsValid() || !Value->TryGetString(Item))
				{
					return false;
				}
				OutItems.Add(Item);
			}
			return true;
		}

		bool GetIntPair(const FJsonObject& Object, const TCHAR* Field, int32 (&OutPair)[Rank::NumSides])
		{
			const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
			if (!Object.TryGetArrayField(Field, Values) || Values->Num() != Rank::NumSides)
			{
				return false;
			}
			for (int32 Side = 0; Side < Rank::NumSides; ++Side)
			{
				double Number = 0.0;
				if (!(*Values)[Side].IsValid() || !(*Values)[Side]->TryGetNumber(Number))
				{
					return false;
				}
				const int32 Value = static_cast<int32>(Number);
				if (static_cast<double>(Value) != Number)
				{
					return false;
				}
				OutPair[Side] = Value;
			}
			return true;
		}
	}

	FString WriteRecordJson(const FRecordFile& File)
	{
		// 키 순서와 줄바꿈을 손으로 고정한다. 같은 내용이면 같은 바이트가 나온다.
		FString Out;
		Out += TEXT("{\n");
		Out += FString::Printf(TEXT("  \"version\": %d,\n"), File.Version);
		Out += FString::Printf(TEXT("  \"commit\": %s,\n"), *JsonString(File.Commit));
		Out += TEXT("  \"defs\": {\n");
		Out += TEXT("    \"units\": [\n");
		for (int32 Index = 0; Index < File.Units.Num(); ++Index)
		{
			const FRankUnitRow& Unit = File.Units[Index];
			Out += FString::Printf(
				TEXT("      {\"key\": %s, \"name\": %s, \"hp\": %d, \"atk\": %d, \"armor\": %d, \"interval\": %d, \"range\": %d, \"target\": %s, \"magic\": %s, \"revive\": %d, \"linkStat\": %s, \"linkAmount\": %d}%s\n"),
				*JsonString(Unit.Key), *JsonString(Unit.Name), Unit.Hp, Unit.Atk, Unit.Armor, Unit.Interval, Unit.Range,
				*JsonString(Unit.Target), Unit.bMagic ? TEXT("true") : TEXT("false"), Unit.Revive, *JsonString(Unit.LinkStat), Unit.LinkAmount,
				Index + 1 < File.Units.Num() ? TEXT(",") : TEXT(""));
		}
		Out += TEXT("    ],\n");
		Out += FString::Printf(
			TEXT("    \"rules\": {\"maxTicks\": %d, \"gateHp\": %d, \"gateDamage\": %d, \"linkStackMax\": %d, \"rangeMax\": %d, \"intervalMin\": %d}\n"),
			File.Rules.MaxTicks, File.Rules.GateHp, File.Rules.GateDamage, File.Rules.LinkStackMax, File.Rules.RangeMax, File.Rules.IntervalMin);
		Out += TEXT("  },\n");
		Out += TEXT("  \"battles\": [\n");
		for (int32 Index = 0; Index < File.Battles.Num(); ++Index)
		{
			const FRecordedBattle& Battle = File.Battles[Index];
			const FRecord& Record = Battle.Record;
			Out += TEXT("    {\n");
			Out += FString::Printf(TEXT("      \"id\": %s,\n"), *JsonString(Battle.Id));
			Out += FString::Printf(TEXT("      \"ally\": %s,\n"), *JsonStringArray(Battle.AllyRows));
			Out += FString::Printf(TEXT("      \"enemy\": %s,\n"), *JsonStringArray(Battle.EnemyRows));
			Out += FString::Printf(TEXT("      \"outcome\": %s,\n"), *JsonString(OutcomeName(Record.Outcome)));
			Out += FString::Printf(TEXT("      \"tick\": %d,\n"), Record.Tick);
			Out += FString::Printf(TEXT("      \"events\": %d,\n"), Record.NumEvents);
			Out += FString::Printf(TEXT("      \"hash\": %s,\n"), *JsonString(HexU32(Record.Hash)));
			Out += FString::Printf(TEXT("      \"gate\": %d,\n"), Record.Gate);
			Out += FString::Printf(TEXT("      \"barrier\": %d,\n"), Record.Barrier);
			Out += FString::Printf(TEXT("      \"alive\": [%d, %d],\n"), Record.Alive[0], Record.Alive[1]);
			Out += FString::Printf(TEXT("      \"hpSum\": [%d, %d],\n"), Record.HpSum[0], Record.HpSum[1]);
			Out += FString::Printf(TEXT("      \"logChecksum\": %s,\n"), *JsonString(HexU32(Record.LogChecksum)));
			Out += FString::Printf(TEXT("      \"branches\": %s\n"), *JsonStringArray(Battle.Branches));
			Out += Index + 1 < File.Battles.Num() ? TEXT("    },\n") : TEXT("    }\n");
		}
		Out += TEXT("  ]\n");
		Out += TEXT("}\n");
		return Out;
	}

	bool ReadRecordJson(const FString& Text, FRecordFile& OutFile, FString& OutError)
	{
		OutFile = FRecordFile();
		TSharedPtr<FJsonObject> Root;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
		{
			OutError = TEXT("JSON으로 읽히지 않는다");
			return false;
		}
		const TSharedPtr<FJsonObject>* Defs = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* Units = nullptr;
		const TSharedPtr<FJsonObject>* Rules = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* Battles = nullptr;
		if (!GetInt(*Root, TEXT("version"), OutFile.Version) || !Root->TryGetStringField(TEXT("commit"), OutFile.Commit)
			|| !Root->TryGetObjectField(TEXT("defs"), Defs) || !(*Defs)->TryGetArrayField(TEXT("units"), Units)
			|| !(*Defs)->TryGetObjectField(TEXT("rules"), Rules) || !Root->TryGetArrayField(TEXT("battles"), Battles))
		{
			OutError = TEXT("version, commit, defs.units, defs.rules, battles 가운데 빠진 것이 있다");
			return false;
		}

		for (const TSharedPtr<FJsonValue>& Value : *Units)
		{
			const TSharedPtr<FJsonObject>* Unit = nullptr;
			FRankUnitRow Row;
			if (!Value.IsValid() || !Value->TryGetObject(Unit)
				|| !(*Unit)->TryGetStringField(TEXT("key"), Row.Key) || !(*Unit)->TryGetStringField(TEXT("name"), Row.Name)
				|| !GetInt(**Unit, TEXT("hp"), Row.Hp) || !GetInt(**Unit, TEXT("atk"), Row.Atk) || !GetInt(**Unit, TEXT("armor"), Row.Armor)
				|| !GetInt(**Unit, TEXT("interval"), Row.Interval) || !GetInt(**Unit, TEXT("range"), Row.Range)
				|| !(*Unit)->TryGetStringField(TEXT("target"), Row.Target) || !(*Unit)->TryGetBoolField(TEXT("magic"), Row.bMagic)
				|| !GetInt(**Unit, TEXT("revive"), Row.Revive) || !(*Unit)->TryGetStringField(TEXT("linkStat"), Row.LinkStat)
				|| !GetInt(**Unit, TEXT("linkAmount"), Row.LinkAmount))
			{
				OutError = TEXT("defs.units의 항목 형식이 다르다");
				return false;
			}
			OutFile.Units.Add(Row);
		}
		const FJsonObject& RulesObject = **Rules;
		if (!GetInt(RulesObject, TEXT("maxTicks"), OutFile.Rules.MaxTicks) || !GetInt(RulesObject, TEXT("gateHp"), OutFile.Rules.GateHp)
			|| !GetInt(RulesObject, TEXT("gateDamage"), OutFile.Rules.GateDamage) || !GetInt(RulesObject, TEXT("linkStackMax"), OutFile.Rules.LinkStackMax)
			|| !GetInt(RulesObject, TEXT("rangeMax"), OutFile.Rules.RangeMax) || !GetInt(RulesObject, TEXT("intervalMin"), OutFile.Rules.IntervalMin))
		{
			OutError = TEXT("defs.rules의 형식이 다르다");
			return false;
		}

		for (const TSharedPtr<FJsonValue>& Value : *Battles)
		{
			const TSharedPtr<FJsonObject>* BattleObject = nullptr;
			if (!Value.IsValid() || !Value->TryGetObject(BattleObject))
			{
				OutError = TEXT("battles의 항목이 객체가 아니다");
				return false;
			}
			const FJsonObject& Object = **BattleObject;
			FRecordedBattle Battle;
			FString OutcomeText;
			FString HashText;
			FString ChecksumText;
			if (!Object.TryGetStringField(TEXT("id"), Battle.Id) || !GetStringArray(Object, TEXT("ally"), Battle.AllyRows)
				|| !GetStringArray(Object, TEXT("enemy"), Battle.EnemyRows)
				|| !Object.TryGetStringField(TEXT("outcome"), OutcomeText) || !ParseOutcome(OutcomeText, Battle.Record.Outcome)
				|| !GetInt(Object, TEXT("tick"), Battle.Record.Tick) || !GetInt(Object, TEXT("events"), Battle.Record.NumEvents)
				|| !Object.TryGetStringField(TEXT("hash"), HashText) || !ParseHexU32(HashText, Battle.Record.Hash)
				|| !GetInt(Object, TEXT("gate"), Battle.Record.Gate) || !GetInt(Object, TEXT("barrier"), Battle.Record.Barrier)
				|| !GetIntPair(Object, TEXT("alive"), Battle.Record.Alive) || !GetIntPair(Object, TEXT("hpSum"), Battle.Record.HpSum)
				|| !Object.TryGetStringField(TEXT("logChecksum"), ChecksumText) || !ParseHexU32(ChecksumText, Battle.Record.LogChecksum)
				|| !GetStringArray(Object, TEXT("branches"), Battle.Branches))
			{
				OutError = FString::Printf(TEXT("battles의 %d번째 항목 형식이 다르다"), OutFile.Battles.Num());
				return false;
			}
			OutFile.Battles.Add(MoveTemp(Battle));
		}
		return true;
	}

	void DiffDefs(const TArray<FRankUnitRow>& UnitsA, const FRankRulesRow& RulesA, const TCHAR* LabelA,
		const TArray<FRankUnitRow>& UnitsB, const FRankRulesRow& RulesB, const TCHAR* LabelB, TArray<FString>& OutDiffs)
	{
		auto DiffText = [&](const FString& Where, const FString& A, const FString& B)
		{
			if (!A.Equals(B, ESearchCase::CaseSensitive))
			{
				OutDiffs.Add(FString::Printf(TEXT("%s: %s %s, %s %s"), *Where, LabelA, *A, LabelB, *B));
			}
		};
		auto DiffInt = [&](const FString& Where, int32 A, int32 B)
		{
			DiffText(Where, FString::FromInt(A), FString::FromInt(B));
		};
		auto FindKey = [](const TArray<FRankUnitRow>& Units, const FString& Key) -> const FRankUnitRow*
		{
			return Units.FindByPredicate([&Key](const FRankUnitRow& Row) { return Row.Key.Equals(Key, ESearchCase::CaseSensitive); });
		};

		if (UnitsA.Num() != UnitsB.Num())
		{
			OutDiffs.Add(FString::Printf(TEXT("병과 수: %s %d, %s %d"), LabelA, UnitsA.Num(), LabelB, UnitsB.Num()));
		}
		for (const FRankUnitRow& B : UnitsB)
		{
			const FRankUnitRow* A = FindKey(UnitsA, B.Key);
			if (A == nullptr)
			{
				OutDiffs.Add(FString::Printf(TEXT("병과 %s: %s에 없다"), *B.Key, LabelA));
				continue;
			}
			const FString& Key = B.Key;
			DiffText(Key + TEXT(".name"), A->Name, B.Name);
			DiffInt(Key + TEXT(".hp"), A->Hp, B.Hp);
			DiffInt(Key + TEXT(".atk"), A->Atk, B.Atk);
			DiffInt(Key + TEXT(".armor"), A->Armor, B.Armor);
			DiffInt(Key + TEXT(".interval"), A->Interval, B.Interval);
			DiffInt(Key + TEXT(".range"), A->Range, B.Range);
			DiffText(Key + TEXT(".target"), A->Target, B.Target);
			DiffText(Key + TEXT(".magic"), A->bMagic ? TEXT("TRUE") : TEXT("FALSE"), B.bMagic ? TEXT("TRUE") : TEXT("FALSE"));
			DiffInt(Key + TEXT(".revive"), A->Revive, B.Revive);
			DiffText(Key + TEXT(".linkStat"), A->LinkStat, B.LinkStat);
			DiffInt(Key + TEXT(".linkAmount"), A->LinkAmount, B.LinkAmount);
		}
		for (const FRankUnitRow& A : UnitsA)
		{
			if (FindKey(UnitsB, A.Key) == nullptr)
			{
				OutDiffs.Add(FString::Printf(TEXT("병과 %s: %s에 없다"), *A.Key, LabelB));
			}
		}
		DiffInt(TEXT("rules.maxTicks"), RulesA.MaxTicks, RulesB.MaxTicks);
		DiffInt(TEXT("rules.gateHp"), RulesA.GateHp, RulesB.GateHp);
		DiffInt(TEXT("rules.gateDamage"), RulesA.GateDamage, RulesB.GateDamage);
		DiffInt(TEXT("rules.linkStackMax"), RulesA.LinkStackMax, RulesB.LinkStackMax);
		DiffInt(TEXT("rules.rangeMax"), RulesA.RangeMax, RulesB.RangeMax);
		DiffInt(TEXT("rules.intervalMin"), RulesA.IntervalMin, RulesB.IntervalMin);
	}
}

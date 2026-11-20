#include "RankContent.h"

#include "Misc/FileHelper.h"

namespace RankContent
{
	namespace
	{
		const TCHAR* const UnitsColumns[] = {
			TEXT("key"), TEXT("name"), TEXT("hp"), TEXT("atk"), TEXT("armor"), TEXT("interval"),
			TEXT("range"), TEXT("target"), TEXT("magic"), TEXT("revive"), TEXT("linkStat"), TEXT("linkAmount") };

		const TCHAR* const RulesColumns[] = {
			TEXT("maxTicks"), TEXT("gateHp"), TEXT("gateDamage"), TEXT("linkStackMax"), TEXT("rangeMax"), TEXT("intervalMin") };

		void SplitLines(const FString& Text, TArray<FString>& OutLines, TArray<int32>& OutLineNumbers)
		{
			SplitCsvLines(Text, OutLines, OutLineNumbers);
		}

		// 부호(-)와 숫자만 받는다. 공백, 소수점, 범위를 넘는 값은 실패다.
		bool ParseInt(const FString& Text, int32& OutValue)
		{
			if (Text.IsEmpty())
			{
				return false;
			}
			int32 Pos = 0;
			const bool bNegative = Text[0] == TEXT('-');
			if (bNegative)
			{
				Pos = 1;
			}
			if (Pos >= Text.Len())
			{
				return false;
			}
			int64 Value = 0;
			for (; Pos < Text.Len(); ++Pos)
			{
				const TCHAR Char = Text[Pos];
				if (Char < TEXT('0') || Char > TEXT('9'))
				{
					return false;
				}
				Value = Value * 10 + (Char - TEXT('0'));
				if (Value > static_cast<int64>(MAX_int32) + 1)
				{
					return false;
				}
			}
			Value = bNegative ? -Value : Value;
			if (Value < MIN_int32 || Value > MAX_int32)
			{
				return false;
			}
			OutValue = static_cast<int32>(Value);
			return true;
		}

		bool ParseBool(const FString& Text, bool& OutValue)
		{
			if (Text.Equals(TEXT("TRUE"), ESearchCase::CaseSensitive))
			{
				OutValue = true;
				return true;
			}
			if (Text.Equals(TEXT("FALSE"), ESearchCase::CaseSensitive))
			{
				OutValue = false;
				return true;
			}
			return false;
		}

		bool CheckHeader(const FString& Line, const TCHAR* const* Expected, int32 NumExpected, const TCHAR* FileLabel, FString& OutError)
		{
			TArray<FString> Fields;
			if (!SplitCsvLine(Line, Fields, OutError))
			{
				OutError = FString::Printf(TEXT("%s 1줄: %s"), FileLabel, *OutError);
				return false;
			}
			bool bSame = Fields.Num() == NumExpected;
			for (int32 Index = 0; bSame && Index < NumExpected; ++Index)
			{
				bSame = Fields[Index].Equals(Expected[Index], ESearchCase::CaseSensitive);
			}
			if (!bSame)
			{
				FString ExpectedText;
				for (int32 Index = 0; Index < NumExpected; ++Index)
				{
					ExpectedText += (Index > 0 ? TEXT(",") : TEXT("")) + FString(Expected[Index]);
				}
				OutError = FString::Printf(TEXT("%s 1줄: 열 이름이 다르다. 기대 \"%s\", 실제 \"%s\""), FileLabel, *ExpectedText, *Line);
				return false;
			}
			return true;
		}

		bool IntField(const TArray<FString>& Fields, int32 Column, const TCHAR* const* Names, const TCHAR* FileLabel, int32 LineNumber, int32& OutValue, FString& OutError)
		{
			if (!ParseInt(Fields[Column], OutValue))
			{
				OutError = FString::Printf(TEXT("%s %d줄 %s: 정수가 아니다(\"%s\")"), FileLabel, LineNumber, Names[Column], *Fields[Column]);
				return false;
			}
			return true;
		}
	}

	void SplitCsvLines(const FString& Text, TArray<FString>& OutLines, TArray<int32>& OutLineNumbers)
	{
		OutLines.Reset();
		OutLineNumbers.Reset();
		TArray<FString> Raw;
		Text.ParseIntoArray(Raw, TEXT("\n"), false);
		for (int32 Index = 0; Index < Raw.Num(); ++Index)
		{
			FString Line = Raw[Index];
			if (Line.EndsWith(TEXT("\r"), ESearchCase::CaseSensitive))
			{
				Line.LeftChopInline(1, EAllowShrinking::No);
			}
			if (Line.IsEmpty())
			{
				continue;
			}
			OutLines.Add(MoveTemp(Line));
			OutLineNumbers.Add(Index + 1);
		}
	}

	bool SplitCsvLine(const FString& Line, TArray<FString>& OutFields, FString& OutError)
	{
		OutFields.Reset();
		FString Current;
		bool bQuoted = false;
		bool bFieldStart = true;
		for (int32 Pos = 0; Pos < Line.Len(); ++Pos)
		{
			const TCHAR Char = Line[Pos];
			if (bQuoted)
			{
				if (Char == TEXT('"'))
				{
					if (Pos + 1 < Line.Len() && Line[Pos + 1] == TEXT('"'))
					{
						Current.AppendChar(TEXT('"')); // 안의 큰따옴표는 두 번 쓴다
						++Pos;
					}
					else
					{
						bQuoted = false;
					}
				}
				else
				{
					Current.AppendChar(Char);
				}
				continue;
			}
			if (Char == TEXT(',') )
			{
				OutFields.Add(MoveTemp(Current));
				Current.Reset();
				bFieldStart = true;
				continue;
			}
			if (Char == TEXT('"') && bFieldStart)
			{
				bQuoted = true;
				bFieldStart = false;
				continue;
			}
			Current.AppendChar(Char);
			bFieldStart = false;
		}
		if (bQuoted)
		{
			OutError = TEXT("큰따옴표가 닫히지 않았다");
			return false;
		}
		OutFields.Add(MoveTemp(Current));
		return true;
	}

	bool ParseUnitsCsv(const FString& Text, TArray<FRankUnitRow>& OutRows, FString& OutError)
	{
		OutRows.Reset();
		const TCHAR* const FileLabel = TEXT("units.csv");
		constexpr int32 NumColumns = UE_ARRAY_COUNT(UnitsColumns);
		TArray<FString> Lines;
		TArray<int32> LineNumbers;
		SplitLines(Text, Lines, LineNumbers);
		if (Lines.Num() == 0)
		{
			OutError = FString::Printf(TEXT("%s: 비어 있다"), FileLabel);
			return false;
		}
		if (!CheckHeader(Lines[0], UnitsColumns, NumColumns, FileLabel, OutError))
		{
			return false;
		}

		for (int32 LineIndex = 1; LineIndex < Lines.Num(); ++LineIndex)
		{
			const int32 LineNumber = LineNumbers[LineIndex];
			TArray<FString> Fields;
			if (!SplitCsvLine(Lines[LineIndex], Fields, OutError))
			{
				OutError = FString::Printf(TEXT("%s %d줄: %s"), FileLabel, LineNumber, *OutError);
				return false;
			}
			if (Fields.Num() != NumColumns)
			{
				OutError = FString::Printf(TEXT("%s %d줄: 칸이 %d개다. %d개여야 한다"), FileLabel, LineNumber, Fields.Num(), NumColumns);
				return false;
			}

			FRankUnitRow Row;
			Row.Key = Fields[0];
			Row.Name = Fields[1];
			Row.Target = Fields[7];
			Row.LinkStat = Fields[10];
			if (!IntField(Fields, 2, UnitsColumns, FileLabel, LineNumber, Row.Hp, OutError)
				|| !IntField(Fields, 3, UnitsColumns, FileLabel, LineNumber, Row.Atk, OutError)
				|| !IntField(Fields, 4, UnitsColumns, FileLabel, LineNumber, Row.Armor, OutError)
				|| !IntField(Fields, 5, UnitsColumns, FileLabel, LineNumber, Row.Interval, OutError)
				|| !IntField(Fields, 6, UnitsColumns, FileLabel, LineNumber, Row.Range, OutError)
				|| !IntField(Fields, 9, UnitsColumns, FileLabel, LineNumber, Row.Revive, OutError)
				|| !IntField(Fields, 11, UnitsColumns, FileLabel, LineNumber, Row.LinkAmount, OutError))
			{
				return false;
			}
			if (!ParseBool(Fields[8], Row.bMagic))
			{
				OutError = FString::Printf(TEXT("%s %d줄 magic: TRUE나 FALSE가 아니다(\"%s\")"), FileLabel, LineNumber, *Fields[8]);
				return false;
			}
			OutRows.Add(MoveTemp(Row));
		}
		return true;
	}

	bool ParseRulesCsv(const FString& Text, FRankRulesRow& OutRow, FString& OutError)
	{
		const TCHAR* const FileLabel = TEXT("rules.csv");
		constexpr int32 NumColumns = UE_ARRAY_COUNT(RulesColumns);
		TArray<FString> Lines;
		TArray<int32> LineNumbers;
		SplitLines(Text, Lines, LineNumbers);
		if (Lines.Num() == 0)
		{
			OutError = FString::Printf(TEXT("%s: 비어 있다"), FileLabel);
			return false;
		}
		if (!CheckHeader(Lines[0], RulesColumns, NumColumns, FileLabel, OutError))
		{
			return false;
		}
		if (Lines.Num() != 2)
		{
			OutError = FString::Printf(TEXT("%s: 데이터가 %d줄이다. 한 줄이어야 한다"), FileLabel, Lines.Num() - 1);
			return false;
		}

		const int32 LineNumber = LineNumbers[1];
		TArray<FString> Fields;
		if (!SplitCsvLine(Lines[1], Fields, OutError))
		{
			OutError = FString::Printf(TEXT("%s %d줄: %s"), FileLabel, LineNumber, *OutError);
			return false;
		}
		if (Fields.Num() != NumColumns)
		{
			OutError = FString::Printf(TEXT("%s %d줄: 칸이 %d개다. %d개여야 한다"), FileLabel, LineNumber, Fields.Num(), NumColumns);
			return false;
		}

		FRankRulesRow Row;
		if (!IntField(Fields, 0, RulesColumns, FileLabel, LineNumber, Row.MaxTicks, OutError)
			|| !IntField(Fields, 1, RulesColumns, FileLabel, LineNumber, Row.GateHp, OutError)
			|| !IntField(Fields, 2, RulesColumns, FileLabel, LineNumber, Row.GateDamage, OutError)
			|| !IntField(Fields, 3, RulesColumns, FileLabel, LineNumber, Row.LinkStackMax, OutError)
			|| !IntField(Fields, 4, RulesColumns, FileLabel, LineNumber, Row.RangeMax, OutError)
			|| !IntField(Fields, 5, RulesColumns, FileLabel, LineNumber, Row.IntervalMin, OutError))
		{
			return false;
		}
		OutRow = Row;
		return true;
	}

	bool ToCoreUnitDef(const FRankUnitRow& Row, Rank::FUnitDef& OutDef, FString& OutError)
	{
		if (Row.Key.Len() != 1 || Row.Key[0] <= 32 || Row.Key[0] >= 127 || Row.Key[0] == TEXT('.'))
		{
			OutError = FString::Printf(TEXT("병과 키는 '.'이 아닌 ASCII 한 글자여야 한다(\"%s\")"), *Row.Key);
			return false;
		}

		Rank::FUnitDef Def;
		Def.Key = static_cast<uint8>(Row.Key[0]);
		Def.Hp = Row.Hp;
		Def.Atk = Row.Atk;
		Def.Armor = Row.Armor;
		Def.Interval = Row.Interval;
		Def.Range = Row.Range;
		Def.bMagic = Row.bMagic;
		Def.Revive = Row.Revive;
		Def.LinkAmount = Row.LinkAmount;

		if (Row.Target.Equals(TEXT("Front"), ESearchCase::CaseSensitive)) { Def.Target = Rank::ETarget::Front; }
		else if (Row.Target.Equals(TEXT("Back"), ESearchCase::CaseSensitive)) { Def.Target = Rank::ETarget::Back; }
		else if (Row.Target.Equals(TEXT("Support"), ESearchCase::CaseSensitive)) { Def.Target = Rank::ETarget::Support; }
		else if (Row.Target.Equals(TEXT("Area"), ESearchCase::CaseSensitive)) { Def.Target = Rank::ETarget::Area; }
		else
		{
			OutError = FString::Printf(TEXT("%s target: 어휘 밖의 값(\"%s\")"), *Row.Key, *Row.Target);
			return false;
		}

		if (Row.LinkStat.Equals(TEXT("None"), ESearchCase::CaseSensitive)) { Def.LinkStat = Rank::ELinkStat::None; }
		else if (Row.LinkStat.Equals(TEXT("Armor"), ESearchCase::CaseSensitive)) { Def.LinkStat = Rank::ELinkStat::Armor; }
		else if (Row.LinkStat.Equals(TEXT("Atk"), ESearchCase::CaseSensitive)) { Def.LinkStat = Rank::ELinkStat::Atk; }
		else if (Row.LinkStat.Equals(TEXT("Range"), ESearchCase::CaseSensitive)) { Def.LinkStat = Rank::ELinkStat::Range; }
		else if (Row.LinkStat.Equals(TEXT("Hp"), ESearchCase::CaseSensitive)) { Def.LinkStat = Rank::ELinkStat::Hp; }
		else if (Row.LinkStat.Equals(TEXT("Interval"), ESearchCase::CaseSensitive)) { Def.LinkStat = Rank::ELinkStat::Interval; }
		else
		{
			OutError = FString::Printf(TEXT("%s linkStat: 어휘 밖의 값(\"%s\")"), *Row.Key, *Row.LinkStat);
			return false;
		}

		OutDef = Def;
		return true;
	}

	void ToCoreRules(const FRankRulesRow& Row, Rank::FRuleParams& OutRules)
	{
		OutRules.MaxTicks = Row.MaxTicks;
		OutRules.GateHp = Row.GateHp;
		OutRules.GateDamage = Row.GateDamage;
		OutRules.LinkStackMax = Row.LinkStackMax;
		OutRules.RangeMax = Row.RangeMax;
		OutRules.IntervalMin = Row.IntervalMin;
	}

	bool LoadDefsFiles(const FString& UnitsCsvPath, const FString& RulesCsvPath,
		TArray<FRankUnitRow>& OutUnitRows, FRankRulesRow& OutRulesRow,
		TArray<Rank::FUnitDef>& OutDefs, Rank::FRuleParams& OutRules, FString& OutError)
	{
		OutDefs.Reset();
		FString UnitsText;
		FString RulesText;
		// FFileHelper::LoadFileToString은 UTF-8 BOM이 있으면 건너뛰고 UTF-8로 읽는다(Engine/Source/Runtime/Core/Private/Misc/FileHelper.cpp, BufferToString).
		if (!FFileHelper::LoadFileToString(UnitsText, *UnitsCsvPath))
		{
			OutError = FString::Printf(TEXT("파일을 읽지 못했다: %s"), *UnitsCsvPath);
			return false;
		}
		if (!FFileHelper::LoadFileToString(RulesText, *RulesCsvPath))
		{
			OutError = FString::Printf(TEXT("파일을 읽지 못했다: %s"), *RulesCsvPath);
			return false;
		}
		if (!ParseUnitsCsv(UnitsText, OutUnitRows, OutError) || !ParseRulesCsv(RulesText, OutRulesRow, OutError))
		{
			return false;
		}
		for (const FRankUnitRow& Row : OutUnitRows)
		{
			Rank::FUnitDef Def;
			if (!ToCoreUnitDef(Row, Def, OutError))
			{
				OutError = FString::Printf(TEXT("units.csv: %s"), *OutError);
				return false;
			}
			OutDefs.Add(Def);
		}
		ToCoreRules(OutRulesRow, OutRules);
		return true;
	}

	bool ParseFormationRows(TConstArrayView<FString> Rows, Rank::FFormation& OutFormation, FString& OutError)
	{
		OutFormation = Rank::FFormation();
		if (Rows.Num() != Rank::BoardRows)
		{
			OutError = FString::Printf(TEXT("진형은 %d행이어야 한다. %d행이다"), Rank::BoardRows, Rows.Num());
			return false;
		}
		for (int32 Row = 0; Row < Rank::BoardRows; ++Row)
		{
			const FString& Text = Rows[Row];
			if (Text.Len() != Rank::BoardCols)
			{
				OutError = FString::Printf(TEXT("행 %d는 %d자여야 한다(\"%s\")"), Row, Rank::BoardCols, *Text);
				return false;
			}
			for (int32 Col = 0; Col < Rank::BoardCols; ++Col)
			{
				const TCHAR Char = Text[Col];
				if (Char == TEXT('.'))
				{
					continue;
				}
				if (Char <= 32 || Char >= 127)
				{
					OutError = FString::Printf(TEXT("행 %d 열 %d: ASCII 글자가 아니다(\"%s\")"), Row, Col, *Text);
					return false;
				}
				OutFormation.Cells[Row][Col] = static_cast<uint8>(Char);
			}
		}
		return true;
	}
}

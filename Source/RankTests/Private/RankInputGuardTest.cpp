#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "RankContent.h"
#include "RankFormationMask.h"
#include "RankTestUtils.h"

// 입력 안전장치 두 가지를 본다(2026-11-20 결정, docs/review/rankcore-crossref-2026-11-20.md "확인이 필요한 곳" 6번, 7번).
// 기대값의 출처는 결정 기록의 문장이다. 손계산이 아니라 성질이다. 화면 없이 돈다.
// - 7번: 배치 판정은 칸 수가 1~4(§2.1)가 아닌 모양을 "놓을 수 없음"으로 돌려준다. 비트마스크 판정과 칸 단위 판정 둘 다.
// - 6번: CSV를 읽는 쪽(RankContent)이 maxTicks 0 이하, gateHp 0 이하, 병과 키 중복을 읽기 실패로 돌려준다.

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags RankTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRankPlacementRejectsBadCellCountTest, "RankAndFile.Core.Placement_RejectsBadCellCount", RankTestFlags)

bool FRankPlacementRejectsBadCellCountTest::RunTest(const FString& Parameters)
{
	Rank::FFormationMask Mask; // 빈 진형판
	bool Occupied[Rank::BoardRows][Rank::BoardCols] = {};

	int32 RejectPassed = 0;
	int32 RejectTotal = 0;
	int32 AcceptPassed = 0;
	int32 AcceptTotal = 0;

	// 1. 칸 수가 1~4가 아니면 진형판의 모든 위치에서 두 판정 모두 false다.
	//    칸 좌표는 모두 (0, 0)으로 둔다. 좌표가 아니라 칸 수로 막는지 본다.
	const int32 BadCounts[] = { -1, 0, Rank::MaxShapeCells + 1 };
	for (const int32 NumCells : BadCounts)
	{
		Rank::FShape Shape;
		Shape.NumCells = NumCells;
		bool bReported = false;
		for (int32 Row = 0; Row < Rank::BoardRows; ++Row)
		{
			for (int32 Col = 0; Col < Rank::BoardCols; ++Col)
			{
				++RejectTotal;
				const bool bMask = Mask.CanPlace(Shape, Row, Col);
				const bool bCellwise = Rank::CanPlaceCellwise(Occupied, Shape, Row, Col);
				if (!bMask && !bCellwise)
				{
					++RejectPassed;
				}
				else if (!bReported)
				{
					bReported = true; // 칸 수마다 처음 어긋난 위치만 적는다
					AddError(FString::Printf(TEXT("배치 판정 칸 수 %d, 위치 (%d, %d): 비트마스크 %s, 칸 단위 %s. 둘 다 false여야 한다"),
						NumCells, Row, Col, bMask ? TEXT("true") : TEXT("false"), bCellwise ? TEXT("true") : TEXT("false")));
				}
			}
		}
	}

	// 2. 칸 수가 1~4인 모양은 그대로 놓을 수 있다. 가로 한 줄 모양을 빈 진형판의 (0, 0)에 놓는다.
	for (int32 NumCells = 1; NumCells <= Rank::MaxShapeCells; ++NumCells)
	{
		Rank::FShape Shape;
		Shape.NumCells = NumCells;
		for (int32 Index = 0; Index < NumCells; ++Index)
		{
			Shape.Cells[Index].Row = 0;
			Shape.Cells[Index].Col = static_cast<int8>(Index);
		}
		++AcceptTotal;
		const bool bMask = Mask.CanPlace(Shape, 0, 0);
		const bool bCellwise = Rank::CanPlaceCellwise(Occupied, Shape, 0, 0);
		if (bMask && bCellwise)
		{
			++AcceptPassed;
		}
		else
		{
			AddError(FString::Printf(TEXT("배치 판정 칸 수 %d, 위치 (0, 0): 비트마스크 %s, 칸 단위 %s. 둘 다 true여야 한다"),
				NumCells, bMask ? TEXT("true") : TEXT("false"), bCellwise ? TEXT("true") : TEXT("false")));
		}
	}

	AddInfo(FString::Printf(TEXT("배치 판정 칸 수 검사: 놓을 수 없음 %d/%d (칸 수 -1, 0, %d, 위치 %d곳씩), 놓을 수 있음 %d/%d (칸 수 1~%d)"),
		RejectPassed, RejectTotal, Rank::MaxShapeCells + 1, Rank::BoardRows * Rank::BoardCols, AcceptPassed, AcceptTotal, Rank::MaxShapeCells));
	return RejectTotal > 0 && AcceptTotal > 0 && RejectPassed == RejectTotal && AcceptPassed == AcceptTotal;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRankContentDefsRejectBadValuesTest, "RankAndFile.Content.Defs_RejectsBadValues", RankTestFlags)

bool FRankContentDefsRejectBadValuesTest::RunTest(const FString& Parameters)
{
	// 수치는 코드에 적지 않는다. 지금 Data/defs의 글을 읽어 한 곳만 바꾼다.
	const FString UnitsPath = RankTests::ProjectFile(TEXT("Data/defs/units.csv"));
	const FString RulesPath = RankTests::ProjectFile(TEXT("Data/defs/rules.csv"));
	FString UnitsText;
	FString RulesText;
	if (!FFileHelper::LoadFileToString(UnitsText, *UnitsPath) || !FFileHelper::LoadFileToString(RulesText, *RulesPath))
	{
		AddError(FString::Printf(TEXT("defs: 파일을 읽지 못했다(%s, %s)"), *UnitsPath, *RulesPath));
		return false;
	}

	// 0. 바꾸지 않은 지금 Data/defs는 읽힌다. 안 읽히면 아래 검사는 뜻이 없다.
	{
		TArray<FRankUnitRow> UnitRows;
		FRankRulesRow RulesRow;
		FString Error;
		if (!RankContent::ParseUnitsCsv(UnitsText, UnitRows, Error) || !RankContent::ParseRulesCsv(RulesText, RulesRow, Error))
		{
			AddError(FString::Printf(TEXT("defs: 바꾸지 않은 Data/defs가 읽히지 않는다: %s"), *Error));
			return false;
		}
		if (UnitRows.Num() == 0)
		{
			AddError(TEXT("defs: units.csv에 병과가 없다"));
			return false;
		}
	}

	TArray<FString> RulesLines;
	TArray<int32> RulesLineNumbers;
	RankContent::SplitCsvLines(RulesText, RulesLines, RulesLineNumbers);
	TArray<FString> RulesHeader;
	TArray<FString> RulesFields;
	FString SplitError;
	if (RulesLines.Num() != 2
		|| !RankContent::SplitCsvLine(RulesLines[0], RulesHeader, SplitError)
		|| !RankContent::SplitCsvLine(RulesLines[1], RulesFields, SplitError))
	{
		AddError(TEXT("defs: rules.csv를 머리줄과 데이터 한 줄로 나누지 못했다"));
		return false;
	}

	TArray<FString> UnitsLines;
	TArray<int32> UnitsLineNumbers;
	RankContent::SplitCsvLines(UnitsText, UnitsLines, UnitsLineNumbers);
	if (UnitsLines.Num() < 2)
	{
		AddError(TEXT("defs: units.csv에 데이터 줄이 없다"));
		return false;
	}

	int32 Passed = 0;
	int32 Total = 0;

	// 실패해야 하고, 실패 사유에 그 열 이름이 "열 이름: " 꼴로 들어 있어야 한다(머리줄 오류 같은 다른 사유로 실패한 것과 가른다).
	auto ExpectRejected = [&](const FString& Case, bool bParsed, const FString& Error, const TCHAR* Column)
	{
		++Total;
		const FString Marker = FString::Printf(TEXT("%s: "), Column);
		if (bParsed)
		{
			AddError(FString::Printf(TEXT("defs %s: 읽기 실패여야 하는데 읽혔다"), *Case));
		}
		else if (!Error.Contains(Marker, ESearchCase::CaseSensitive))
		{
			AddError(FString::Printf(TEXT("defs %s: 실패 사유에 \"%s\"가 없다(\"%s\")"), *Case, *Marker, *Error));
		}
		else
		{
			++Passed;
		}
	};

	// 1. rules.csv의 maxTicks와 gateHp를 0과 -1로 바꾼다.
	const TCHAR* const RulesColumns[] = { TEXT("maxTicks"), TEXT("gateHp") };
	const TCHAR* const BadValues[] = { TEXT("0"), TEXT("-1") };
	for (const TCHAR* Column : RulesColumns)
	{
		const int32 ColumnIndex = RulesHeader.IndexOfByKey(FString(Column));
		if (ColumnIndex == INDEX_NONE || ColumnIndex >= RulesFields.Num())
		{
			++Total;
			AddError(FString::Printf(TEXT("defs: rules.csv에 %s 열이 없다"), Column));
			continue;
		}
		for (const TCHAR* Value : BadValues)
		{
			TArray<FString> Changed = RulesFields;
			Changed[ColumnIndex] = Value;
			const FString Text = RulesLines[0] + TEXT("\n") + FString::Join(Changed, TEXT(",")) + TEXT("\n");
			FRankRulesRow Row;
			FString Error;
			const bool bParsed = RankContent::ParseRulesCsv(Text, Row, Error);
			ExpectRejected(FString::Printf(TEXT("rules.csv %s %s"), Column, Value), bParsed, Error, Column);
		}
	}

	// 2. units.csv의 첫 데이터 줄을 끝에 한 번 더 붙인다(병과 키 중복).
	{
		FString Text = UnitsText;
		if (!Text.EndsWith(TEXT("\n")))
		{
			Text += TEXT("\n");
		}
		Text += UnitsLines[1] + TEXT("\n");
		TArray<FRankUnitRow> Rows;
		FString Error;
		const bool bParsed = RankContent::ParseUnitsCsv(Text, Rows, Error);
		ExpectRejected(TEXT("units.csv 병과 키 중복"), bParsed, Error, TEXT("key"));
	}

	AddInfo(FString::Printf(TEXT("defs 읽기 실패 %d/%d (maxTicks 0·-1, gateHp 0·-1, 병과 키 중복), 바꾸지 않은 Data/defs는 읽힘"), Passed, Total));
	return Total > 0 && Passed == Total;
}

#endif // WITH_DEV_AUTOMATION_TESTS

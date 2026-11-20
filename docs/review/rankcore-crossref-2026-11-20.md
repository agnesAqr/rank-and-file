# RankCore 조항 대조표 (2026-11-20)

사람이 `Source/RankCore`를 기준으로 확정하기 전에 `docs/rules.md`의 조항과 코드를 맞대어 리뷰할 때 쓰는 자료다(`Source/RankCore/AGENTS.md` "기준으로 확정하는 절차" 3번). AI는 조항마다 해당 코드의 파일과 줄을 찾고, 문장과 달라 보이는 곳을 표시만 했다. 코드가 맞는지 틀렸는지는 판정하지 않았고, 코드와 문서는 고치지 않았다(루트 `AGENTS.md` 3절 5번).

- 기준 문서: `docs/rules.md` 작업 트리 판(11/20 고침: §9 17·18번 반영).
- 줄 번호: 2026-11-20 커밋하지 않은 작업 트리 기준이다(브랜치 `feat/rules-and-rankcore`, 마지막 커밋 `4d1b039` 위에 `RankLinks.cpp`, `RankSimulate.cpp`, `RankTypes.h`의 11/20 변경이 얹힌 상태). 커밋하거나 코드를 고치면 줄이 밀린다.
- 다룬 파일: 아래 10개 전부.
- 뺀 파일: `Source/RankCore/Public/RankRandom.h`, `Private/RankRandom.cpp`, `Public/RankPlacement.h`, `Private/RankPlacement.cpp`. 다른 작업자가 만드는 중이라 열지도, 목록을 보지도 않았다. 그래서 무작위 배치, 정수 난수기, 분대 배치를 진형(`FFormation`)으로 옮기는 코드는 이 표에 없다.
- 보지 않은 것: 손계산 시나리오(`Source/RankTests/Fixtures/hand_v1.json`)는 열지 않았고 기대값을 계산하지 않았다. 기획 단계의 JS 참조 구현은 근거로 쓰지 않았다. `docs/rules.md`의 "한눈에 보기"는 개요라 따로 대조하지 않았다.
- 빌드와 테스트는 돌리지 않았다(이번 작업의 지시). 아래 코드 동작 설명은 모두 소스를 읽고 적은 것이다.

작업 정의(루트 `AGENTS.md` 3절 1번):

```
목표: docs/rules.md 조항마다 RankCore 코드 위치를 적은 대조표
완료 기준: §1~§9의 조항·하위 규칙·§9 1~18번이 모두 표에 있고, 인용한 파일:줄을 모두 다시 열어 맞췄다
검증 명령: 없음(문서만 만든다). 인용한 파일:줄을 다시 열어 대조한다
금지: 기존 파일 수정, hand_v1.json 열기, JS 참조 구현을 근거로 쓰기, 맞고 틀림 판정, 개인 정보·사용자 폴더 경로
산출물: docs/review/rankcore-crossref-2026-11-20.md
```

다룬 파일(아래 표에서는 파일 이름만 쓴다):

| 파일 이름 | 저장소 경로 | 줄 수 |
|---|---|---|
| `RankTypes.h` | `Source/RankCore/Public/RankTypes.h` | 174 |
| `RankBattle.h` | `Source/RankCore/Public/RankBattle.h` | 23 |
| `RankFormationMask.h` | `Source/RankCore/Public/RankFormationMask.h` | 42 |
| `RankSimulate.cpp` | `Source/RankCore/Private/RankSimulate.cpp` | 449 |
| `RankLinks.cpp` | `Source/RankCore/Private/RankLinks.cpp` | 115 |
| `RankHash.h` | `Source/RankCore/Private/RankHash.h` | 21 |
| `RankJudge.cpp` | `Source/RankCore/Private/RankJudge.cpp` | 39 |
| `RankFormationMask.cpp` | `Source/RankCore/Private/RankFormationMask.cpp` | 89 |
| `RankCoreModule.cpp` | `Source/RankCore/Private/RankCoreModule.cpp` | 3 |
| `RankCore.Build.cs` | `Source/RankCore/RankCore.Build.cs` | 11 |

## 1. 조항 대조표

"확인 N번"은 3절 "확인이 필요한 곳"의 번호다. 코드 위치에서 쌍반점(;)은 파일을 나누고, 쉼표는 같은 파일의 줄을 나눈다.

### §1 용어

| 조항 | 조항 요약(한 줄) | 코드 위치(파일:줄) | 메모 |
|---|---|---|---|
| §1 진영 | 진영 0은 플레이어, 진영 1은 해골 | RankTypes.h:13, 120; RankSimulate.cpp:11 | 진영 번호는 배열 인덱스로만 쓴다. 성문은 `Gate[0]`, 결계는 `Gate[1]`(RankTypes.h:120) |
| §1 진형판 | 진영마다 6열 × 4행, 행 0이 맨 앞 | RankTypes.h:9-12, 90-94 | 규칙 상수다(§9 18번 행). 행 0을 맨 앞으로 보는 것은 생성 순서(§3.1 행)로 나타난다 |
| §1 열 번호 | 왼쪽부터 0~5, 같은 번호끼리 마주 본다 | RankSimulate.cpp:167 | |
| §1 병과 | 병사의 종류, 수치는 §7 | RankTypes.h:62-76; RankLinks.cpp:20-34 | 병과 키(ASCII 한 글자)로 찾는다. 병과 이름은 코어에 없다 |
| §1 유닛 | 전투에 나선 병사 한 명 | RankSimulate.cpp:8-22, 110-124 | |
| §1 대열·대열 순번 | 열마다 빈칸 없이 앞으로 붙은 줄, 맨 앞이 0번째 | RankSimulate.cpp:36, 248 | 변수 `Rank`가 대열 순번이다 |
| §1 틱 | 전투 시간의 단위 | RankSimulate.cpp:406 | 1틱의 길이(0.1초)는 §7 "1틱 = 0.1초" 행 |
| §1 간격 | 몇 틱마다 한 번 공격하는지 | RankTypes.h:69; RankSimulate.cpp:16, 117 | |
| §1 대기 시간 | 간격에서 시작해 틱마다 1씩 줄고 0이면 공격 | RankSimulate.cpp:18, 119, 252-265 | 변수 `Cd` |
| §1 사거리 | 대열 몇 번째 자리까지 공격하는지 | RankSimulate.cpp:17, 118, 260 | |
| §1 성문·결계 | 진영 0은 성문, 진영 1은 결계, 둘 다 60에서 시작 | RankSimulate.cpp:37, 394-397; RankTypes.h:120 | 60은 `gateHp`(§7)에서 읽는다(확인 4번) |

### §2 기본 조항

| 조항 | 조항 요약(한 줄) | 코드 위치(파일:줄) | 메모 |
|---|---|---|---|
| §2.1 분대 모양 | 1~4칸이 변끼리 붙은 폴리오미노(polyomino), 한 칸에 한 명 | RankFormationMask.h:7-20 | 칸 자리 4개(`MaxShapeCells`)만 있다. 칸 수와 연결성 검사는 없다(확인 7번) |
| §2.1 회전·뒤집기 | 90도씩 돌릴 수 있고 뒤집기는 없다 | RankFormationMask.h:25-26; RankFormationMask.cpp:27-37 | (행, 열) → (열, −행)으로 돌린 뒤 왼쪽 위로 옮긴다(RankFormationMask.cpp:29-36). 좌우 반전 함수는 없다 |
| §2.1 겹침 금지 | 진형판 안에, 다른 분대와 겹치지 않게 놓는다 | RankFormationMask.cpp:39-60, 62-70, 72-88 | 행마다 비트 6개로 보는 판정(`CanPlace`)과 칸 단위 대조군(`CanPlaceCellwise`, T4용)이 함께 있다 |
| §2.1 밤마다 다시 놓기 | 밤마다 모든 분대를 다시 놓을 수 있다 | 코드 없음 | 런 진행(`RankGame`)의 일이라 RankCore에 없다 |
| §2.2 링크 생성 | 상하좌우로 맞닿은 두 병사의 병과가 다르면 링크, 서로 상대 병과의 보너스를 한 겹씩. 같은 병과·대각선은 없음 | RankLinks.cpp:36-82, 47, 51-54, 55-59 | 47은 이웃 네 방향, 51-54는 빈칸·같은 병과 제외, 55-59는 효과 없는 병과 제외. 받는 칸의 병과는 보지 않는다(확인 1번) |
| §2.2 겹 상한(병과마다) | 종류마다 세 겹까지. 같은 종류를 두 병과 이상이 주면 병과마다 세 겹 | RankLinks.cpp:62-79, 102; RankTypes.h:126-145 | 11/20 작업 트리 변경분. 주는 병과는 키로 구분한다(RankLinks.cpp:66) |
| §2.2 사거리 상한·간격 하한 | 사거리는 4까지, 간격은 4틱까지 | RankLinks.cpp:110, 112 | |
| §2.2 값의 출처 | 위 세 값과 한 겹의 크기는 §7 매개변수 | RankTypes.h:75, 85-87 | 코드에 값이 없다(구조체 기본값 0) |
| §2.2 진영 무관 | 링크는 병과로 정하고 진영은 보지 않는다 | RankLinks.cpp:46; RankSimulate.cpp:94-108 | 두 진영에 같은 함수를 부른다. 두 진영의 진형판 사이는 넘지 않는다(확인 2번) |
| §2.2 시작 순간에 고정 | 전투 중 옆 병사가 쓰러져도 받은 보너스는 그대로 | RankSimulate.cpp:108, 114-118 | 능력치 대입은 유닛 생성 때뿐이다(검색: `Atk`·`Armor`·`Range`·`Interval` 대입은 RankSimulate.cpp:115-118 밖에 0건) |
| §2.3 대열 | 빈칸이 사라지고 앞으로 붙어 한 줄이 된다 | RankSimulate.cpp:102-105, 125 | 빈칸을 건너뛰고 행 순서로 자기 열 대열 뒤에 붙인다 |
| §2.3 전진 | 앞사람이 쓰러지면 한 칸씩 앞으로, 대기 시간은 그대로 | RankSimulate.cpp:356-358 | 빼낸 뒤 `Cd`를 건드리지 않는다. `RemoveAt`이 남은 순서를 지킨다는 것은 코드 주석(357)의 설명이다([미확인], 4절) |
| §2.3 빈칸은 링크에만 | 빈칸은 링크 계산에만 의미가 있다 | RankLinks.cpp:7-14, 50-54 | 링크는 대열이 아니라 진형판 칸(`FFormation`)으로 계산한다 |
| §2.4 행동 | 간격마다 공격. 대열 i번째는 i < 사거리일 때만, 아니면 준비된 채 대기 | RankSimulate.cpp:252-265, 260 | |
| §2.5 표적 | 근접은 맨 앞, 척후는 맨 뒤, 석궁수는 지원, 마법사는 범위, 대상이 없으면 상대 성문 | RankSimulate.cpp:163-238, 121, 157; RankTypes.h:22-29 | 병과 이름이 아니라 병과 수치의 `target` 값으로 고른다(121). 157은 상대 성문 |
| §2.6 피해 식 | 물리 max(1, 공격 − 방어), 마법은 공격 그대로, 성문·결계는 1 | RankSimulate.cpp:132-140, 158 | 성문 피해는 `gateDamage`(§7)를 읽는다(확인 4번) |
| §2.6 동시 적용 | 틱 시작 상태로 표적을 고르고 피해는 모았다가 한꺼번에 | RankSimulate.cpp:241-269, 288-318 | 2절 결정론 점검표 6번 |
| §2.7 승패 | 패배: 아군 전멸, 성문 0 이하. 승리: 섬멸, 결계 0 이하, 600틱 | RankSimulate.cpp:366-380, 403-404, 417-424 | 600은 `maxTicks`를 읽는다(확인 4번) |
| §2.7 같은 틱에 함께 끝남 | 성문이 남아 있으면 승리 | RankJudge.cpp:22-26 | |
| §2.7 시작 내구도 | 성문과 결계는 60에서 시작 | RankSimulate.cpp:394-397 | `gateHp`를 읽는다(확인 4번) |
| §2.8 해골의 링크 | 해골에게는 링크가 없다 | RankLinks.cpp:55-59 | `linkStat`이 None인 병과는 주지 않는다. 받는 쪽은 막지 않는다(확인 1번) |
| §2.8 졸개 부활 | 처음 쓰러질 때 한 번만 체력 6으로 | RankSimulate.cpp:120, 337-348 | 병과 키가 아니라 `revive` 값(0보다 큼)으로 정한다. 6은 데이터에서 온다 |

### §3 유닛 생성과 링크 계산

| 조항 | 조항 요약(한 줄) | 코드 위치(파일:줄) | 메모 |
|---|---|---|---|
| §3.1 생성 순서 | 진영 0 → 1, 행 0 → 3, 열 0 → 5. 자기 열 대열 맨 뒤에 붙는다 | RankSimulate.cpp:91-129, 94, 97, 99, 125 | |
| §3.2 유닛 번호 | 진영 × 100 + 행 × 6 + 열. 전진해도 바뀌지 않는다 | RankSimulate.cpp:111; RankTypes.h:15-17 | 식의 6은 `BoardCols`가 아니라 따로 둔 `UnitIdRowFactor`(RankTypes.h:17)다. 값은 같다. `Id` 대입은 111 한 곳 |
| §3.3 링크 보너스 | 이웃 병과가 다르고 링크 효과가 있으면 이웃 한 명마다 한 겹. 해골은 주지도 받지도 않는다. 진영은 보지 않는다 | RankLinks.cpp:36-82; RankBattle.h:10-12 | `FLinkStacks.Count`(RankTypes.h:142, RankLinks.cpp:60)는 종류별 합계이고 겹 상한 전 값이다. 최종 능력치에는 쓰지 않는다(RankLinks.cpp:93-106은 `Givers`만 본다). 받는 칸 조건은 확인 1번 |
| §3.4 최종 능력치 | 방어·공격·최대 체력은 기본 + 병과마다 a × min(n, 겹 상한)의 합. 사거리는 상한, 간격은 하한을 더한 뒤 한 번 | RankLinks.cpp:84-114, 93-106, 108-112 | 102가 병과마다 겹 상한, 110이 사거리 상한, 112가 간격 하한 |
| §3.4 최대 체력으로 시작 | 전투는 최대 체력으로 시작한다 | RankSimulate.cpp:114 | |
| §3.5 대기 시간 | 처음 값은 간격. 첫 공격은 간격 번째 틱 | RankSimulate.cpp:119, 252-259 | |
| §3.6 입력 오류 | 한쪽이 비거나 병과 수치에 없는 키가 있으면 계산하지 않는다. 틱·해시·이벤트 없음 | RankSimulate.cpp:62-88, 386-390; RankTypes.h:46, 116-123 | 병과 수치가 없거나 0개일 때도 입력 오류(RankSimulate.cpp:64-67). "없음"은 구조체 기본값(틱 0, 해시 0, 이벤트 수 0, 빈 로그)으로 나타낸다. 문서가 다루지 않는 입력은 확인 6번 |
| §3.6 배치 화면 | 병사를 놓지 않으면 전투를 시작할 수 없게 한다 | 코드 없음 | 배치 화면(`RankGame`)의 일 |

### §4 한 틱의 처리 순서

| 조항 | 조항 요약(한 줄) | 코드 위치(파일:줄) | 메모 |
|---|---|---|---|
| §4 틱 범위 | 1틱부터 최대 600틱까지, 틱마다 ①~⑤ | RankSimulate.cpp:406-425 | 최대 틱은 `maxTicks`(확인 4번) |
| §4.1 ① 의도 모으기 | 진영 0 → 1, 열 0 → 5, 대열 앞 → 뒤. 대기 시간을 줄이고, 사거리 밖이면 0에 둔 채 대기, 아니면 표적을 고르고 간격으로 되돌린다 | RankSimulate.cpp:241-269 | 243 진영, 245 열, 248 대열 순서. 252-255 줄이기, 256-259 아직 0보다 큼, 260-263 사거리 밖, 264-265 표적과 되돌리기 |
| §4.1 ② 공격이 없는 틱 | 의도가 없으면 ③~⑤를 건너뛴다 | RankSimulate.cpp:410-413 | |
| §4.1 ③ 피해 주기 | 모은 순서대로, 표적은 그대로, 이미 쓰러진 표적에도, 피해는 계산값 그대로, 0 아래로도 내려간다 | RankSimulate.cpp:288-318, 296, 303, 308, 315 | 피해 값은 ①에서 계산해 의도에 담는다(148, 158). 공격·방어는 생성 뒤 바뀌지 않으므로(대입은 115-116뿐) ③ 시점의 값과 같다 |
| §4.1 ④ 쓰러진 병사 정리 | 진영 0 → 1, 열 0 → 5, 대열 앞 → 뒤. 부활 값이 있으면 부활(자리 그대로), 없으면 빼고 당긴다. 대기 시간은 그대로 | RankSimulate.cpp:321-363 | 323·325·329 순서, 337-348 부활, 349-359 사망 |
| §4.1 ⑤ 승패 확인 | §5.1 | RankSimulate.cpp:417-424 | |
| §4.2 마주 보는 열·왼쪽 | 열 번호가 같은 열, 왼쪽은 열 번호가 작은 쪽(양쪽 같은 기준) | RankSimulate.cpp:167, 203 | |
| §4.2 맨 앞 | 마주 보는 열의 대열 맨 앞, 비었으면 상대 성문 | RankSimulate.cpp:171-180 | |
| §4.2 맨 뒤 | 마주 보는 열의 대열 맨 뒤, 비었으면 상대 성문 | RankSimulate.cpp:182-191 | |
| §4.2 지원 | 마주 보는 열의 맨 앞. 비면 거리 1~5, 같은 거리는 작은 열 먼저. 모두 비면 성문 | RankSimulate.cpp:193-216 | 195-199 마주 보는 열, 201 거리 1~5, 203 작은 열 먼저, 207 판 안 검사, 214 성문 |
| §4.2 범위 | c−1, c, c+1 순서로 판 안의 비지 않은 열의 맨 앞을 모두. 판 밖은 빈 열. 셋 다 비면 성문 한 번 | RankSimulate.cpp:218-236 | 223 순서, 225 판 안·비지 않음, 231-234 성문 한 번 |
| §4.3 피해 | 마법은 공격 그대로, 물리는 max(1, 공격 − 방어), 성문은 성문 피해 값. 범위는 표적마다 따로 | RankSimulate.cpp:132-140, 158, 223-230 | 범위 공격은 표적마다 `AddUnitIntent`를 부른다(227) |

### §5 판정

| 조항 | 조항 요약(한 줄) | 코드 위치(파일:줄) | 메모 |
|---|---|---|---|
| §5.1 진 진영 | ④ 뒤 진영마다, 성문(결계) 0 이하이거나 모든 열이 비면 진다 | RankSimulate.cpp:366-380, 415-418 | |
| §5.1 코어 결과 | 해골만 짐 A, 플레이어만 짐 B, 둘 다 Draw, 600틱까지 없으면 Timeout(틱 600) | RankSimulate.cpp:403-404, 421; RankTypes.h:44-51 | |
| §5.1 음수 결과 값 | 결과의 성문·결계·유닛 체력은 음수일 수 있고 결과 값은 바꾸지 않는다 | RankSimulate.cpp:431-446 | 성문·결계는 그대로 옮긴다(436). 유닛 체력은 확인 5번 |
| §5.2 플레이어 기준 판정 | A 승리(결계 파괴·섬멸), Timeout 승리(새벽), Draw 성문 남으면 승리(성문 사수)·아니면 패배(성문 붕괴), B 패배(성문 붕괴·전멸) | RankJudge.cpp:6-38, 14-17, 18-21, 22-26, 27-30; RankTypes.h:157-173 | 입력 오류는 31-35(확인 3번) |

### §6 이벤트와 해시

| 조항 | 조항 요약(한 줄) | 코드 위치(파일:줄) | 메모 |
|---|---|---|---|
| §6 해시 계산 | FNV-1a 32비트, 시작값 `0x811c9dc5`, 곱수 `0x01000193`, 하위 바이트부터 4바이트를 XOR하고 곱함, 넘침은 버림 | RankHash.h:9-20; RankSimulate.cpp:399 | 9 시작값, 10 곱수, 14 `int32` → `uint32`, 15-19 바이트 반복, 17 XOR, 18 곱 |
| §6 성문 타격(종류 1) | ③에서 표적이 성문일 때. 로그: 공격자, 999, 피해. 해시: 틱, 1, 공격자, 999, 피해 | RankSimulate.cpp:294-304; RankTypes.h:20, 56 | 298-302 해시, 303 로그 |
| §6 피해(종류 2) | ③에서 표적이 유닛일 때. 로그: 공격자, 표적, 피해. 해시: 틱, 2, 공격자, 표적, 피해 | RankSimulate.cpp:305-316; RankTypes.h:57 | 310-314 해시, 315 로그 |
| §6 부활(종류 3) | ④에서 졸개가 일어설 때. 로그: 번호, 번호, 부활한 뒤 체력. 해시: 틱, 3, 번호 | RankSimulate.cpp:337-348; RankTypes.h:58 | 343-345 해시, 346 로그 |
| §6 사망(종류 4) | ④에서 대열에서 뺄 때. 로그: 번호, 번호, 0. 해시: 틱, 4, 번호 | RankSimulate.cpp:349-359; RankTypes.h:59 | 352-354 해시, 355 로그 |
| §6 종료 | 끝날 때 한 번. 해시: 마지막 틱, 결과 코드(A 1, B 2, Draw 3, Timeout 4). 로그·이벤트 수에 넣지 않음 | RankSimulate.cpp:427-429; RankTypes.h:44-51 | 입력 오류는 389에서 돌아가므로 종료도 섞지 않는다 |
| §6 로그 한 줄 | 틱, 종류, 행위자, 표적, 값 | RankTypes.h:104-112; RankSimulate.cpp:271-284 | |
| §6 이벤트 수 | 종류 1~4인 로그 줄의 수 | RankSimulate.cpp:273; RankTypes.h:119 | 로그를 기록하지 않아도 센다(273은 274의 조건 밖). 해시 섞기도 `Emit` 밖이라 `bRecordLog`와 상관없다 |
| §6 해시에는 마지막 칸 값만 | 부활한 뒤 체력처럼 로그에만 있는 값은 해시에 넣지 않는다 | RankSimulate.cpp:343-346 | |

### §7 매개변수

RankCore는 병과 수치 배열(`FUnitDef`)과 규칙 매개변수(`FRuleParams`)를 입력으로 받는다(RankTypes.h:96-102). 아래 "코드 위치"는 RankCore 안에서 그 값을 읽는 곳이다.

| 조항 | 조항 요약(한 줄) | 코드 위치(파일:줄) | 메모 |
|---|---|---|---|
| §7 값의 기준 | 값은 `Data/defs/units.csv`, `rules.csv`에서 온다 | RankTypes.h:62-88, 96-102 | CSV를 이 구조체로 옮기는 코드는 `Source/RankGame`, `Source/RankEditor`, `Source/RankTests`에서 찾지 못했다(대소문자 무시 검색어 `FRuleParams`, `IntervalMin`, `LinkStackMax`, `rules.csv`, `units.csv`: 코드 0건. 걸린 4건은 `AGENTS.md` 3건과 `regression_branches.csv` 1건의 설명 문장). [미확인] 4절 |
| §7 키(key) | 병과 키 | RankTypes.h:65; RankLinks.cpp:20-34 | |
| §7 체력(hp) | 기본 체력 | RankTypes.h:66; RankLinks.cpp:111; RankSimulate.cpp:114 | |
| §7 공격(atk) | 기본 공격 | RankTypes.h:67; RankLinks.cpp:109; RankSimulate.cpp:115, 136, 138 | |
| §7 방어(armor) | 기본 방어 | RankTypes.h:68; RankLinks.cpp:108; RankSimulate.cpp:116, 138 | |
| §7 간격(interval) | 기본 간격 | RankTypes.h:69; RankLinks.cpp:112; RankSimulate.cpp:117, 119, 265 | |
| §7 사거리(range) | 기본 사거리 | RankTypes.h:70; RankLinks.cpp:110; RankSimulate.cpp:118, 260 | |
| §7 표적(target) | 맨 앞, 맨 뒤, 지원, 범위 | RankTypes.h:71, 22-29; RankSimulate.cpp:121, 169 | |
| §7 마법(magic) | "예"면 방어를 무시 | RankTypes.h:72; RankSimulate.cpp:122, 134 | |
| §7 부활(revive) | 처음 쓰러질 때 일어서는 체력 | RankTypes.h:73; RankSimulate.cpp:120, 337-341 | |
| §7 링크로 주는 것: 종류(linkStat) | 방어, 공격, 사거리, 최대 체력, 간격, 없음 | RankTypes.h:74, 31-41; RankLinks.cpp:56, 60, 75 | |
| §7 링크로 주는 것: 크기(linkAmount) | 한 겹의 크기 | RankTypes.h:75; RankLinks.cpp:76, 102 | |
| §7 최대 틱(maxTicks) | 600 | RankTypes.h:82; RankSimulate.cpp:404, 406 | |
| §7 성문과 결계(gateHp) | 60 | RankTypes.h:83; RankSimulate.cpp:396 | |
| §7 성문 피해(gateDamage) | 1 | RankTypes.h:84; RankSimulate.cpp:158 | |
| §7 링크 겹 상한(linkStackMax) | 3 | RankTypes.h:85; RankLinks.cpp:102 | |
| §7 사거리 상한(rangeMax) | 4 | RankTypes.h:86; RankLinks.cpp:110 | |
| §7 간격 하한(intervalMin) | 4 | RankTypes.h:87; RankLinks.cpp:112 | |
| §7 진형판 크기 | 6열 × 4행은 매개변수가 아니라 규칙이 고정한 값 | RankTypes.h:9-12, 78-79 | `Data/defs/rules.csv:1`의 열은 `maxTicks`, `gateHp`, `gateDamage`, `linkStackMax`, `rangeMax`, `intervalMin` 여섯이고 판 크기는 없다 |
| §7 1틱 = 0.1초 | 매개변수가 아니라 규칙이 고정한 값 | 코드 없음 | RankCore는 시간을 쓰지 않고 틱만 센다. 초로 바꾸는 일은 재생(`RankGame`) 쪽이다 |

### §8 검산 예제

| 조항 | 조항 요약(한 줄) | 코드 위치(파일:줄) | 메모 |
|---|---|---|---|
| §8 검산 예제 1~3 | 손계산 시나리오(T5)의 출발점 | 코드 없음 | 예제라 따로 구현이 없다. 해당 계산은 §3.4, §4 행의 코드가 한다. 이 문서에서는 예제 값을 계산하지 않았다 |

### §9 모호한 곳의 답

| 조항 | 조항 요약(한 줄) | 코드 위치(파일:줄) | 메모 |
|---|---|---|---|
| §9 1번 | 같은 열 번호가 마주 보고, 왼쪽은 양쪽 모두 번호가 작은 쪽 | RankSimulate.cpp:167, 203 | |
| §9 2번 | 그 틱에 이미 쓰러진 표적에도 피해가 들어간다 | RankSimulate.cpp:305-308 | 적용 전에 표적의 체력을 보지 않는다 |
| §9 3번 | 부활한 졸개는 자리와 그 틱의 대기 시간을 유지 | RankSimulate.cpp:337-348 | `Cd`를 건드리지 않고 다음 순번으로 넘어간다(347) |
| §9 4번 | 의도가 없는 틱은 판정도 건너뛴다 | RankSimulate.cpp:410-413 | |
| §9 5번 | 범위는 셋 다 비었을 때만 성문을 친다 | RankSimulate.cpp:222-234 | |
| §9 6번 | 해골 진형도 행 0이 최전열, 데이터 첫 줄이 행 0 | RankTypes.h:90; RankSimulate.cpp:94-125 | 코어는 두 진영을 같은 행 순서로 읽는다. 데이터 첫 줄을 행 0으로 옮기는 코드는 RankCore 밖이라 보지 않았다 |
| §9 7번 | 앞으로 당겨져도 대기 시간은 그대로 | RankSimulate.cpp:356-358 | |
| §9 8번 | 진형판 밖의 열은 빈 열 | RankSimulate.cpp:40-43, 225 | |
| §9 9번 | 한쪽이 비었거나 모르는 키면 입력 오류. 배치 화면은 시작을 막는다 | RankSimulate.cpp:62-88, 386-390 | 배치 화면 쪽은 코드 없음(`RankGame`) |
| §9 10번 | 뒤집기는 없다 | RankFormationMask.h:25-26; RankFormationMask.cpp:27-37 | |
| §9 11번 | 피해 기록은 계산한 값 그대로 | RankSimulate.cpp:303, 315 | 의도에 담긴 피해를 줄이지 않고 기록한다 |
| §9 12번 | 체력과 성문은 0 아래로, 결과 값도 음수 그대로 | RankSimulate.cpp:296, 308, 436 | 결과의 유닛 체력은 확인 5번 |
| §9 13번 | 종료는 로그와 이벤트 수에 넣지 않고 해시에만 | RankSimulate.cpp:427-429 | `Emit`을 부르지 않는다 |
| §9 14번 | 한 겹의 크기, 겹 상한, 사거리 상한, 간격 하한은 데이터 | RankTypes.h:75, 85-87; RankLinks.cpp:102, 110, 112 | |
| §9 15번 | 링크는 병과로 정하고 진영은 보지 않는다 | RankLinks.cpp:46; RankSimulate.cpp:94-108 | |
| §9 16번 | 부활 로그는 번호, 번호, 부활한 뒤 체력. 사망 로그는 번호, 번호, 0 | RankSimulate.cpp:346, 355 | |
| §9 17번 | 같은 종류를 두 병과 이상이 주면 병과마다 따로 센다. 상한·하한은 더한 뒤 | RankLinks.cpp:62-79, 93-112; RankSimulate.cpp:61 | 입력 오류로 막지 않는다(`ValidateInput`에 그런 검사가 없다, RankSimulate.cpp:62-88) |
| §9 18번 | 진형판 크기는 규칙 상수, `rules.csv`에 두지 않는다 | RankTypes.h:9-12, 78-79; RankSimulate.cpp:60 | `Data/defs/rules.csv:1`에 판 크기 열이 없다 |

## 2. 결정론 점검표

`Source/RankCore/AGENTS.md`의 여덟 항목이다. 검색은 위 10개 파일만 대상으로 했다(뺀 4개 파일은 대상이 아니다). "관찰"은 읽고 검색한 결과이고, 코어 전체가 결정론적이라는 판정이 아니다.

| # | 항목 | 확인한 근거 | 파일:줄 | 관찰 |
|---|---|---|---|---|
| 1 | `float`, `double`이 없다. 비율·평균·보간도 계산하지 않는다 | `\bfloat\b`, `\bdouble\b` 검색: 각 1건, 모두 주석. 주석을 뺀 코드에서 나눗셈 `/`과 나머지 `%`: 0건 | RankTypes.h:6(주석) | 어긴 곳을 찾지 못함 |
| 2 | `TMap`, `TSet`을 순회하지 않는다 | `TMap`, `TSet`, `TSortedMap` 검색: 0건. 쓰는 컨테이너는 `TArray`와 고정 크기 배열이고, 순회는 모두 인덱스 반복이다 | RankSimulate.cpp:35-36, 243-248, 323-329; RankTypes.h:123 | 어긴 곳을 찾지 못함 |
| 3 | 포인터 주소로 비교·정렬하지 않는다 | `Sort`, `Algo` 검색: 0건. 포인터 비교는 `nullptr`과의 비교뿐이다. 병과 찾기는 배열 앞에서부터 처음 맞는 것 | RankSimulate.cpp:64, 80; RankLinks.cpp:22, 26-32, 56, 87 | 어긴 곳을 찾지 못함 |
| 4 | 초기화하지 않은 멤버가 없다 | 구조체 멤버를 하나씩 봤다: 모두 기본값이 있다. `FShape::Cells`에는 `= {}`가 없지만 원소 `FShapeCell`의 멤버에 기본값이 있다. `FBattleInput::Side`는 `FFormation`의 기본값을 쓴다. [판단] 근거: 기본 멤버 초기화자가 있는 클래스의 배열 원소는 기본 생성 때 그 값으로 초기화된다 | RankTypes.h:63-76, 80-88, 91-94, 96-102, 105-112, 114-124, 130-136, 140-145, 148-155, 169-173; RankFormationMask.h:10-20, 29-31; RankSimulate.cpp:8-22, 25-31, 33-38 | 어긴 곳을 찾지 못함 |
| 5 | 전역·정적 변수에 상태를 두지 않는다 | `static` 검색(`static_cast` 제외): 0건. 이름공간 범위의 변수는 `inline constexpr` 상수뿐이다. 전투 상태는 `Simulate` 안의 지역 변수다. 모듈 등록 매크로 `IMPLEMENT_MODULE`이 펼쳐지는 내용은 보지 않았다([미확인], 4절) | RankTypes.h:11-13, 16-17, 20, 127; RankHash.h:9-10; RankFormationMask.h:8; RankSimulate.cpp:385, 392, 399-404; RankCoreModule.cpp:3 | 어긴 곳을 찾지 못함(매크로 안은 미확인) |
| 6 | 한 틱 안의 표적 선택은 틱 시작 상태만 본다 | ①에서 쓰는 값은 `Cd`뿐이다. 표적 고르기는 대열과 공격·방어·열·표적 종류만 읽는다. 체력·성문·대열 쓰기는 ③과 ④에만 있다. 공격·방어 대입은 생성 때뿐 | RankSimulate.cpp:254, 265(① 쓰기); 163-238(표적); 296, 308(③); 340-341, 358(④); 115-116(생성) | 어긴 곳을 찾지 못함 |
| 7 | 엔진 난수 함수를 부르지 않는다. `Simulate` 안에 난수가 없다. 정수 난수기는 `Simulate` 밖에서만 | `Rand`, `FRandom`, `FMath` 검색: 0건. 정수 난수기는 뺀 파일(`RankRandom.*`)에 있을 것으로 보여 "밖에서만 쓴다"는 확인하지 못했다([추측] 파일 이름으로 짐작) | (해당 줄 없음) | 다룬 파일에서는 어긴 곳을 찾지 못함. 셋째 문장은 [미확인] |
| 8 | `UObject`, `UWorld`, 시각, 프레임 시간을 쓰지 않는다. `USTRUCT`·`UCLASS`·`UENUM`이 없다 | `UObject`(주석 1건 제외), `UWorld`, `FDateTime`, `FPlatformTime`, `GetTime`, `Seconds`, `Delta`, `GFrame`, `USTRUCT`, `UCLASS`, `UENUM`, `UPROPERTY` 검색: 0건. 의존 모듈은 `Core`뿐. include는 `CoreMinimal.h`와 `Modules/ModuleManager.h`뿐 | RankCore.Build.cs:9; RankTypes.h:3; RankHash.h:3; RankCoreModule.cpp:1 | 어긴 곳을 찾지 못함 |

## 3. 확인이 필요한 곳

코드가 문장과 달라 보이거나, 문장이 코드에 모호한 곳이다. 어느 쪽이 맞는지는 정하지 않았다. 일곱 건이다.

1. **§2.8, §3.3: 해골이 링크 보너스를 받는가.** RankLinks.cpp:39, 50-60.
   - 문서: §2.8 "해골에게는 링크가 없다"(rules.md:106), §3.3 "해골은 링크 효과가 없으므로 링크를 주지도 받지도 않는다"(rules.md:116).
   - 코드: 이웃(주는 쪽)의 `linkStat`만 보고(RankLinks.cpp:55-59), 받는 칸(`Self`, 39)의 병과는 보지 않는다. 한 진형판에서 해골 병과가 아군 병과와 맞닿으면 해골 칸도 아군 병과의 보너스를 받는다.
   - 참고: §3.3 첫 문장(rules.md:116)과 `Source/RankCore/AGENTS.md:29`는 주는 쪽 조건만 적어 코드와 같다. 한 조항 안의 두 문장이 섞인 진형판에서 다른 답을 낸다.
   - [미확인] 지금 입력에서 섞인 진형판이 생기는지: 해골 진형 데이터와 무작위 배치 생성기(작업 중이라 보지 않음)를 봐야 한다.
   - 섞인 진형판이 입력으로 올 수 있는지부터 정하면 어느 문장을 따를지 좁혀진다.

2. **§2.2, §3.3: 두 진영의 진형판이 맞닿는 선을 넘어 링크가 생기는가.** RankLinks.cpp:7-14, 36.
   - 문서: "위·아래·왼쪽·오른쪽으로 맞닿은 두 병사"(rules.md:70), "한 칸의 위·아래·왼쪽·오른쪽 이웃"(rules.md:116). 그림(rules.md:38)은 두 진영의 행 0이 한 선에서 맞닿아 있다. "같은 진형판 안"이라는 말은 없다.
   - 코드: `ComputeLinks`는 진형 하나(`FFormation`)만 받고(36), 판 밖은 빈칸으로 본다(7-14). 진영 0 행 0과 진영 1 행 0 사이에는 링크가 없다.
   - [판단] §1이 진형판을 "진영마다 하나씩"(rules.md:55)으로 정의하므로 코드처럼 읽는 것이 자연스럽다. 다만 미러 테스트(T3)처럼 양쪽 행 0에 아군 병과가 서면 두 읽기의 결과가 갈린다.

3. **§5.2: 입력 오류(Invalid)의 판정.** RankJudge.cpp:31-35.
   - 문서: §5.2 표(rules.md:188-193)에는 A, Timeout, Draw, B 네 줄뿐이다. §3.6(rules.md:136)은 "입력 오류"를 돌려준다고만 적는다.
   - 코드: 입력 오류에 `bWin = false`, 종류 `None`을 돌려준다. `bWin = false`는 문서에 없는 선택이다. 게임이 이것을 패배(3패에 셈)로 다루는지는 `RankGame` 쪽이라 보지 않았다.

4. **§1, §2.6, §2.7, §4, §5.1: 지금 값을 고정값처럼 적은 문장과 매개변수.** RankSimulate.cpp:158, 396, 404, 406.
   - 문서: §2.6 "성문과 결계에 들어가는 피해는 공격과 상관없이 1"(rules.md:97), §2.7 "600틱(60초)"(rules.md:102)과 "60에서 시작"(rules.md:104), §5.1 "끝난 틱은 600"(rules.md:182), §1 "둘 다 60에서 시작"(rules.md:64), §4 "최대 600틱"(rules.md:140). 같은 값을 §4.3(rules.md:171)과 §7(rules.md:230)은 매개변수(성문 피해, 최대 틱, 성문과 결계)로 적는다.
   - 코드: 매개변수를 읽는다. 지금 `Data/defs/rules.csv:2`의 값(600, 60, 1)이 문장과 같아 결과는 같다. `rules.csv` 값을 바꾸면 문장과 코드가 달라진다. §2.2는 "§7의 매개변수"라고 적어 두었다(rules.md:73).

5. **§5.1: 결과에 적는 "유닛 체력".** RankTypes.h:114-124; RankSimulate.cpp:431-446.
   - 문서: "결과에 적는 성문과 결계 값, 유닛 체력은 음수일 수 있다"(rules.md:184).
   - 코드: 결과에 유닛별 체력이 없다. 진영별 생존 수(`Alive`)와 대열에 남은 유닛의 체력 합(`HpSum`)만 있다(RankSimulate.cpp:437-445). ④ 뒤 대열에 남은 유닛은 체력이 0보다 크므로(332-359) `HpSum`은 음수가 되지 않는다. 성문·결계는 음수 그대로다(436).
   - 문장의 "유닛 체력"이 무엇을 가리키는지(쓰러진 유닛을 넣는지) 정해져 있지 않다. `Alive`·`HpSum`의 정의는 `docs/rules.md`에 없고 `docs/design.md:167`에 이름만 있다.

6. **§3.6: 문서가 다루지 않는 입력.** RankSimulate.cpp:62-88.
   - 문서: §3.6(rules.md:136)은 빈 진영과 모르는 병과 키만 입력 오류로 정한다.
   - 코드가 검사하지 않고 그대로 계산하는 경우:
     - `maxTicks`가 0 이하: 틱을 하나도 돌지 않고 Timeout, 틱은 `maxTicks` 값(RankSimulate.cpp:403-406).
     - `gateHp`가 0 이하: 의도가 처음 생긴 틱의 ⑤에서 양쪽이 모두 져 Draw(RankSimulate.cpp:368, 421).
     - 병과 수치에 같은 키가 둘 이상: 배열 앞의 것을 쓴다(RankLinks.cpp:26-32).
     - `linkStat`이 `ELinkStat` 범위 밖(`Count` 이상): 크기 6인 `Count` 배열(RankTypes.h:142) 밖에 쓴다(RankLinks.cpp:60).
   - [미확인] 관문 1(`node tools/validate_content.js`)이나 CSV 변환 코드가 이런 값을 막는지 보지 않았다.

7. **§2.1: 분대 모양(1~4칸, 변끼리 붙음)을 코드가 보장하는가.** RankFormationMask.h:16-20; RankFormationMask.cpp:5-88.
   - 문서: "분대는 1~4칸이 변끼리 붙은 모양"(rules.md:68).
   - 코드: `FShape`에는 칸 자리 4개만 있고, `NormalizeShape`·`RotateShape90`·`CanPlace`·`Place`·`CanPlaceCellwise`는 `NumCells`가 1~4인지, 칸이 변끼리 붙었는지 보지 않는다. `NumCells`가 4보다 크면 `Cells` 배열 밖을 읽는다.
   - [미확인] 모양을 어디서 검사하는지: 관문 1과 `RankPlacement`(작업 중)를 봐야 한다.

## 4. 확인하지 못한 것

- 뺀 파일 4개(`RankRandom.h`, `RankRandom.cpp`, `RankPlacement.h`, `RankPlacement.cpp`): 작업이 끝나면 같은 방식으로 대조표에 더해야 한다. 결정론 점검표 7번의 셋째 문장도 그때 확인한다.
- [미확인] `TArray::RemoveAt`이 남은 원소의 순서를 지키는지(§2.3, §4.1 ④가 기대는 동작): 코드 주석(RankSimulate.cpp:357)만 있다. 확인 방법: 5.8 엔진 소스 `Engine/Source/Runtime/Core/Public/Containers/Array.h`의 `RemoveAt` 구현을 연다. 이 환경에는 `UE_ROOT`가 없어 열지 못했다.
- [미확인] `IMPLEMENT_MODULE`이 펼쳐지는 내용(결정론 점검표 5번): 확인 방법: 엔진 소스 `Engine/Source/Runtime/Core/Public/Modules/ModuleManager.h`의 매크로 정의를 연다.
- [미확인] CSV를 `FUnitDef`·`FRuleParams`로 옮기는 코드의 위치(§7): `Source/RankGame`, `Source/RankEditor`, `Source/RankTests`에서 찾지 못했다. 아직 없거나 다른 이름을 쓴다. 확인 방법: 임포터 작업이 끝난 뒤 다시 검색한다.
- 컴파일과 테스트는 돌리지 않았다. 위 동작 설명은 모두 소스를 읽은 것이다.

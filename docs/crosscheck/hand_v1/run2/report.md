# 손계산 시나리오 교차 검증 보고서: docs/crosscheck/hand_v1/run2

`tools/hand_crosscheck.js compare --report`가 만든 파일이다. 손으로 고치지 않는다. 판정은 맨 아래 "판정" 칸에 사람이 적는다.

- 검증자: Antigravity · Gemini 4 (XHigh) · 2026-11-21 · 방법 program
- 입력: inputsId dda68def6c71e0ba · 규칙 문서 SHA-256 bfd41f9ad7516d1995cd286328006a4930f7089a14a27190c0f44e42d4bfc8ab
- 일치 19/19 (h01, h02, h03, h04, h05, h06, h07, h08, h09, h10, h11, h12, h13, h14, h15, h16, h17, h18, h19)
- 불일치 0
- 모호 표시 0
- 허용 목록 밖 파일 0
- 검증자가 연 파일: `docs/crosscheck/hand_v1/INSTRUCTIONS.md`, `docs/AGENTS.md`, `docs/rules.md`, `docs/crosscheck/hand_v1/inputs.json`
- 검증자가 실행한 명령: `node docs/crosscheck/hand_v1/run2/solve.js`, `node tools/hand_crosscheck.js validate docs/crosscheck/hand_v1/run2/result.json`
- 검증자 메모: docs/rules.md만을 근거로 solve.js를 새로 작성해 19건 전부를 계산했다. inputs.json의 defs는 docs/rules.md §7과 전부 일치했다. 수동 대조 검증 시나리오: h03(척후·마법사 링크, 일부 열만 빈 범위 공격의 결계 미타격, 맨 뒤 표적 및 부활), h04(링크 겹 상한 3, 열 우선 공격 순서, 동일 틱 헛방·음수 체력·부활), h06(대기시간 0 유지 후 전진 직후 다음 틱 즉시 공격·부활), h09(범위 3열 동시 마법 공격, 방어 무시, 지원 표적), h12(맨 뒤 표적, 동시 사망 Draw 및 성문 사수), h14(빈 행 당김, 음수 성문 붕괴), h19(동일 병과 무링크, 지원 표적, 동일 틱 음수 체력) 7건을 규칙 문서와 틱 단위로 직접 대조해 일치함을 확인했다.

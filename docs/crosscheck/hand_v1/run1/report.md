# 손계산 시나리오 교차 검증 보고서: docs/crosscheck/hand_v1/run1

`tools/hand_crosscheck.js compare --report`가 만든 파일이다. 손으로 고치지 않는다. 판정은 맨 아래 "판정" 칸에 사람이 적는다.

- 검증자: Antigravity · Gemini 4 Neon (XHigh) · 2026-11-20 · 방법 program
- 입력: inputsId db533d4dd53510d6 · 규칙 문서 SHA-256 00aac19e8f8ad7d7622461a102465256cfd3839528ea4fd4d736d09af2f5c758
- 일치 18/18 (h01, h02, h03, h04, h05, h06, h07, h08, h09, h10, h11, h12, h13, h14, h15, h16, h17, h18)
- 불일치 0
- 모호 표시 0
- 허용 목록 밖 파일 0
- 검증자가 연 파일: `docs/crosscheck/hand_v1/INSTRUCTIONS.md`, `docs/AGENTS.md`, `docs/rules.md`, `docs/crosscheck/hand_v1/inputs.json`
- 검증자가 실행한 명령: `node docs/crosscheck/hand_v1/run1/solve.js`, `node tools/hand_crosscheck.js validate docs/crosscheck/hand_v1/run1/result.json`
- 검증자 메모: 루트 AGENTS.md는 작업공간 규칙으로 자동 로드됨. inputs.json의 defs.units와 defs.rules는 docs/rules.md §7의 표·매개변수와 전부 일치함. docs/rules.md만 보고 docs/crosscheck/hand_v1/run1/solve.js를 새로 작성해 18건을 계산했고, 프로그램 출력 로그 중 h01, h02, h03, h04, h05, h06, h07, h08, h09, h10, h12를 docs/rules.md와 한 틱씩 직접 대조 검산했으며, h11, h13, h14, h15는 주기·경계 틱을 직접 검산하고 h16~h18은 §3.6을 직접 대조함.

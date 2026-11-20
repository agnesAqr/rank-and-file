#!/usr/bin/env node
'use strict';

// 손계산 시나리오(T5) 교차 검증 도구.
// 절차는 docs/crosscheck/README.md, 검증자에게 주는 지시는 docs/crosscheck/hand_v1/INSTRUCTIONS.md다.
//
//   node tools/hand_crosscheck.js export [--check]             눈가림 입력(inputs.json)을 만든다
//   node tools/hand_crosscheck.js validate <결과 파일>          검증자 결과의 형식만 본다. 기대값 파일을 읽지 않는다
//   node tools/hand_crosscheck.js compare <결과 파일> [--report] 기대값과 항목마다 비교한다
//   node tools/hand_crosscheck.js apply <결과 파일>             전 항목이 일치한 시나리오의 확인 상태를 적는다
//
// - 전투 규칙을 해석하는 코드를 넣지 않는다. 형식 검사와 값 비교만 한다.
// - 표준 출력에는 개수, 시나리오 id, 어긋난 항목 이름만 낸다. 값은 --report가 쓰는 사람용 보고서에만 적는다.
// - 종료 코드: 0 통과, 1 검사에서 걸림(형식 오류, 불일치, 모호 표시, 빠짐, 허용 목록 밖 파일), 2 사용법이나 파일 문제.
// - 내장 모듈만 쓴다.

const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

const HAND_REL = 'Source/RankTests/Fixtures/hand_v1.json';
const RULES_REL = 'docs/rules.md';
const DIR_REL = 'docs/crosscheck/hand_v1';
const INPUTS_REL = `${DIR_REL}/inputs.json`;
const INSTRUCTIONS_REL = `${DIR_REL}/INSTRUCTIONS.md`;

// 결과 파일에 쓰는 낱말. hand_v1.json의 expected와 같은 낱말이다.
const OUTCOMES = ['A', 'B', 'Draw', 'Timeout', 'Invalid'];
const VERDICTS = ['섬멸', '결계 파괴', '새벽', '성문 사수', '성문 붕괴', '전멸', '없음'];
const METHODS = ['manual', 'program'];
const UNIT_STATS = ['atk', 'armor', 'range', 'maxHp', 'interval'];
const RESULT_FIELDS = ['outcome', 'verdict', 'endTick', 'gate', 'barrier'];
const RESULT_INTS = ['endTick', 'gate', 'barrier'];
const UNCONFIRMED = '확인 전';

// 검증자가 열어도 되는 파일. 실행 폴더 안의 파일은 따로 허용한다.
const ALLOWED_FILES = [RULES_REL, INPUTS_REL, INSTRUCTIONS_REL, 'AGENTS.md', 'docs/AGENTS.md'];

class UsageError extends Error {}

const isObj = (v) => v !== null && typeof v === 'object' && !Array.isArray(v);
const isStr = (v) => typeof v === 'string';
const isStrArray = (v) => Array.isArray(v) && v.every(isStr);
const isInt = (v) => Number.isInteger(v);

function readText(file) {
  let text;
  try {
    text = fs.readFileSync(file, 'utf8');
  } catch (error) {
    throw new UsageError(`파일을 읽지 못했다: ${file} (${error.code || error.message})`);
  }
  return text.charCodeAt(0) === 0xfeff ? text.slice(1) : text;
}

function readJson(file) {
  const text = readText(file);
  try {
    return JSON.parse(text);
  } catch (error) {
    throw new UsageError(`JSON으로 읽히지 않는다: ${file} (${error.message})`);
  }
}

function sha256(text) {
  return crypto.createHash('sha256').update(text, 'utf8').digest('hex');
}

function toRel(root, file) {
  return path.relative(root, file).split(path.sep).join('/');
}

// hand_v1.json과 rules.md에서 눈가림 입력을 만든다. 기대값, 계산 과정, 목적, 조항 번호, 갈래는 옮기지 않는다.
function buildInputs(root) {
  const hand = readJson(path.join(root, HAND_REL));
  if (!isObj(hand) || !isObj(hand.defs) || !Array.isArray(hand.scenarios) || hand.scenarios.length === 0) {
    throw new UsageError(`${HAND_REL}: defs나 scenarios가 없다`);
  }
  const ids = new Set();
  const scenarios = hand.scenarios.map((scenario, index) => {
    if (!isObj(scenario) || !isStr(scenario.id) || !isStrArray(scenario.ally) || !isStrArray(scenario.enemy)) {
      throw new UsageError(`${HAND_REL}: scenarios[${index}]에 id, ally, enemy가 없다`);
    }
    if (ids.has(scenario.id)) {
      throw new UsageError(`${HAND_REL}: id가 겹친다(${scenario.id})`);
    }
    ids.add(scenario.id);
    return { id: scenario.id, ally: scenario.ally, enemy: scenario.enemy };
  });
  // 줄바꿈(CRLF, LF)이 달라도 같은 값이 나오게 한다.
  const rulesSha256 = sha256(readText(path.join(root, RULES_REL)).replace(/\r\n/g, '\n'));
  const inputsId = sha256(JSON.stringify({ rulesSha256, defs: hand.defs, scenarios })).slice(0, 16);
  return {
    hand,
    inputs: { inputsId, rulesDoc: RULES_REL, rulesSha256, defs: hand.defs, scenarios },
  };
}

function serialize(value) {
  return `${JSON.stringify(value, null, 2)}\n`;
}

// ---------- 형식 검사 (기대값을 읽지 않는다) ----------

function collectStrings(value, out) {
  if (isStr(value)) {
    out.push(value);
  } else if (Array.isArray(value)) {
    value.forEach((item) => collectStrings(item, out));
  } else if (isObj(value)) {
    Object.values(value).forEach((item) => collectStrings(item, out));
  }
}

function validateBattleResult(result, where, err) {
  if (!isObj(result)) {
    err(`${where}: 객체여야 한다`);
    return;
  }
  if (!OUTCOMES.includes(result.outcome)) {
    err(`${where}.outcome: ${OUTCOMES.join(', ')} 가운데 하나여야 한다`);
  }
  if (!VERDICTS.includes(result.verdict)) {
    err(`${where}.verdict: ${VERDICTS.join(', ')} 가운데 하나여야 한다`);
  }
  for (const field of RESULT_INTS) {
    if (!(result[field] === null || isInt(result[field]))) {
      err(`${where}.${field}: 정수 또는 null이어야 한다`);
    }
  }
  if (!Array.isArray(result.units)) {
    err(`${where}.units: 배열이어야 한다(전투를 계산하지 않았으면 빈 배열)`);
    return;
  }
  const unitIds = new Set();
  result.units.forEach((unit, index) => {
    const at = `${where}.units[${index}]`;
    if (!isObj(unit)) {
      err(`${at}: 객체여야 한다`);
      return;
    }
    if (!isInt(unit.id)) {
      err(`${at}.id: 정수여야 한다`);
    } else if (unitIds.has(unit.id)) {
      err(`${at}.id: 번호가 겹친다(${unit.id})`);
    } else {
      unitIds.add(unit.id);
    }
    if (!isStr(unit.key) || unit.key.length !== 1) {
      err(`${at}.key: 병과 키 한 글자여야 한다`);
    }
    for (const stat of UNIT_STATS) {
      if (!isInt(unit[stat])) {
        err(`${at}.${stat}: 정수여야 한다`);
      }
    }
    if (typeof unit.alive !== 'boolean') {
      err(`${at}.alive: true 또는 false여야 한다`);
    }
    if (!isInt(unit.hp)) {
      err(`${at}.hp: 정수여야 한다`);
    }
  });
}

function validateScenario(scenario, where, err) {
  if (scenario.status !== 'computed' && scenario.status !== 'ambiguous') {
    err(`${where}.status: computed 또는 ambiguous여야 한다`);
  }
  if (!isStrArray(scenario.steps) || scenario.steps.length === 0 || scenario.steps.some((line) => line.trim() === '')) {
    err(`${where}.steps: 비어 있지 않은 문자열의 배열이어야 한다(계산 과정)`);
  }
  const isLogLine = (line) => Array.isArray(line) && line.length === 5 && line.every(isInt);
  if (!Array.isArray(scenario.log) || !scenario.log.every(isLogLine)) {
    err(`${where}.log: 정수 5개(틱, 종류, 행위자, 표적, 값)짜리 줄의 배열이어야 한다(없으면 빈 배열)`);
  }
  if (!Array.isArray(scenario.ambiguities)) {
    err(`${where}.ambiguities: 배열이어야 한다(없으면 빈 배열)`);
  } else {
    scenario.ambiguities.forEach((item, index) => {
      const at = `${where}.ambiguities[${index}]`;
      const ok = isObj(item) && isStr(item.clause) && item.clause.trim() !== '' && isStr(item.quote) && item.quote.trim() !== ''
        && isStrArray(item.readings) && item.readings.length >= 2 && isStr(item.effect);
      if (!ok) {
        err(`${at}: clause, quote(문자열), readings(문자열 2개 이상), effect(문자열)가 있어야 한다`);
      }
    });
  }
  if (scenario.status === 'computed') {
    validateBattleResult(scenario.result, `${where}.result`, err);
  }
  if (scenario.status === 'ambiguous') {
    if (Array.isArray(scenario.ambiguities) && scenario.ambiguities.length === 0) {
      err(`${where}: status가 ambiguous면 ambiguities에 한 건 이상 적는다`);
    }
    if (scenario.result !== null) {
      err(`${where}.result: status가 ambiguous면 null이어야 한다. 읽기마다 계산한 값은 alternatives에 적는다`);
    }
    if (scenario.alternatives !== undefined) {
      if (!Array.isArray(scenario.alternatives)) {
        err(`${where}.alternatives: 배열이어야 한다`);
      } else {
        scenario.alternatives.forEach((item, index) => {
          const at = `${where}.alternatives[${index}]`;
          if (!isObj(item) || !isStr(item.reading) || item.reading.trim() === '') {
            err(`${at}.reading: 어느 읽기인지 적은 문자열이어야 한다`);
            return;
          }
          validateBattleResult(item.result, `${at}.result`, err);
        });
      }
    }
  }
}

function validateShape(result, inputs) {
  const errors = [];
  const err = (message) => errors.push(message);
  if (!isObj(result)) {
    err('결과 파일의 최상위가 객체가 아니다');
    return errors;
  }
  if (result.inputsId !== inputs.inputsId) {
    err('inputsId: inputs.json의 inputsId와 다르다. 그 값을 그대로 옮긴다');
  }
  for (const field of ['run', 'tool', 'model']) {
    if (!isStr(result[field]) || result[field].trim() === '') {
      err(`${field}: 비어 있지 않은 문자열이어야 한다`);
    }
  }
  if (!isStr(result.date) || !/^\d{4}-\d{2}-\d{2}$/.test(result.date)) {
    err('date: YYYY-MM-DD 형식이어야 한다');
  }
  if (!METHODS.includes(result.method)) {
    err(`method: ${METHODS.join(', ')} 가운데 하나여야 한다`);
  }
  if (!isStrArray(result.filesRead) || result.filesRead.length === 0) {
    err('filesRead: 연 파일을 모두 적은 문자열 배열이어야 한다(비어 있을 수 없다)');
  }
  if (!isStrArray(result.commandsRun)) {
    err('commandsRun: 문자열 배열이어야 한다(없으면 빈 배열)');
  }
  if (!isStr(result.notes)) {
    err('notes: 문자열이어야 한다(없으면 빈 문자열)');
  }

  // 공개 저장소다. 절대 경로와 사용자 폴더 이름을 적지 않는다(루트 AGENTS.md 5절).
  const strings = [];
  collectStrings(result, strings);
  if (strings.some((text) => /[\\/](Users|home)[\\/]/i.test(text) || /(^|[\s"'=(])[A-Za-z]:[\\/]/.test(text))) {
    err('절대 경로가 들어 있다. 경로는 저장소 루트 기준으로 적는다(사용자 폴더 이름을 적지 않는다)');
  }

  if (!Array.isArray(result.scenarios)) {
    err('scenarios: 배열이어야 한다');
    return errors;
  }
  const wanted = inputs.scenarios.map((scenario) => scenario.id);
  const seen = new Set();
  result.scenarios.forEach((scenario, index) => {
    if (!isObj(scenario) || !isStr(scenario.id)) {
      err(`scenarios[${index}]: id가 있는 객체여야 한다`);
      return;
    }
    if (seen.has(scenario.id)) {
      err(`${scenario.id}: id가 겹친다`);
      return;
    }
    seen.add(scenario.id);
    if (!wanted.includes(scenario.id)) {
      err(`${scenario.id}: inputs.json에 없는 id다`);
      return;
    }
    validateScenario(scenario, scenario.id, err);
  });
  for (const id of wanted) {
    if (!seen.has(id)) {
      err(`${id}: 결과에 없다`);
    }
  }
  return errors;
}

// ---------- 비교 ----------

// expected에 적힌 항목만 비교한다(T5 테스트와 같은 범위). 어긋난 항목의 이름만 돌려준다.
function diffScenario(expected, actual) {
  const diffs = [];
  for (const field of RESULT_FIELDS) {
    if (field in expected && actual[field] !== expected[field]) {
      diffs.push(field);
    }
  }
  if (Array.isArray(expected.units)) {
    for (const want of expected.units) {
      const got = actual.units.find((unit) => unit.id === want.id);
      if (!got) {
        diffs.push(`유닛 ${want.id} 없음`);
        continue;
      }
      if ('alive' in want && got.alive !== want.alive) {
        diffs.push(`유닛 ${want.id} alive`);
      }
      if ('hp' in want && got.hp !== want.hp) {
        diffs.push(`유닛 ${want.id} hp`);
      }
    }
  }
  return diffs;
}

function outsideAllowed(filesRead, runRel) {
  const norm = (file) => file.replace(/\\/g, '/').replace(/^\.\//, '').toLowerCase();
  const allowed = new Set(ALLOWED_FILES.map(norm));
  const prefix = `${norm(runRel)}/`;
  return filesRead.filter((file) => !(allowed.has(norm(file)) || norm(file).startsWith(prefix)));
}

function loadResult(root, argument) {
  if (!argument) {
    throw new UsageError('결과 파일 경로가 없다');
  }
  const candidates = [path.resolve(argument), path.resolve(root, argument)];
  const file = candidates.find((candidate) => fs.existsSync(candidate));
  if (!file) {
    throw new UsageError(`결과 파일이 없다: ${argument}`);
  }
  return { file, result: readJson(file), runRel: toRel(root, path.dirname(file)) };
}

function compareAll(root, argument) {
  const { hand, inputs } = buildInputs(root);
  const { file, result, runRel } = loadResult(root, argument);
  const formatErrors = validateShape(result, inputs);
  const rows = [];
  if (formatErrors.length === 0) {
    for (const scenario of hand.scenarios) {
      const got = result.scenarios.find((item) => item.id === scenario.id);
      const expected = isObj(scenario.expected) ? scenario.expected : {};
      let category = 'match';
      let diffs = [];
      if (got.status === 'ambiguous') {
        category = 'ambiguous';
      } else {
        diffs = diffScenario(expected, got.result);
        if (diffs.length > 0) {
          category = 'mismatch';
        } else if (got.ambiguities.length > 0) {
          category = 'ambiguous';
        }
      }
      rows.push({ id: scenario.id, category, diffs, scenario, got });
    }
  }
  const outside = formatErrors.length === 0 ? outsideAllowed(result.filesRead, runRel) : [];
  return { hand, inputs, file, result, runRel, formatErrors, rows, outside };
}

function summarize(rows) {
  const pick = (category) => rows.filter((row) => row.category === category);
  return { match: pick('match'), mismatch: pick('mismatch'), ambiguous: pick('ambiguous') };
}

function printSummary(comparison) {
  const { result, runRel, rows, outside, inputs } = comparison;
  const { match, mismatch, ambiguous } = summarize(rows);
  console.log(`교차 검증 비교: ${runRel} · 모델 ${result.model} · ${result.date} · 방법 ${result.method} · inputsId ${inputs.inputsId}`);
  console.log(`일치 ${match.length}/${rows.length}`);
  console.log(`불일치 ${mismatch.length}${mismatch.length ? `: ${mismatch.map((row) => `${row.id}(${row.diffs.join(', ')})`).join(' · ')}` : ''}`);
  console.log(`모호 표시 ${ambiguous.length}${ambiguous.length ? `: ${ambiguous.map((row) => row.id).join(', ')}` : ''}`);
  console.log(`허용 목록 밖 파일 ${outside.length}${outside.length ? `: ${outside.join(', ')}` : ''}`);
}

// ---------- 사람용 보고서 (값을 적는다) ----------

function show(value) {
  return value === undefined ? '(없음)' : JSON.stringify(value);
}

function renderResultTable(expected, actual) {
  const lines = ['| 항목 | 기존 기대값 | 검증자 | |', '|---|---|---|---|'];
  for (const field of RESULT_FIELDS) {
    if (field in expected) {
      lines.push(`| ${field} | ${show(expected[field])} | ${show(actual[field])} | ${actual[field] !== expected[field] ? '다름' : ''} |`);
    }
  }
  if (Array.isArray(expected.units)) {
    for (const want of expected.units) {
      const got = actual.units.find((unit) => unit.id === want.id);
      for (const field of ['alive', 'hp']) {
        if (field in want) {
          const value = got ? got[field] : undefined;
          lines.push(`| 유닛 ${want.id} ${field} | ${show(want[field])} | ${show(value)} | ${value !== want[field] ? '다름' : ''} |`);
        }
      }
    }
  }
  return lines;
}

function renderUnits(actual) {
  const lines = ['| 번호 | 병과 | 공격 | 방어 | 사거리 | 최대 체력 | 간격 | 생존 | 체력 |', '|---|---|---|---|---|---|---|---|---|'];
  for (const unit of actual.units) {
    lines.push(`| ${unit.id} | ${unit.key} | ${unit.atk} | ${unit.armor} | ${unit.range} | ${unit.maxHp} | ${unit.interval} | ${unit.alive} | ${unit.hp} |`);
  }
  return lines;
}

function renderReport(comparison) {
  const { result, runRel, rows, outside, inputs } = comparison;
  const { match, mismatch, ambiguous } = summarize(rows);
  const out = [];
  out.push(`# 손계산 시나리오 교차 검증 보고서: ${runRel}`);
  out.push('');
  out.push('`tools/hand_crosscheck.js compare --report`가 만든 파일이다. 손으로 고치지 않는다. 판정은 맨 아래 "판정" 칸에 사람이 적는다.');
  out.push('');
  out.push(`- 검증자: ${result.tool} · ${result.model} · ${result.date} · 방법 ${result.method}`);
  out.push(`- 입력: inputsId ${inputs.inputsId} · 규칙 문서 SHA-256 ${inputs.rulesSha256}`);
  out.push(`- 일치 ${match.length}/${rows.length}${match.length ? ` (${match.map((row) => row.id).join(', ')})` : ''}`);
  out.push(`- 불일치 ${mismatch.length}${mismatch.length ? ` (${mismatch.map((row) => row.id).join(', ')})` : ''}`);
  out.push(`- 모호 표시 ${ambiguous.length}${ambiguous.length ? ` (${ambiguous.map((row) => row.id).join(', ')})` : ''}`);
  out.push(`- 허용 목록 밖 파일 ${outside.length}${outside.length ? `: ${outside.map((file) => `\`${file}\``).join(', ')}` : ''}`);
  out.push(`- 검증자가 연 파일: ${result.filesRead.map((file) => `\`${file}\``).join(', ')}`);
  out.push(`- 검증자가 실행한 명령: ${result.commandsRun.length ? result.commandsRun.map((command) => `\`${command}\``).join(', ') : '없음'}`);
  if (result.notes.trim() !== '') {
    out.push(`- 검증자 메모: ${result.notes}`);
  }

  for (const row of [...mismatch, ...ambiguous]) {
    const { scenario, got } = row;
    const expected = isObj(scenario.expected) ? scenario.expected : {};
    out.push('');
    out.push(`## ${row.id} · ${row.category === 'mismatch' ? '불일치' : '모호 표시'}`);
    out.push('');
    if (Array.isArray(scenario.clauses)) {
      out.push(`- 조항: ${scenario.clauses.join(', ')}`);
    }
    if (isStr(scenario.purpose)) {
      out.push(`- 목적: ${scenario.purpose}`);
    }
    if (row.diffs.length > 0) {
      out.push(`- 다른 항목: ${row.diffs.join(', ')}`);
    }
    out.push('');
    out.push('```');
    out.push('아군(진영 0)   해골(진영 1)');
    for (let index = 0; index < Math.max(scenario.ally.length, scenario.enemy.length); index += 1) {
      out.push(`행 ${index}  ${scenario.ally[index] || ''}   ${scenario.enemy[index] || ''}`);
    }
    out.push('```');
    if (got.result) {
      out.push('');
      out.push(...renderResultTable(expected, got.result));
      out.push('');
      out.push('검증자가 계산한 유닛(링크를 반영한 능력치와 끝 상태)');
      out.push('');
      out.push(...renderUnits(got.result));
    } else {
      out.push('');
      out.push(`기존 기대값: ${show(expected)}`);
    }
    if (got.ambiguities.length > 0) {
      out.push('');
      out.push('검증자가 모호하다고 적은 곳');
      out.push('');
      got.ambiguities.forEach((item, index) => {
        out.push(`${index + 1}. ${item.clause}: "${item.quote}"`);
        item.readings.forEach((reading, k) => out.push(`   - 읽기 ${k + 1}: ${reading}`));
        out.push(`   - 결과에 미치는 영향: ${item.effect}`);
      });
    }
    if (Array.isArray(got.alternatives) && got.alternatives.length > 0) {
      got.alternatives.forEach((item) => {
        out.push('');
        out.push(`읽기별 결과: ${item.reading}`);
        out.push('');
        out.push(...renderResultTable(expected, item.result));
      });
    }
    out.push('');
    out.push('기존 계산 과정');
    out.push('');
    (Array.isArray(scenario.steps) ? scenario.steps : []).forEach((line, index) => out.push(`${index + 1}. ${line}`));
    out.push('');
    out.push('검증자 계산 과정');
    out.push('');
    got.steps.forEach((line, index) => out.push(`${index + 1}. ${line}`));
    if (got.log.length > 0) {
      out.push('');
      out.push('검증자 로그(틱, 종류, 행위자, 표적, 값)');
      out.push('');
      out.push('```');
      got.log.forEach((line) => out.push(line.join(', ')));
      out.push('```');
    }
    out.push('');
    out.push('판정(본인)');
    out.push('');
    out.push('- [ ] 기존 기대값이 맞다(검증자가 틀렸다)');
    out.push('- [ ] 검증자가 맞다(기대값과 계산 과정을 고친다)');
    out.push('- [ ] 규칙 문서가 모호하다(문서를 먼저 고친다)');
    out.push('- 사유:');
  }
  out.push('');
  return out.join('\n');
}

// ---------- 하위 명령 ----------

function cmdExport(root, flags) {
  const { inputs } = buildInputs(root);
  const text = serialize(inputs);
  const file = path.join(root, INPUTS_REL);
  if (flags.has('--check')) {
    const current = fs.existsSync(file) ? readText(file).replace(/\r\n/g, '\n') : null;
    if (current !== text) {
      console.log(`${INPUTS_REL}이 지금의 ${HAND_REL}, ${RULES_REL}과 맞지 않는다. export를 다시 돌린다`);
      return 1;
    }
    console.log(`${INPUTS_REL} 최신 · 시나리오 ${inputs.scenarios.length}건 · inputsId ${inputs.inputsId}`);
    return 0;
  }
  fs.mkdirSync(path.dirname(file), { recursive: true });
  fs.writeFileSync(file, text, 'utf8');
  console.log(`눈가림 입력을 썼다: ${INPUTS_REL} · 시나리오 ${inputs.scenarios.length}건 · inputsId ${inputs.inputsId}`);
  return 0;
}

// 검증자가 실행해도 되는 하위 명령은 이것뿐이다. hand_v1.json을 읽지 않는다.
function cmdValidate(root, argument) {
  const inputs = readJson(path.join(root, INPUTS_REL));
  const { result, runRel } = loadResult(root, argument);
  const errors = validateShape(result, inputs);
  if (errors.length > 0) {
    errors.forEach((message) => console.log(`형식 오류: ${message}`));
    console.log(`형식 검사 실패 ${errors.length}건 (${runRel})`);
    return 1;
  }
  console.log(`형식 검사 통과 · 시나리오 ${result.scenarios.length}건 (${runRel})`);
  return 0;
}

function cmdCompare(root, argument, flags) {
  const comparison = compareAll(root, argument);
  if (comparison.formatErrors.length > 0) {
    comparison.formatErrors.forEach((message) => console.log(`형식 오류: ${message}`));
    console.log('형식 오류가 있어 비교하지 않았다. inputsId가 다르면 검증 뒤에 시나리오, 수치, 규칙 문서가 바뀐 것이다');
    return 1;
  }
  printSummary(comparison);
  if (flags.has('--report')) {
    const reportFile = path.join(path.dirname(comparison.file), 'report.md');
    fs.writeFileSync(reportFile, renderReport(comparison), 'utf8');
    console.log(`사람용 보고서를 썼다: ${toRel(root, reportFile)}`);
  }
  const { match } = summarize(comparison.rows);
  return match.length === comparison.rows.length && comparison.outside.length === 0 ? 0 : 1;
}

function escapeRegExp(text) {
  return text.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

// 확인 상태의 글자만 바꾼다. 파일의 다른 글자(줄바꿈, 들여쓰기 포함)는 건드리지 않는다.
function replaceStatus(text, id, status) {
  const idMatch = new RegExp(`"id"\\s*:\\s*"${escapeRegExp(id)}"`).exec(text);
  if (!idMatch) {
    throw new UsageError(`${HAND_REL}: ${id}를 찾지 못했다`);
  }
  const nextId = /"id"\s*:\s*"/g;
  nextId.lastIndex = idMatch.index + idMatch[0].length;
  const next = nextId.exec(text);
  const end = next ? next.index : text.length;
  const statusPattern = /"status"\s*:\s*"((?:[^"\\]|\\.)*)"/g;
  statusPattern.lastIndex = idMatch.index;
  let last = null;
  for (let found = statusPattern.exec(text); found && found.index < end; found = statusPattern.exec(text)) {
    last = found;
  }
  if (!last) {
    throw new UsageError(`${HAND_REL}: ${id}의 status를 찾지 못했다`);
  }
  const valueStart = last.index + last[0].length - 1 - last[1].length;
  return text.slice(0, valueStart) + JSON.stringify(status).slice(1, -1) + text.slice(valueStart + last[1].length);
}

function cmdApply(root, argument) {
  const comparison = compareAll(root, argument);
  if (comparison.formatErrors.length > 0) {
    comparison.formatErrors.forEach((message) => console.log(`형식 오류: ${message}`));
    console.log('형식 오류가 있어 확인 상태를 적지 않았다');
    return 1;
  }
  printSummary(comparison);
  if (comparison.outside.length > 0) {
    console.log('허용 목록 밖의 파일을 연 실행이다. 사람이 판정하기 전에는 확인 상태를 적지 않는다');
    return 1;
  }
  const model = comparison.result.model.replace(/[()"\\\r\n]/g, ' ').replace(/\s+/g, ' ').trim();
  const status = `교차 확인(${model}, ${comparison.result.date})`;
  const targets = summarize(comparison.rows).match.filter((row) => row.scenario.status === UNCONFIRMED).map((row) => row.id);
  if (targets.length === 0) {
    console.log(`확인 상태를 적을 시나리오가 없다(일치하면서 "${UNCONFIRMED}"인 것만 적는다)`);
    return 0;
  }
  const handFile = path.join(root, HAND_REL);
  const original = fs.readFileSync(handFile, 'utf8');
  let text = original;
  for (const id of targets) {
    text = replaceStatus(text, id, status);
  }
  // 다시 읽어 확인 상태 말고 달라진 것이 없는지 본다. 다르면 쓰지 않는다.
  const strip = (raw) => (raw.charCodeAt(0) === 0xfeff ? raw.slice(1) : raw);
  const want = JSON.parse(strip(original));
  want.scenarios.forEach((scenario) => {
    if (targets.includes(scenario.id)) {
      scenario.status = status;
    }
  });
  if (JSON.stringify(JSON.parse(strip(text))) !== JSON.stringify(want)) {
    throw new UsageError(`${HAND_REL}: 확인 상태 말고 다른 곳이 바뀔 뻔했다. 파일을 쓰지 않았다`);
  }
  fs.writeFileSync(handFile, text, 'utf8');
  console.log(`확인 상태를 적었다 ${targets.length}건: ${targets.join(', ')} → ${status}`);
  return 0;
}

function main(argv) {
  const args = [];
  const flags = new Set();
  let root = path.resolve(__dirname, '..');
  for (let index = 0; index < argv.length; index += 1) {
    if (argv[index] === '--root') {
      // 스크립트 자체를 시험할 때만 쓴다.
      root = path.resolve(argv[index + 1] || '');
      index += 1;
    } else if (argv[index].startsWith('--')) {
      flags.add(argv[index]);
    } else {
      args.push(argv[index]);
    }
  }
  const [command, argument] = args;
  switch (command) {
    case 'export': return cmdExport(root, flags);
    case 'validate': return cmdValidate(root, argument);
    case 'compare': return cmdCompare(root, argument, flags);
    case 'apply': return cmdApply(root, argument);
    default:
      throw new UsageError('사용법: node tools/hand_crosscheck.js <export [--check] | validate <결과 파일> | compare <결과 파일> [--report] | apply <결과 파일>>');
  }
}

if (require.main === module) {
  try {
    process.exitCode = main(process.argv.slice(2));
  } catch (error) {
    console.error(error instanceof UsageError ? error.message : error.stack);
    process.exitCode = 2;
  }
} else {
  module.exports = { buildInputs, validateShape, diffScenario, outsideAllowed, replaceStatus, renderReport, main };
}

'use strict';

// RANK & FILE 전투 규칙(docs/rules.md)만으로 손계산 시나리오(T5)를 계산하는 독립 프로그램.
// 입력: docs/crosscheck/hand_v1/inputs.json
// 출력: docs/crosscheck/hand_v1/run1/result.json

const fs = require('fs');
const path = require('path');

const BOARD_ROWS = 4; // §1, §2.1, §7
const BOARD_COLS = 6; // §1, §2.1, §7

const LINK_STAT_LABEL = {
  None: '없음',
  Armor: '방어',
  Atk: '공격',
  Range: '사거리',
  Hp: '최대 체력',
  Interval: '간격',
};

const TARGET_LABEL = {
  Front: '맨 앞',
  Back: '맨 뒤',
  Support: '지원',
  Area: '범위',
};

function colSubject(col) {
  return col === 2 || col === 4 || col === 5 ? `열 ${col}가` : `열 ${col}이`;
}

function factionName(faction) {
  return faction === 0 ? '진영 0(아군)' : '진영 1(해골)';
}

function gateNameForTarget(attackerFaction) {
  return attackerFaction === 0 ? '결계(진영 1)' : '성문(진영 0)';
}

function validateScenario(scenario, unitMap) {
  const boards = [scenario.ally, scenario.enemy];
  const counts = [0, 0];

  for (let f = 0; f < 2; f++) {
    const board = boards[f];
    if (!Array.isArray(board) || board.length !== BOARD_ROWS) {
      return {
        valid: false,
        reason: `${factionName(f)} 진형판 행 수가 ${BOARD_ROWS}이 아니다 (§2.1, §3.6)`,
      };
    }
    for (let r = 0; r < BOARD_ROWS; r++) {
      const rowStr = board[r];
      if (typeof rowStr !== 'string' || rowStr.length !== BOARD_COLS) {
        return {
          valid: false,
          reason: `${factionName(f)} 행 ${r}의 열 수가 ${BOARD_COLS}이 아니다 (§2.1, §3.6)`,
        };
      }
      for (let c = 0; c < BOARD_COLS; c++) {
        const ch = rowStr[c];
        if (ch === '.') continue;
        if (!unitMap.has(ch)) {
          return {
            valid: false,
            reason: `${factionName(f)} (${r},${c})에 병과 수치(§7)에 없는 병과 키 '${ch}'가 있다 (§3.6)`,
          };
        }
        counts[f]++;
      }
    }
  }

  if (counts[0] === 0 && counts[1] === 0) {
    return {
      valid: false,
      reason: '양쪽 진영이 모두 비어 있다 (§3.6)',
    };
  }
  if (counts[0] === 0) {
    return {
      valid: false,
      reason: '진영 0(아군)에 병사가 0명이다 (§3.6)',
    };
  }
  if (counts[1] === 0) {
    return {
      valid: false,
      reason: '진영 1(해골)에 병사가 0명이다 (§3.6)',
    };
  }

  return { valid: true, counts };
}

function computeVerdict(outcome, gate, barrier) {
  // §5.2 플레이어 기준 판정
  if (outcome === 'A') {
    return barrier <= 0 ? '결계 파괴' : '섬멸';
  }
  if (outcome === 'Timeout') {
    return '새벽';
  }
  if (outcome === 'Draw') {
    return gate > 0 ? '성문 사수' : '성문 붕괴';
  }
  if (outcome === 'B') {
    return gate <= 0 ? '성문 붕괴' : '전멸';
  }
  return '없음';
}

function simulateScenario(scenario, defs) {
  const unitMap = new Map();
  for (const u of defs.units) {
    unitMap.set(u.key, u);
  }
  const rules = defs.rules;
  const steps = [];

  // §3.6 시작 조건 검사
  const check = validateScenario(scenario, unitMap);
  if (!check.valid) {
    steps.push(`시작 조건 검사 실패: ${check.reason}`);
    steps.push(
      '§3.6에 따라 전투를 계산하지 않고 입력 오류(outcome=Invalid)를 반환한다. 틱·해시·이벤트가 없고(endTick=null, gate=null, barrier=null, units=[], log=[]), §5.2 표에 해당하는 판정이 없으므로 verdict=없음이다.'
    );
    return {
      id: scenario.id,
      status: 'computed',
      result: {
        outcome: 'Invalid',
        verdict: '없음',
        endTick: null,
        gate: null,
        barrier: null,
        units: [],
      },
      log: [],
      steps,
      ambiguities: [],
    };
  }

  steps.push(
    `시작 조건 검사 통과 (§3.6): 진영 0 병사 ${check.counts[0]}명, 진영 1 병사 ${check.counts[1]}명, 모든 병과 키 유효. 성문=${rules.gateHp}, 결계=${rules.gateHp} (§2.7, §7).`
  );

  const boards = [scenario.ally, scenario.enemy];
  const queues = [
    Array.from({ length: BOARD_COLS }, () => []),
    Array.from({ length: BOARD_COLS }, () => []),
  ];
  const allUnits = [];

  // §3.1 생성 순서: 진영 0 -> 1, 행 0 -> 3, 열 0 -> 5
  const dirs = [
    [-1, 0],
    [1, 0],
    [0, -1],
    [0, 1],
  ];

  for (let f = 0; f < 2; f++) {
    for (let r = 0; r < BOARD_ROWS; r++) {
      for (let c = 0; c < BOARD_COLS; c++) {
        const key = boards[f][r][c];
        if (key === '.') continue;
        const base = unitMap.get(key);
        const id = f * 100 + r * BOARD_COLS + c; // §3.2
        const initQueueIdx = queues[f][c].length; // §3.1, §2.3

        // §2.2, §3.3, §3.4 링크 보너스 계산 (같은 진형판 내 상하좌우 이웃)
        // 같은 종류를 두 병과 이상이 주면 병과마다 따로 겹 수를 세고 상한을 적용한다 (§3.4, §9 17번).
        const neighborCountsByKey = new Map();
        const neighborSources = [];
        for (const [dr, dc] of dirs) {
          const nr = r + dr;
          const nc = c + dc;
          if (nr < 0 || nr >= BOARD_ROWS || nc < 0 || nc >= BOARD_COLS) continue;
          const nKey = boards[f][nr][nc];
          if (nKey === '.' || nKey === key) continue;
          const nBase = unitMap.get(nKey);
          if (nBase.linkStat === 'None') continue;
          const nId = f * 100 + nr * BOARD_COLS + nc;
          neighborCountsByKey.set(nKey, (neighborCountsByKey.get(nKey) || 0) + 1);
          neighborSources.push(`#${nId}(${nKey}, ${nr},${nc})`);
        }

        let armorBonus = 0;
        let atkBonus = 0;
        let rangeBonus = 0;
        let hpBonus = 0;
        let intervalBonus = 0;
        const linkDetails = [];

        for (const [nKey, rawCount] of neighborCountsByKey.entries()) {
          const nBase = unitMap.get(nKey);
          const capped = Math.min(rawCount, rules.linkStackMax);
          const delta = nBase.linkAmount * capped;
          if (nBase.linkStat === 'Armor') armorBonus += delta;
          else if (nBase.linkStat === 'Atk') atkBonus += delta;
          else if (nBase.linkStat === 'Range') rangeBonus += delta;
          else if (nBase.linkStat === 'Hp') hpBonus += delta;
          else if (nBase.linkStat === 'Interval') intervalBonus += delta;
          const sign = delta >= 0 ? `+${delta}` : `${delta}`;
          linkDetails.push(
            `${nKey}(${nBase.name}) ${rawCount}명 -> ${LINK_STAT_LABEL[nBase.linkStat]} ${nBase.linkAmount}×min(${rawCount},${rules.linkStackMax})=${sign}`
          );
        }

        // §3.4 최종 능력치
        const armor = base.armor + armorBonus;
        const atk = base.atk + atkBonus;
        const range = Math.min(base.range + rangeBonus, rules.rangeMax);
        const maxHp = base.hp + hpBonus;
        const interval = Math.max(base.interval + intervalBonus, rules.intervalMin);

        const unit = {
          id,
          faction: f,
          row: r,
          col: c,
          key,
          name: base.name,
          atk,
          armor,
          range,
          maxHp,
          interval,
          target: base.target,
          magic: base.magic,
          revive: base.revive,
          hp: maxHp,
          cd: interval, // §3.5
          alive: true,
        };

        queues[f][c].push(unit);
        allUnits.push(unit);

        const linkSummary =
          linkDetails.length > 0
            ? `링크(§2.2, §3.3, §3.4): 이웃[${neighborSources.join(', ')}]로부터 ${linkDetails.join('; ')}`
            : '링크(§2.2, §3.3): 없음';

        steps.push(
          `유닛 생성 (§3.1~§3.5): #${id} ${key}(${base.name}), 진영 ${f} 행 ${r} 열 ${c}, 초기 대열 순번 ${initQueueIdx}. ${linkSummary}. 최종 능력치: 공격 ${atk}, 방어 ${armor}, 사거리 ${range}, 최대 체력 ${maxHp}, 간격 ${interval}, 초기 대기 ${unit.cd}, 표적 ${TARGET_LABEL[base.target]}${base.magic ? '(마법)' : '(물리)'}${base.revive > 0 ? `, 부활 ${base.revive}` : ''}.`
        );
      }
    }
  }

  let gate = rules.gateHp;
  let barrier = rules.gateHp;
  const log = [];
  let outcome = null;
  let endTick = null;

  // §4 한 틱의 처리 순서: 1틱부터 maxTicks(600)까지
  for (let tick = 1; tick <= rules.maxTicks; tick++) {
    const intents = [];
    const waitingNotes = [];

    // §4.1 ① 공격할 병사 모으기: 진영 0 -> 1, 열 0 -> 5, 대열 앞 -> 뒤
    for (let f = 0; f < 2; f++) {
      const opp = 1 - f;
      for (let c = 0; c < BOARD_COLS; c++) {
        const q = queues[f][c];
        for (let idx = 0; idx < q.length; idx++) {
          const u = q[idx];
          if (u.cd > 0) {
            u.cd -= 1;
          }
          if (u.cd > 0) {
            continue;
          }
          if (idx >= u.range) {
            waitingNotes.push(`#${u.id}(${u.key}, 열${c} 순번${idx}≥사거리${u.range}, cd=0 대기)`);
            continue;
          }

          // §4.2 표적 선택
          if (u.target === 'Front') {
            const eq = queues[opp][c];
            if (eq.length > 0) {
              intents.push({
                attacker: u,
                isGate: false,
                targetUnit: eq[0],
                targetReason: `§4.2 맨 앞: 마주 보는 열 ${c}의 대열 맨 앞 #${eq[0].id}`,
              });
            } else {
              intents.push({
                attacker: u,
                isGate: true,
                targetUnit: null,
                targetReason: `§4.2 맨 앞: 마주 보는 ${colSubject(c)} 비어 ${gateNameForTarget(f)} 타격`,
              });
            }
          } else if (u.target === 'Back') {
            const eq = queues[opp][c];
            if (eq.length > 0) {
              const backUnit = eq[eq.length - 1];
              intents.push({
                attacker: u,
                isGate: false,
                targetUnit: backUnit,
                targetReason: `§4.2 맨 뒤: 마주 보는 열 ${c}의 대열 맨 뒤 #${backUnit.id}`,
              });
            } else {
              intents.push({
                attacker: u,
                isGate: true,
                targetUnit: null,
                targetReason: `§4.2 맨 뒤: 마주 보는 ${colSubject(c)} 비어 ${gateNameForTarget(f)} 타격`,
              });
            }
          } else if (u.target === 'Support') {
            let foundCol = -1;
            let searchDesc = '';
            if (queues[opp][c].length > 0) {
              foundCol = c;
              searchDesc = `마주 보는 열 ${c}의 맨 앞`;
            } else {
              for (let d = 1; d < BOARD_COLS; d++) {
                const left = c - d;
                const right = c + d;
                if (left >= 0 && queues[opp][left].length > 0) {
                  foundCol = left;
                  searchDesc = `열 ${c} 비어 거리 ${d} 왼쪽 우선 열 ${left}의 맨 앞`;
                  break;
                }
                if (right < BOARD_COLS && queues[opp][right].length > 0) {
                  foundCol = right;
                  searchDesc = `열 ${c} 비어 거리 ${d} 열 ${right}의 맨 앞`;
                  break;
                }
              }
            }
            if (foundCol >= 0) {
              const targetUnit = queues[opp][foundCol][0];
              intents.push({
                attacker: u,
                isGate: false,
                targetUnit,
                targetReason: `§4.2 지원: ${searchDesc} #${targetUnit.id}`,
              });
            } else {
              intents.push({
                attacker: u,
                isGate: true,
                targetUnit: null,
                targetReason: `§4.2 지원: 상대 모든 열이 비어 ${gateNameForTarget(f)} 타격`,
              });
            }
          } else if (u.target === 'Area') {
            const colsToCheck = [c - 1, c, c + 1];
            const hitCols = [];
            for (const tc of colsToCheck) {
              if (tc >= 0 && tc < BOARD_COLS && queues[opp][tc].length > 0) {
                const targetUnit = queues[opp][tc][0];
                hitCols.push(tc);
                intents.push({
                  attacker: u,
                  isGate: false,
                  targetUnit,
                  targetReason: `§4.2 범위(열 ${c - 1},${c},${c + 1} 순서): 열 ${tc} 맨 앞 #${targetUnit.id}`,
                });
              }
            }
            if (hitCols.length === 0) {
              intents.push({
                attacker: u,
                isGate: true,
                targetUnit: null,
                targetReason: `§4.2 범위(§9 8번): 열 ${c - 1},${c},${c + 1}(판 밖은 빈 열)이 모두 비어 ${gateNameForTarget(f)} 1회 타격`,
              });
            }
          }

          u.cd = u.interval; // §4.1 ① 대기 시간을 간격으로 되돌림
        }
      }
    }

    // §4.1 ② 의도가 없으면 ③~⑤ 건너뜀
    if (intents.length === 0) {
      continue;
    }

    // §4.1 ③ 피해 주기
    const actionDescs = [];
    for (const intent of intents) {
      const a = intent.attacker;
      if (intent.isGate) {
        const dmg = rules.gateDamage; // §4.3
        if (a.faction === 0) {
          const before = barrier;
          barrier -= dmg;
          log.push([tick, 1, a.id, 999, dmg]); // §6
          actionDescs.push(
            `#${a.id}(${a.key})->결계(999) [${intent.targetReason}; §4.3 성문피해=${dmg}; 결계 ${before}->${barrier}]`
          );
        } else {
          const before = gate;
          gate -= dmg;
          log.push([tick, 1, a.id, 999, dmg]); // §6
          actionDescs.push(
            `#${a.id}(${a.key})->성문(999) [${intent.targetReason}; §4.3 성문피해=${dmg}; 성문 ${before}->${gate}]`
          );
        }
      } else {
        const t = intent.targetUnit;
        const dmg = a.magic ? a.atk : Math.max(1, a.atk - t.armor); // §4.3
        const formula = a.magic
          ? `마법=${a.atk}`
          : `물리 max(1,${a.atk}-${t.armor})=${dmg}`;
        const before = t.hp;
        t.hp -= dmg; // §4.1 ③ 0 아래로도 내려감
        log.push([tick, 2, a.id, t.id, dmg]); // §6
        actionDescs.push(
          `#${a.id}(${a.key})->#${t.id}(${t.key}) [${intent.targetReason}; §4.3 ${formula}; 체력 ${before}->${t.hp}]`
        );
      }
    }

    // §4.1 ④ 쓰러진 병사 정리: 진영 0 -> 1, 열 0 -> 5, 대열 앞 -> 뒤
    const cleanupDescs = [];
    for (let f = 0; f < 2; f++) {
      for (let c = 0; c < BOARD_COLS; c++) {
        const q = queues[f][c];
        const nextQ = [];
        let removedAny = false;
        let shiftedAny = false;
        for (let idx = 0; idx < q.length; idx++) {
          const u = q[idx];
          if (u.hp <= 0) {
            if (u.revive > 0) {
              const reviveHp = u.revive;
              u.hp = reviveHp;
              u.revive = 0;
              log.push([tick, 3, u.id, u.id, reviveHp]); // §6
              if (nextQ.length !== idx) shiftedAny = true;
              nextQ.push(u);
              cleanupDescs.push(
                `#${u.id}(${u.key}) 부활(체력->${reviveHp}, 부활->0, 자리·cd=${u.cd} 유지; §2.8, §4.1 ④, §9 3번)`
              );
            } else {
              u.alive = false;
              removedAny = true;
              log.push([tick, 4, u.id, u.id, 0]); // §6
              cleanupDescs.push(`#${u.id}(${u.key}) 사망(대열 이탈; §4.1 ④)`);
            }
          } else {
            if (nextQ.length !== idx) shiftedAny = true;
            nextQ.push(u);
          }
        }
        if (removedAny && nextQ.length > 0) {
          const shifted = nextQ.map((u, i) => `#${u.id}(순번 ${i}, cd=${u.cd})`).join(', ');
          if (shiftedAny) {
            cleanupDescs.push(
              `진영 ${f} 열 ${c} 대열 당김(§2.3, §4.1 ④, §9 7번): [${shifted}]`
            );
          } else {
            cleanupDescs.push(
              `진영 ${f} 열 ${c} 남은 대열(앞 순번 변동 없음; §4.1 ④): [${shifted}]`
            );
          }
        }
        queues[f][c] = nextQ;
      }
    }

    // §4.1 ⑤ 승패 확인 (§5.1)
    let allyAlive = 0;
    let enemyAlive = 0;
    for (let c = 0; c < BOARD_COLS; c++) {
      allyAlive += queues[0][c].length;
      enemyAlive += queues[1][c].length;
    }
    const allyLost = gate <= 0 || allyAlive === 0;
    const enemyLost = barrier <= 0 || enemyAlive === 0;

    let checkDesc;
    if (allyLost && enemyLost) {
      outcome = 'Draw';
      endTick = tick;
      checkDesc = `§5.1 양쪽 패배 조건 동시 충족(성문=${gate}, 아군=${allyAlive}, 결계=${barrier}, 적=${enemyAlive}) -> Draw 종료`;
    } else if (enemyLost) {
      outcome = 'A';
      endTick = tick;
      checkDesc = `§5.1 진영 1만 패배(결계=${barrier}, 적=${enemyAlive}; 성문=${gate}, 아군=${allyAlive}) -> A 종료`;
    } else if (allyLost) {
      outcome = 'B';
      endTick = tick;
      checkDesc = `§5.1 진영 0만 패배(성문=${gate}, 아군=${allyAlive}; 결계=${barrier}, 적=${enemyAlive}) -> B 종료`;
    } else {
      checkDesc = `§5.1 계속(성문=${gate}, 결계=${barrier}, 아군=${allyAlive}, 적=${enemyAlive})`;
    }

    const waitStr = waitingNotes.length > 0 ? ` [사거리 밖 대기 §4.1 ①: ${waitingNotes.join(', ')}]` : '';
    const cleanStr = cleanupDescs.length > 0 ? cleanupDescs.join('; ') : '쓰러진 병사 없음(§4.1 ④)';
    steps.push(
      `틱 ${tick}: ①·③ ${actionDescs.join(' / ')}${waitStr} | ④ ${cleanStr} | ⑤ ${checkDesc}`
    );

    if (outcome !== null) {
      break;
    }
  }

  if (outcome === null) {
    outcome = 'Timeout';
    endTick = rules.maxTicks;
    steps.push(
      `틱 ${rules.maxTicks} 완료까지 어느 진영도 패배 조건에 해당하지 않아 Timeout 종료(성문=${gate}, 결계=${barrier}; §5.1).`
    );
  }

  const verdict = computeVerdict(outcome, gate, barrier);
  steps.push(
    `최종 결론: 끝난 틱=${endTick}, 결과=${outcome}(§5.1), 판정=${verdict}(§5.2; 성문=${gate}, 결계=${barrier}).`
  );

  return {
    id: scenario.id,
    status: 'computed',
    result: {
      outcome,
      verdict,
      endTick,
      gate,
      barrier,
      units: allUnits.map((u) => ({
        id: u.id,
        key: u.key,
        atk: u.atk,
        armor: u.armor,
        range: u.range,
        maxHp: u.maxHp,
        interval: u.interval,
        alive: u.alive,
        hp: u.hp,
      })),
    },
    log,
    steps,
    ambiguities: [],
  };
}

function main() {
  const inputsPath = path.join(__dirname, '..', 'inputs.json');
  const outputPath = path.join(__dirname, 'result.json');
  const inputs = JSON.parse(fs.readFileSync(inputsPath, 'utf8'));

  const scenarios = inputs.scenarios.map((sc) => simulateScenario(sc, inputs.defs));

  const output = {
    inputsId: inputs.inputsId,
    run: 'run1',
    tool: 'Antigravity',
    model: 'Gemini 4 Neon (XHigh)',
    date: '2026-11-20',
    method: 'program',
    filesRead: [
      'docs/crosscheck/hand_v1/INSTRUCTIONS.md',
      'docs/AGENTS.md',
      'docs/rules.md',
      'docs/crosscheck/hand_v1/inputs.json',
    ],
    commandsRun: [
      'node docs/crosscheck/hand_v1/run1/solve.js',
      'node tools/hand_crosscheck.js validate docs/crosscheck/hand_v1/run1/result.json',
    ],
    notes:
      '루트 AGENTS.md는 작업공간 규칙으로 자동 로드됨. inputs.json의 defs.units와 defs.rules는 docs/rules.md §7의 표·매개변수와 전부 일치함. docs/rules.md만 보고 docs/crosscheck/hand_v1/run1/solve.js를 새로 작성해 18건을 계산했고, 프로그램 출력 로그 중 h01, h02, h03, h04, h05, h06, h07, h08, h09, h10, h12를 docs/rules.md와 한 틱씩 직접 대조 검산했으며, h11, h13, h14, h15는 주기·경계 틱을 직접 검산하고 h16~h18은 §3.6을 직접 대조함.',
    scenarios,
  };

  fs.writeFileSync(outputPath, JSON.stringify(output, null, 2) + '\n', 'utf8');
  console.log(`Wrote ${scenarios.length} scenarios to ${path.relative(process.cwd(), outputPath)}`);
}

main();

#!/usr/bin/env node
"use strict";

const fs = require("fs");
const path = require("path");

const BOARD_ROWS = 4;
const BOARD_COLS = 6;

const TARGET_LABEL = {
  Front: "맨 앞",
  Back: "맨 뒤",
  Support: "지원",
  Area: "범위",
};

const STAT_LABEL = {
  Armor: "방어",
  Atk: "공격",
  Range: "사거리",
  Hp: "최대 체력",
  Interval: "간격",
  None: "없음",
};

function sideLabel(side) {
  return side === 0 ? "진영 0(아군)" : "진영 1(해골)";
}

function targetStructureLabel(oppSide) {
  return oppSide === 0 ? "성문(진영 0)" : "결계(진영 1)";
}

function validateScenario(scenario, unitDefs) {
  const boards = [scenario.ally, scenario.enemy];
  const counts = [0, 0];

  for (let s = 0; s < 2; s++) {
    const board = boards[s];
    if (!Array.isArray(board) || board.length !== BOARD_ROWS) {
      return { ok: false, reason: `${sideLabel(s)}의 행 수가 ${BOARD_ROWS}가 아님` };
    }
    for (let r = 0; r < BOARD_ROWS; r++) {
      const rowStr = board[r];
      if (typeof rowStr !== "string" || rowStr.length !== BOARD_COLS) {
        return { ok: false, reason: `${sideLabel(s)} 행 ${r}의 열 수가 ${BOARD_COLS}가 아님` };
      }
      for (let c = 0; c < BOARD_COLS; c++) {
        const ch = rowStr[c];
        if (ch === ".") continue;
        if (!unitDefs[ch]) {
          return {
            ok: false,
            reason: `${sideLabel(s)} 행 ${r} 열 ${c}에 병과 수치(§7)에 없는 병과 키 '${ch}'가 있음`,
          };
        }
        counts[s]++;
      }
    }
  }

  if (counts[0] === 0 && counts[1] === 0) {
    return { ok: false, reason: "양쪽 진영 모두 병사가 없음" };
  }
  if (counts[0] === 0) {
    return { ok: false, reason: "진영 0(아군)이 비어 있음(병사 0명)" };
  }
  if (counts[1] === 0) {
    return { ok: false, reason: "진영 1(해골)이 비어 있음(병사 0명)" };
  }
  return { ok: true, counts };
}

function selectTargets(attacker, queues) {
  const opp = 1 - attacker.side;
  const oppCols = queues[opp];
  const c = attacker.col;
  const oppStruct = targetStructureLabel(opp);

  if (attacker.target === "Front") {
    if (oppCols[c].length > 0) {
      const t = oppCols[c][0];
      return {
        targets: [t],
        reason: `§4.2 맨 앞: 마주 보는 ${c}열 맨 앞 #${t.id}(${t.key})`,
      };
    }
    return {
      targets: ["GATE"],
      reason: `§4.2 맨 앞: 마주 보는 ${c}열이 비어 ${oppStruct} 타격`,
    };
  }

  if (attacker.target === "Back") {
    if (oppCols[c].length > 0) {
      const t = oppCols[c][oppCols[c].length - 1];
      return {
        targets: [t],
        reason: `§4.2 맨 뒤: 마주 보는 ${c}열 맨 뒤 #${t.id}(${t.key})`,
      };
    }
    return {
      targets: ["GATE"],
      reason: `§4.2 맨 뒤: 마주 보는 ${c}열이 비어 ${oppStruct} 타격`,
    };
  }

  if (attacker.target === "Support") {
    if (oppCols[c].length > 0) {
      const t = oppCols[c][0];
      return {
        targets: [t],
        reason: `§4.2 지원: 마주 보는 ${c}열 맨 앞 #${t.id}(${t.key})`,
      };
    }
    for (let d = 1; d < BOARD_COLS; d++) {
      const left = c - d;
      if (left >= 0 && oppCols[left].length > 0) {
        const t = oppCols[left][0];
        return {
          targets: [t],
          reason: `§4.2 지원: ${c}열 비어 거리 ${d} 왼쪽 ${left}열 맨 앞 #${t.id}(${t.key})`,
        };
      }
      const right = c + d;
      if (right < BOARD_COLS && oppCols[right].length > 0) {
        const t = oppCols[right][0];
        return {
          targets: [t],
          reason: `§4.2 지원: ${c}열 비어 거리 ${d} 오른쪽 ${right}열 맨 앞 #${t.id}(${t.key})`,
        };
      }
    }
    return {
      targets: ["GATE"],
      reason: `§4.2 지원: 상대 전 열이 비어 ${oppStruct} 타격`,
    };
  }

  if (attacker.target === "Area") {
    const targets = [];
    const hitCols = [];
    for (const tc of [c - 1, c, c + 1]) {
      if (tc >= 0 && tc < BOARD_COLS && oppCols[tc].length > 0) {
        targets.push(oppCols[tc][0]);
        hitCols.push(`${tc}열 #${oppCols[tc][0].id}(${oppCols[tc][0].key})`);
      }
    }
    if (targets.length > 0) {
      return {
        targets,
        reason: `§4.2 범위(${c - 1},${c},${c + 1}열): ${hitCols.join(", ")}`,
      };
    }
    return {
      targets: ["GATE"],
      reason: `§4.2 범위(${c - 1},${c},${c + 1}열, 판 밖은 빈 열): 세 열 모두 비어 ${oppStruct} 1회 타격`,
    };
  }

  throw new Error(`Unknown target type: ${attacker.target}`);
}

function solveScenario(scenario, unitDefs, rules) {
  const steps = [];
  const check = validateScenario(scenario, unitDefs);
  if (!check.ok) {
    steps.push(
      `§3.6 시작 조건 확인: 실패(${check.reason}). 전투를 계산하지 않고 입력 오류(Invalid) 반환(틱·해시·이벤트 없음).`
    );
    steps.push(
      `§5.2 판정: 입력 오류(§3.6)는 전투를 계산하지 않았으므로 판정하지 않음(결과 Invalid, 판정 없음).`
    );
    return {
      id: scenario.id,
      status: "computed",
      result: {
        outcome: "Invalid",
        verdict: "없음",
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
    `§3.6 시작 조건 확인: 통과(아군 ${check.counts[0]}명, 해골 ${check.counts[1]}명, 모든 병과 키 유효). 성문 ${rules.gateHp}, 결계 ${rules.gateHp}(§1, §2.7).`
  );

  // §3.1, §3.2 Unit creation and initial queues
  const boards = [scenario.ally, scenario.enemy];
  const units = [];
  const queues = [
    Array.from({ length: BOARD_COLS }, () => []),
    Array.from({ length: BOARD_COLS }, () => []),
  ];
  const grids = [
    Array.from({ length: BOARD_ROWS }, () => Array(BOARD_COLS).fill(null)),
    Array.from({ length: BOARD_ROWS }, () => Array(BOARD_COLS).fill(null)),
  ];

  for (let s = 0; s < 2; s++) {
    for (let r = 0; r < BOARD_ROWS; r++) {
      for (let c = 0; c < BOARD_COLS; c++) {
        const key = boards[s][r][c];
        if (key === ".") continue;
        const def = unitDefs[key];
        const id = s * 100 + r * BOARD_COLS + c;
        const queueIndex = queues[s][c].length;
        const unit = {
          id,
          key,
          name: def.name,
          side: s,
          row: r,
          col: c,
          initialQueueIndex: queueIndex,
          baseDef: def,
          target: def.target,
          magic: def.magic,
          revive: def.revive,
          atk: def.atk,
          armor: def.armor,
          range: def.range,
          maxHp: def.hp,
          interval: def.interval,
          hp: def.hp,
          cd: def.interval,
          alive: true,
        };
        units.push(unit);
        queues[s][c].push(unit);
        grids[s][r][c] = unit;
      }
    }
  }

  // §2.2, §2.8, §3.3, §3.4 Link calculation & final stats, §3.5 initial cd
  const dirs = [
    [-1, 0],
    [1, 0],
    [0, -1],
    [0, 1],
  ];

  for (const u of units) {
    const def = u.baseDef;
    const stacksByKey = {};
    const receivedFrom = [];

    if (def.linkStat !== "None") {
      for (const [dr, dc] of dirs) {
        const nr = u.row + dr;
        const nc = u.col + dc;
        if (nr < 0 || nr >= BOARD_ROWS || nc < 0 || nc >= BOARD_COLS) continue;
        const nb = grids[u.side][nr][nc];
        if (!nb) continue;
        if (nb.key === u.key) continue;
        if (nb.baseDef.linkStat === "None") continue;
        stacksByKey[nb.key] = (stacksByKey[nb.key] || 0) + 1;
        receivedFrom.push(`#${nb.id}(${nb.key})`);
      }
    }

    const bonuses = { Armor: 0, Atk: 0, Range: 0, Hp: 0, Interval: 0 };
    const linkDescParts = [];
    for (const [nbKey, rawCount] of Object.entries(stacksByKey)) {
      const nbDef = unitDefs[nbKey];
      const capped = Math.min(rawCount, rules.linkStackMax);
      const delta = nbDef.linkAmount * capped;
      bonuses[nbDef.linkStat] += delta;
      const sign = delta >= 0 ? "+" : "";
      const capNote = rawCount > rules.linkStackMax ? `(원래 ${rawCount}겹, 겹 상한 ${rules.linkStackMax} 적용)` : "";
      linkDescParts.push(
        `${nbKey} ${capped}겹${capNote}→${STAT_LABEL[nbDef.linkStat]}${sign}${delta}`
      );
    }

    u.armor = def.armor + bonuses.Armor;
    u.atk = def.atk + bonuses.Atk;
    u.range = Math.min(def.range + bonuses.Range, rules.rangeMax);
    u.maxHp = def.hp + bonuses.Hp;
    u.interval = Math.max(def.interval + bonuses.Interval, rules.intervalMin);
    u.hp = u.maxHp;
    u.cd = u.interval;

    const linkText =
      linkDescParts.length > 0
        ? `이웃[${receivedFrom.join(",")}]로부터 ${linkDescParts.join(", ")}`
        : def.linkStat === "None"
        ? "없음(해골은 링크 없음 §2.8, §3.3)"
        : "없음";

    steps.push(
      `§3.1~§3.5 유닛 #${u.id}(${u.key} ${u.name}): ${sideLabel(u.side)} 행 ${u.row} 열 ${u.col}, 대열 순번 ${u.initialQueueIndex} | 링크: ${linkText} | 최종 능력치 공격 ${u.atk}, 방어 ${u.armor}, 사거리 ${u.range}, 최대체력 ${u.maxHp}, 간격 ${u.interval}, 초기 대기시간 ${u.cd}, 표적 ${TARGET_LABEL[u.target]}${u.magic ? ", 마법" : ""}${u.revive > 0 ? `, 부활 ${u.revive}` : ""}`
    );
  }

  // §4 Tick loop
  const gate = [rules.gateHp, rules.gateHp];
  const log = [];
  let outcome = null;
  let endTick = null;

  for (let tick = 1; tick <= rules.maxTicks; tick++) {
    // §4.1 ① Collect intents: side 0->1, col 0->5, queue front->back
    const intents = [];
    for (let s = 0; s < 2; s++) {
      for (let c = 0; c < BOARD_COLS; c++) {
        const q = queues[s][c];
        for (let idx = 0; idx < q.length; idx++) {
          const u = q[idx];
          if (u.cd > 0) {
            u.cd -= 1;
            if (u.cd > 0) continue;
          }
          // u.cd === 0
          if (idx >= u.range) {
            // Out of range: stay at cd = 0 waiting
            continue;
          }
          const sel = selectTargets(u, queues);
          u.cd = u.interval;
          intents.push({
            attacker: u,
            targets: sel.targets,
            reason: sel.reason,
          });
        }
      }
    }

    // §4.1 ② Skip if no intents
    if (intents.length === 0) {
      continue;
    }

    // §4.1 ③ Apply damage in collected order
    const actionDescs = [];
    for (const intent of intents) {
      const u = intent.attacker;
      const opp = 1 - u.side;
      const hitDescs = [];
      for (const t of intent.targets) {
        if (t === "GATE") {
          const dmg = rules.gateDamage;
          gate[opp] -= dmg;
          log.push([tick, 1, u.id, 999, dmg]);
          hitDescs.push(
            `${targetStructureLabel(opp)} 피해 ${dmg}(§4.3 성문 피해)→남은 ${gate[opp]}`
          );
        } else {
          let dmg;
          let formula;
          if (u.magic) {
            dmg = u.atk;
            formula = `마법 ${u.atk}(§4.3)`;
          } else {
            dmg = Math.max(1, u.atk - t.armor);
            formula = `max(1,${u.atk}-${t.armor})=${dmg}(§4.3)`;
          }
          t.hp -= dmg;
          log.push([tick, 2, u.id, t.id, dmg]);
          hitDescs.push(`#${t.id} 피해 ${formula}→체력 ${t.hp}`);
        }
      }
      actionDescs.push(`#${u.id}(${u.key}) [${intent.reason}] → ${hitDescs.join("; ")}`);
    }

    // §4.1 ④ Cleanup fallen units: side 0->1, col 0->5, queue front->back
    const cleanupDescs = [];
    for (let s = 0; s < 2; s++) {
      for (let c = 0; c < BOARD_COLS; c++) {
        const oldQ = queues[s][c];
        const newQ = [];
        let shiftedAny = false;
        let removedAny = false;
        for (let i = 0; i < oldQ.length; i++) {
          const u = oldQ[i];
          if (u.hp <= 0) {
            if (u.revive > 0) {
              u.hp = u.revive;
              u.revive = 0;
              log.push([tick, 3, u.id, u.id, u.hp]);
              newQ.push(u);
              cleanupDescs.push(
                `#${u.id}(${u.key}) 부활→체력 ${u.hp}, 자리·대기시간(${u.cd}) 유지(§2.8, §4.1 ④)`
              );
            } else {
              u.alive = false;
              removedAny = true;
              log.push([tick, 4, u.id, u.id, 0]);
              cleanupDescs.push(`#${u.id}(${u.key}) 사망·대열 이탈(§4.1 ④)`);
            }
          } else {
            if (removedAny) shiftedAny = true;
            newQ.push(u);
          }
        }
        if (removedAny && newQ.length > 0) {
          const orderStr = newQ.map((x, i) => `${i}:#${x.id}(cd=${x.cd})`).join(",");
          if (shiftedAny) {
            cleanupDescs.push(
              `${sideLabel(s)} ${c}열 당김→[${orderStr}](대기시간 유지 §2.3, §4.1 ④)`
            );
          } else {
            cleanupDescs.push(`${sideLabel(s)} ${c}열 남은 대열→[${orderStr}]`);
          }
        }
        queues[s][c] = newQ;
      }
    }

    // §4.1 ⑤ Win/loss check (§5.1)
    let alive0 = 0;
    let alive1 = 0;
    for (let c = 0; c < BOARD_COLS; c++) {
      alive0 += queues[0][c].length;
      alive1 += queues[1][c].length;
    }
    const lost0 = gate[0] <= 0 || alive0 === 0;
    const lost1 = gate[1] <= 0 || alive1 === 0;

    let checkDesc;
    if (lost0 && lost1) {
      outcome = "Draw";
      endTick = tick;
      checkDesc = `§5.1 승패 확인: 양쪽 동시 패배(성문 ${gate[0]}, 아군 ${alive0}명 / 결계 ${gate[1]}, 해골 ${alive1}명)→Draw 종료`;
    } else if (lost1) {
      outcome = "A";
      endTick = tick;
      checkDesc = `§5.1 승패 확인: 진영 1 패배(결계 ${gate[1]}, 해골 ${alive1}명 / 성문 ${gate[0]}, 아군 ${alive0}명)→A 종료`;
    } else if (lost0) {
      outcome = "B";
      endTick = tick;
      checkDesc = `§5.1 승패 확인: 진영 0 패배(성문 ${gate[0]}, 아군 ${alive0}명 / 결계 ${gate[1]}, 해골 ${alive1}명)→B 종료`;
    } else {
      checkDesc = `§5.1 승패 확인: 계속(성문 ${gate[0]}, 결계 ${gate[1]}, 아군 ${alive0}명, 해골 ${alive1}명)`;
    }

    const cleanupPart = cleanupDescs.length > 0 ? ` | §4.1 ④ ${cleanupDescs.join("; ")}` : "";
    steps.push(`T${tick}: §4.1 ①·③ ${actionDescs.join(" / ")}${cleanupPart} | ${checkDesc}`);

    if (outcome !== null) {
      break;
    }
  }

  if (outcome === null) {
    outcome = "Timeout";
    endTick = rules.maxTicks;
    steps.push(
      `T${rules.maxTicks}: §5.1 ${rules.maxTicks}틱까지 양쪽 모두 패배하지 않음(성문 ${gate[0]}, 결계 ${gate[1]})→Timeout 종료.`
    );
  }

  // §5.2 Player-perspective verdict
  let verdict;
  let verdictReason;
  if (outcome === "A") {
    if (gate[1] <= 0) {
      verdict = "결계 파괴";
      verdictReason = `결과 A이고 결계(${gate[1]}) ≤ 0이므로 승리 "결계 파괴"`;
    } else {
      verdict = "섬멸";
      verdictReason = `결과 A이고 결계(${gate[1]}) > 0이므로 승리 "섬멸"`;
    }
  } else if (outcome === "Timeout") {
    verdict = "새벽";
    verdictReason = `결과 Timeout이므로 승리 "새벽"`;
  } else if (outcome === "Draw") {
    if (gate[0] > 0) {
      verdict = "성문 사수";
      verdictReason = `결과 Draw이고 성문(${gate[0]}) > 0이므로 승리 "성문 사수"`;
    } else {
      verdict = "성문 붕괴";
      verdictReason = `결과 Draw이고 성문(${gate[0]}) ≤ 0이므로 패배 "성문 붕괴"`;
    }
  } else {
    // B
    if (gate[0] <= 0) {
      verdict = "성문 붕괴";
      verdictReason = `결과 B이고 성문(${gate[0]}) ≤ 0이므로 패배 "성문 붕괴"`;
    } else {
      verdict = "전멸";
      verdictReason = `결과 B이고 성문(${gate[0]}) > 0이므로 패배 "전멸"`;
    }
  }

  steps.push(
    `종료 요약: 끝난 틱 ${endTick}(§5.1), 결과 ${outcome}(§5.1), 판정 "${verdict}"(§5.2: ${verdictReason}). 최종 성문 ${gate[0]}, 결계 ${gate[1]}.`
  );

  return {
    id: scenario.id,
    status: "computed",
    result: {
      outcome,
      verdict,
      endTick,
      gate: gate[0],
      barrier: gate[1],
      units: units.map((u) => ({
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
  const repoRoot = path.resolve(__dirname, "..", "..", "..", "..");
  const inputsPath = path.join(repoRoot, "docs", "crosscheck", "hand_v1", "inputs.json");
  const outPath = path.join(__dirname, "result.json");

  const inputs = JSON.parse(fs.readFileSync(inputsPath, "utf8"));
  const unitDefs = {};
  for (const u of inputs.defs.units) {
    unitDefs[u.key] = u;
  }
  const rules = inputs.defs.rules;

  const scenarios = inputs.scenarios.map((sc) => solveScenario(sc, unitDefs, rules));

  const output = {
    inputsId: inputs.inputsId,
    run: "run2",
    tool: "Antigravity",
    model: "Gemini 4 (XHigh)",
    date: "2026-11-21",
    method: "program",
    filesRead: [
      "docs/crosscheck/hand_v1/INSTRUCTIONS.md",
      "docs/AGENTS.md",
      "docs/rules.md",
      "docs/crosscheck/hand_v1/inputs.json",
    ],
    commandsRun: [
      "node docs/crosscheck/hand_v1/run2/solve.js",
      "node tools/hand_crosscheck.js validate docs/crosscheck/hand_v1/run2/result.json",
    ],
    notes:
      "docs/rules.md만을 근거로 solve.js를 새로 작성해 19건 전부를 계산했다. inputs.json의 defs는 docs/rules.md §7과 전부 일치했다. 수동 대조 검증 시나리오: h03(척후·마법사 링크, 일부 열만 빈 범위 공격의 결계 미타격, 맨 뒤 표적 및 부활), h04(링크 겹 상한 3, 열 우선 공격 순서, 동일 틱 헛방·음수 체력·부활), h06(대기시간 0 유지 후 전진 직후 다음 틱 즉시 공격·부활), h09(범위 3열 동시 마법 공격, 방어 무시, 지원 표적), h12(맨 뒤 표적, 동시 사망 Draw 및 성문 사수), h14(빈 행 당김, 음수 성문 붕괴), h19(동일 병과 무링크, 지원 표적, 동일 틱 음수 체력) 7건을 규칙 문서와 틱 단위로 직접 대조해 일치함을 확인했다.",
    scenarios,
  };

  fs.writeFileSync(outPath, JSON.stringify(output, null, 2) + "\n", "utf8");
  console.log(`Wrote ${outPath} (${scenarios.length} scenarios)`);
  for (const sc of scenarios) {
    const r = sc.result;
    console.log(
      `${sc.id}: outcome=${r.outcome}, verdict=${r.verdict}, endTick=${r.endTick}, gate=${r.gate}, barrier=${r.barrier}, logLines=${sc.log.length}`
    );
  }
}

main();

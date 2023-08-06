import { execSync } from "child_process"

export function getTundraTargets() {
  const targets = execSync("tundra2 -t").toString("utf-8")

  /** @type {string[]} */
  const units = []

  /** @type {string[]} */
  const commands = []

  /** @type {string[]} */
  const tests = []

  for (const m of targets.matchAll(/ - ([a-z0-9_-]+)/g)) {
    const unit = m[1]
    if (unit.endsWith("_test")) {
      tests.push(unit)
    } else if (!unit.startsWith("lib")) {
      commands.push(unit)
    }
    units.push(unit)
  }

  return { units, commands, tests }
}

import { readFileSync, writeFileSync, statSync } from "fs"

import { getTundraTargets } from "./tundra.mjs"

const { units, tests, commands } = getTundraTargets()

/**
 * @typedef {Object} Task
 * @property {string} label
 * @property {string} detail
 */

function isGenerated(/** @type {Task} */ task) {
  return task.detail === "generated"
}

function makeBuildTask(/** @type {string} */ unit) {
  return {
    label: unit,
    type: "shell",
    command: "tundra2",
    args: ["win64-msvc", unit],
    problemMatcher: {
      owner: "cpp",
      fileLocation: "absolute",
      pattern: {
        regexp: "^([a-z]:[^:]+)\\((\\d+)\\): (fatal error|error|warning) (.*)$",
        file: 1,
        line: 2,
        severity: 3,
        message: 4,
      },
    },
    group: {
      kind: "build",
    },
    detail: "generated",
  }
}

function isFile(fn) {
  try {
    return statSync(fn).isFile()
  } catch {
    return false
  }
}

/**
 * Patch `tasks.json` build targets
 */
function patchTasks() {
  const VSCODE_TASKS_JSON = "./.vscode/tasks.json"

  /** @type {{tasks: Task[]}} */
  const tasks = isFile(VSCODE_TASKS_JSON) ? JSON.parse(readFileSync(VSCODE_TASKS_JSON, "utf-8")) : { tasks: [] }

  /** @type {Map<string, Task>} */
  const map = new Map()

  for (const task of tasks.tasks) {
    map.set(task.label, task)
  }

  for (const [label, task] of map) {
    if (isGenerated(task)) {
      if (units.includes(label)) {
        Object.assign(task, makeBuildTask(label)) // update
      } else {
        tasks.tasks.splice(tasks.tasks.indexOf(task), 1)
      }
    }
  }

  for (const unit of units) {
    if (!map.has(unit)) {
      tasks.tasks.push(makeBuildTask(unit))
    }
  }

  tasks.tasks.sort((a, b) => a.label.localeCompare(b.label))

  writeFileSync(VSCODE_TASKS_JSON, JSON.stringify(tasks, undefined, 2))
}

// ---

function makeLaunch(/** @type {string} */ unit) {
  return {
    name: unit,
    type: "cppvsdbg",
    request: "launch",
    cwd: "${workspaceRoot}",
    program: "${workspaceRoot}/t2-output/win64-msvc-debug-default/" + unit + ".exe",
    symbolSearchPath: "${workspaceRoot}/t2-output/win64-msvc-debug-default",
    console: "integratedTerminal",
    logging: {
      moduleLoad: false,
      trace: true,
    },
    preLaunchTask: unit,
    visualizerFile: "${workspaceFolder}/game.natvis",
  }
}

/**
 * @typedef {Object} Configuration
 * @property {string} name
 */

/**
 * Replace `launch.json` debug targets
 */
function replaceLaunch() {
  const VSCODE_LAUNCH_JSON = "./.vscode/launch.json"

  /** @type {{configurations: Configuration[]}} */
  const launch = isFile(VSCODE_LAUNCH_JSON)
    ? JSON.parse(readFileSync(VSCODE_LAUNCH_JSON, "utf-8"))
    : { configurations: [] }

  launch.configurations = [] // nuke

  for (const test of tests) {
    launch.configurations.push(makeLaunch(test))
  }

  for (const cmd of commands) {
    launch.configurations.push(makeLaunch(cmd))
  }

  launch.configurations.sort((a, b) => a.name.localeCompare(b.name))

  writeFileSync(VSCODE_LAUNCH_JSON, JSON.stringify(launch, undefined, 2))
}

// ---

patchTasks()
replaceLaunch()

import { execSync } from "child_process"
import { join } from "path"
import { platform } from "os"
import { readFileSync, writeFileSync } from "fs"

import { getTundraTargets } from "./tundra.mjs"

let config = platform() == "win32" ? "win64-msvc" : platform() == "linux" ? "linux-clang" : ""

const targets = getTundraTargets()

const options = process.argv.slice(2)

let buildId = ""

const tundra = ["tundra2"]

if (options[0] == "-release") {
  buildId = `${config}-release-default`
  options.shift()
} else {
  buildId = `${config}-debug-default`
}

if (options[0] == "-verbose") {
  tundra.push("-v")
  options.shift()
}

const filter = options[0] ?? ""

// trying to cache the test run is not going to speed anything up
// because we need to invoke tundra to know if the build is up to date
// and that is the slowest part of all this
//
// we use filtering instead when we only want to run a small part of the
// test suite

for (const test of targets.tests) {
  if (test.startsWith(filter)) {
    try {
      execSync(`${tundra.join(" ")} ${buildId} ${test}`, { stdio: "inherit" })
    } catch {
      console.error("cannot build test", test)
      process.exit(1) // continue on error true or false
    }

    console.log("running", test, "...")

    try {
      // need to use join because forward slash will not work with execSync on Windows
      const command = join("t2-output", buildId, test)
      execSync(command, {
        stdio: "inherit",
      })
    } catch (err) {
      console.error(
        "test exited with non-zero exit code",
        err.status,
        typeof err.status === "number" ? "(0x" + err.status.toString(16) + ")" : ""
      )
      process.exit(1) // continue on error true or false?
    }
  }
}

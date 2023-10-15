// @ts-check

import { posix } from "path"
import { readFileSync, readdirSync, writeFileSync } from "fs"
import { isDirectory } from "./file-utils.mjs"
import { ConfigFilter, luaStringify } from "./lua.mjs"

// Tundra does not recursively propagate dependencies for static libraries
// https://github.com/deplinenoise/tundra/issues/44
// https://github.com/deplinenoise/tundra/issues/44#issuecomment-23216579
// https://github.com/deplinenoise/tundra/issues/195#issuecomment-22954568

// (node:17660) ExperimentalWarning: Importing JSON modules is an experimental feature. This feature could change at any time
import implicitUnits from "./implicit-units.json" assert { type: "json" }

const IMPLICIT_UNITS = new Set(Object.values(implicitUnits.headers).flat())

const t0 = performance.now()

const root = "src"

// main.cc, foo_test.cc foo_windows.cc foo_linux.cc, *.cc, *.c, *.hh, *.h
const FILE_PATTERN = /(?:(?<main>main\.cc)|(?:_(?<variant>test|windows|linux)\.cc))|(?<c>\.cc?)|(?<h>\.hh?)$/

// see https://pkg.go.dev/go/build#hdr-Build_Constraints

/**
 * @typedef {Object} FilePatternGroups
 * @property {string} main
 * @property {string} variant
 * @property {string} c
 * @property {string} h
 */

function execFilePattern(file) {
  FILE_PATTERN.lastIndex = 0
  const m = FILE_PATTERN.exec(file)
  if (m) {
    const groups = /** @type {FilePatternGroups} */ (m.groups)
    return groups
  }
  return undefined
}

/**
 * @typedef {Object} Package
 * @property {string} name
 * @property {string} path
 * @property {string} main
 * @property {string[]} headers
 * @property {string[]} source
 * @property {string[]} tests
 * @property {string[]} windows
 * @property {string[]} windowsDeps
 * @property {string[]} linux
 * @property {string[]} linuxDeps
 * @property {string[]} deps
 */

/** @type {Map<string, Package>} */
const pkgs = new Map()

for (const pkgName of readdirSync(root)) {
  /** @type {Package} */
  const pkg = {
    name: pkgName,
    path: posix.join(root, pkgName),
    main: "",
    headers: [],
    source: [],
    tests: [],
    windows: [],
    windowsDeps: [], // build constraints?
    linux: [],
    linuxDeps: [],
    deps: [],
  }

  if (!isDirectory(pkg.path)) {
    continue
  }

  for (const file of readdirSync(pkg.path)) {
    const m = execFilePattern(file)
    if (m) {
      if (m.main) {
        pkg.main = posix.join(pkg.path, file)
        continue
      }

      switch (m.variant) {
        case "test":
          pkg.tests.push(posix.join(pkg.path, file))
          continue
        case "windows":
          pkg.windows.push(posix.join(pkg.path, file)) // Windows specific builds
          continue
        case "linux":
          pkg.linux.push(posix.join(pkg.path, file)) // Linux specific builds
          continue
      }

      if (m.c) {
        pkg.source.push(posix.join(pkg.path, file))
        continue
      }

      if (m.h) {
        pkg.headers.push(posix.join(pkg.path, file))
        continue
      }
    }
  }

  pkgs.set(pkg.name, pkg)
}

// Package dependency analysis

const INCLUDE_PATTERN = /^#include (?:\"(?<rel>[^\"]+)\"|<(?<ext>[^>]+)>)/gm

/**
 * @typedef {Object} IncludePatternGroups
 * @property {string} rel
 * @property {string} ext
 */

const PKG_PREFIX = "../"

/**
 * @typedef {Object} Dependency
 * @property {string} pkg
 * @property {string} rel
 * @property {string} ext
 */

function analyzeFileDependencies(filename) {
  const source = readFileSync(filename, "utf-8")

  /** @type {Dependency[]} */
  const deps = []

  for (const m of source.matchAll(INCLUDE_PATTERN)) {
    const groups = /** @type {IncludePatternGroups} */ (m.groups)

    const rel = groups.rel ?? ""
    if (rel) {
      if (rel.startsWith(PKG_PREFIX)) {
        const end = rel.indexOf("/", PKG_PREFIX.length)
        if (0 < end) {
          const pkg = rel.slice(PKG_PREFIX.length, end)
          if (pkgs.has(pkg)) {
            deps.push({ pkg, rel, ext: "" })
            continue
          } else {
            console.warn("warn: unknown package dependency", pkg, "in", filename)
          }
        }
      }
      deps.push({ pkg: "", rel, ext: "" })
      continue
    }

    const ext = groups.ext

    const implicitDep = implicitUnits.headers[ext]
    if (implicitDep) {
      if (Array.isArray(implicitDep)) {
        for (const dep of implicitDep) {
          deps.push({ pkg: dep, rel: "", ext })
        }
      } else {
        deps.push({ pkg: implicitDep, rel: "", ext })
      }
      continue
    }

    deps.push({ pkg: "", rel: "", ext })
  }

  return deps
}

function analyzePackageDependencies(/** @type {string[]} */ sources, /** @type {string[]} */ deps) {
  for (const filename of sources) {
    for (const dep of analyzeFileDependencies(filename)) {
      if (dep.pkg) {
        if (!deps.includes(dep.pkg)) {
          deps.push(dep.pkg)
        }
      }
    }
  }
  return deps
}

for (const [, pkg] of pkgs) {
  pkg.deps = analyzePackageDependencies(pkg.headers, pkg.deps)
  pkg.deps = analyzePackageDependencies(pkg.source, pkg.deps)
  pkg.deps = analyzePackageDependencies(pkg.windows, pkg.deps)
  pkg.deps = analyzePackageDependencies(pkg.linux, pkg.deps)

  if (pkg.main) {
    pkg.deps = analyzePackageDependencies([pkg.main], pkg.deps)
  }

  pkg.deps.sort((a, b) => a.localeCompare(b))
}

// Check for circular dependencies

function checkDependencies(root, dep, visited) {
  // console.debug("checkDependencies", root.name, dep, visited)
  if (visited.includes(dep)) {
    throw new Error(`package ${root.name} has circular dependency ${[...visited, dep].join(" -> ")}`)
  }
  const depPkg = pkgs.get(dep)
  if (!depPkg) {
    if (IMPLICIT_UNITS.has(dep)) {
      return // ignore
    }
    throw new Error(`package dependency ${dep} not found`)
  }
  for (const depDep of depPkg.deps) {
    checkDependencies(root, depDep, [...visited, dep])
  }
}

for (const [, pkg] of pkgs) {
  for (const dep of pkg.deps) {
    checkDependencies(pkg, dep, [])
  }
}

console.debug(pkgs)

// Tundra Propagate doesn't work recursively, only on direct dependants therefore flatten.
// We've already checked for cycles so we don't need to do that again.
function flattenDependencies(/** @type {Package} */ pkg, deps) {
  for (const dep of pkg.deps) {
    if (!deps.includes(dep)) {
      deps.push(dep)
    }
    const depPkg = pkgs.get(dep)
    if (depPkg) {
      deps = flattenDependencies(depPkg, deps)
    }
  }
  return deps
}

let units = ""

units += "\n"
units += "-- Generated by scripts/generate-units.mjs. DO NOT EDIT!\n"
units += "\n"

for (const [, pkg] of pkgs) {
  const deps = flattenDependencies(pkg, [])

  // implicit units can define build constraints
  for (let i = 0; i < deps.length; i++) {
    const dep = deps[i]
    const unit = implicitUnits.units[dep]
    if (unit) {
      if (unit.config) {
        deps[i] = new ConfigFilter(dep, unit.config)
      }
    }
  }

  /** @type {(string|ConfigFilter)[]} */
  const sources = [...pkg.source]

  for (const windows of pkg.windows) {
    sources.push(new ConfigFilter(windows, "win64-*-*"))
  }

  for (const windows of pkg.linux) {
    sources.push(new ConfigFilter(windows, "linux-*-*"))
  }

  if (pkg.main) {
    const unit = {
      ["Name"]: pkg.name,
      ["Depends"]: deps,
      ["Sources"]: [...sources, pkg.main],
    }

    units += "Program" + " " + luaStringify(unit) + "\n\n"
  } else if (0 < sources.length) {
    const unit = {
      ["Name"]: pkg.name,
      ["Depends"]: deps,
      ["Sources"]: sources,
    }

    units += "StaticLibrary" + " " + luaStringify(unit) + "\n\n"
  }

  for (const test of pkg.tests) {
    const testUnit = {
      ["Name"]: pkg.name + "_" + posix.basename(test, posix.extname(test)),
      ["Depends"]: [...deps, pkg.name, "test"],
      //                     ^^^^^^^^
      //                     if 0 < sources.length
      ["Sources"]: [test],
    }

    units += "Program" + " " + luaStringify(testUnit) + "\n\n"
  }
}

writeFileSync("units-generated.lua", units)

console.debug("done in", performance.now() - t0, "milliseconds") // ~20 milliseconds

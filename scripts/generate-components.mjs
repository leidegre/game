import { resolve, dirname, basename, extname, join } from "path"
import { isDataType } from "./component-types.mjs"
import { writeFileSync } from "fs"

const argv = process.argv.slice(2) // ["node", ".\scripts\generate-components.mjs"]

const fn = resolve(argv[0])
const dir = dirname(fn)
const base = basename(fn, extname(fn))

const hhFilename = join(dir, base + ".hh")
const ccFilename = join(dir, base + ".cc")

console.debug({ fn, dir, base, hhFilename, ccFilename })

import("file:///" + resolve(argv[0])).then(
  (/**@type {{[key: string]:import("./component-types.mjs").DataComponent}}*/ exports) => {
    let componentTypeId = 1

    /**@type {Map<import("./component-types.mjs").DataComponent, {name: string, typeId: number}>}*/
    const componentTypeMap = new Map()

    for (const [name, v] of Object.entries(exports)) {
      componentTypeMap.set(v, { name, typeId: componentTypeId++ })
    }

    console.debug(componentTypeMap)

    const DISCLAIMER = "// Generated by generate-components.mjs. DO NOT EDIT!\n\n"

    let typeInfoAccessor = "GetComponentTypeInfoArray"

    function getMemberName(name) {
      return name + "_"
    }

    let hh = "#pragma once\n\n"
    hh += DISCLAIMER
    hh += '#include "../ecs/type-system.hh"\n'
    hh += "\n"
    hh += "namespace game {\n"
    for (const [component, meta] of componentTypeMap) {
      hh += "struct " + meta.name + " {\n"
      hh += "  enum { COMPONENT_TYPE = " + meta.typeId + " };\n"
      hh += "\n"
      for (const [name, desc] of Object.entries(component.members)) {
        if (isDataType(desc)) {
          hh += "  " + desc.description + " " + getMemberName(name) + ";\n"
        } else {
          throw new Error(name + " unsupported member descriptor " + String(desc))
        }
      }
      hh += "};\n\n"
    }

    hh += "Slice<const TypeInfo> " + typeInfoAccessor + "();\n"
    hh += "} // namespace game\n"

    let cc = '#include "' + basename(hhFilename) + '"\n\n'
    cc += DISCLAIMER
    cc += "using namespace game;\n\n"
    cc += "Slice<const TypeInfo> game::" + typeInfoAccessor + "() {\n"
    cc += "  static const TypeInfo components[] = {\n"
    cc += "    GAME_COMPONENT(Entity),\n"
    for (const [_, meta] of componentTypeMap) {
      cc += "    GAME_COMPONENT(" + meta.name + "),\n"
    }
    cc += "  };\n"
    cc += "  return slice::FromArray(components);\n"
    cc += "}\n"

    writeFileSync(hhFilename, hh)
    writeFileSync(ccFilename, cc)
  }
)
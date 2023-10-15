// @ts-check

export class ConfigFilter {
  /**
   * @param {unknown} value
   * @param {string} config
   */
  constructor(value, config) {
    this.value = value
    this.config = config
  }
}

/**
 * @param {unknown} v
 */
export function luaStringify(v, indent = 0) {
  switch (typeof v) {
    case "object":
      if (v === null) {
        return "nil"
      }
      let s = ""
      s += "{"
      if (Array.isArray(v)) {
        const entries = v
        for (let i = 0; i < entries.length; i++) {
          const v = entries[i]
          s += "\n"
          s += " ".repeat(4 * (indent + 1))
          s += luaStringify(v, indent + 1)
          s += i + 1 < entries.length ? "," : ""
        }
        if (0 < entries.length) {
          // non-empty
          s += "\n"
          s += " ".repeat(4 * indent)
        }
      } else {
        if (v instanceof ConfigFilter) {
          return `{ ${luaStringify(v.value)}; Config = ${luaStringify(v.config)} }`
        }
        const entries = Object.entries(v)
        for (let i = 0; i < entries.length; i++) {
          const ent = entries[i]
          const k = ent[0]
          const v = ent[1]
          s += "\n"
          s += " ".repeat(4 * (indent + 1))
          s += k
          s += " = "
          s += luaStringify(v, indent + 1)
          s += i + 1 < entries.length ? "," : ""
        }
        if (0 < entries.length) {
          // non-empty
          s += "\n"
          s += " ".repeat(4 * indent)
        }
      }
      s += "}"
      return s
    case "boolean":
    case "string":
    case "number":
      return JSON.stringify(v)
  }
  return "nil--[[" + typeof v + "--]]"
}

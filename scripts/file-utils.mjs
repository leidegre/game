import { statSync } from "fs"

export function isDirectory(path) {
  try {
    return statSync(path).isDirectory()
  } catch {
    return false
  }
}

export function isFile(path) {
  try {
    return statSync(path).isFile()
  } catch {
    return false
  }
}

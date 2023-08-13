export const bool = Symbol("bool")
export const byte = Symbol("byte")
export const f32 = Symbol("f32")
export const f64 = Symbol("f64")
export const i16 = Symbol("i16")
export const i32 = Symbol("i32")
export const i64 = Symbol("i64")
export const u16 = Symbol("u16")
export const u32 = Symbol("u32")
export const u64 = Symbol("u64")
export const vec3 = Symbol("vec3")
export const vec4 = Symbol("vec4")
export const mat4 = Symbol("mat4")
export const quat = Symbol("quat")

// export const string = Symbol("string") // fixed size

export const DATA_TYPES = new Set([bool, byte, f32, f64, i16, i32, i64, u16, u32, u64, vec3, vec4, mat4, quat])

/**
 * @typedef {typeof bool|typeof byte|typeof f32|typeof f64|typeof i16|typeof i32|typeof i64|typeof u16|typeof u32|typeof u64|typeof vec3|typeof vec4|typeof mat4} DataType
 */

/**
 * @typedef {DataType | { type: DataType, comment?: string }} TypeAnnotation
 */

/**
 * @typedef {{ [name: string]: TypeAnnotation }} DataMemberObject
 */

export class DataComponent {
  constructor(/**@type {DataMemberObject}*/ members) {
    this.members = members
  }
}

/**@type {(x: TypeAnnotation) => x is DataType}*/
export const isDataType = (x) => {
  return DATA_TYPES.has(x)
}

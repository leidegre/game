// This is where we define all built-in system components
// non specific game components

import { DataComponent, f32, mat4, quat, vec3 } from "../../scripts/component-types.mjs"

export const Translation = new DataComponent({ value: vec3 })
export const Rotation = new DataComponent({ value: quat })
export const Scale = new DataComponent({ value: f32 })
export const LocalToWorld = new DataComponent({ value: mat4 })

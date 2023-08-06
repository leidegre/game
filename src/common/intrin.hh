#pragma once

#include "mem.hh"

#if _WIN32
// __x86_64__
#include <intrin.h>
#elif __clang__
#include <x86intrin.h>
#endif

namespace game {
// Count the number of trailing zero bits in unsigned 64-bit integer
// https://en.wikipedia.org/wiki/X86_Bit_manipulation_instruction_set BMI1
inline u64 tzcnt_u64(u64 v) {
  // https://stackoverflow.com/questions/62348210/bitscanforward64-can-not-be-found#comment110268539_62348210
  return _tzcnt_u64(v);
}
} // namespace game

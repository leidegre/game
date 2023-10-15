#pragma once

#include <cstring>

namespace game {
inline bool stringHasPrefix(const char* s, const char* prefix) {
  if (strlen(prefix) <= strlen(s)) {
    return strncmp(s, prefix, strlen(prefix)) == 0;
  }
  return false;
}
} // namespace game

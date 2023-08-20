#include "../test/test.h"

#include "file.hh"

#include "mem.hh"

#include <cstdio>

using namespace game;

namespace {
void HexDump(byte* buf, i32 buf_len) {
  const char* nibble = "0123456789ABCDEF";
  // 00000000 00000000 00000000 00000000 ................
  char s[4 * 8 + 4 + 16 + 1];
  s[sizeof(s) - 1] = '\0';
  for (i32 i = 0; i < buf_len; i += 16) {
    byte* stride = buf + i;

    memset(s, ' ', sizeof(s) - 1);

    for (i32 j = 0, len = 16 <= buf_len - i ? 16 : buf_len - i; j < len; j++) {
      byte b = stride[j];

      s[j / 4 + 2 * j]     = nibble[b >> 4];
      s[j / 4 + 2 * j + 1] = nibble[b & 15];

      s[4 * 8 + 4 + j] = ((0x21 <= b) & (b <= 0x7E)) ? b : '.';
    }

    puts(s);
  }
};
} // namespace

int main(int argc, char* argv[]) {
  test_init(argc, argv);

  TEST_CASE("FileOpenRead") {
    Error      err;
    ScopedFile f;

    ASSERT_TRUE((err = FileOpenRead("data/textures/debug/xz-grid-1024.png", &f)).Ok());

    i64 size;
    ASSERT_TRUE((err = FileSize(f, &size)).Ok());

    byte* buffer = MemAlloc(MEM_ALLOC_HEAP, (i32)size, 16);

    ASSERT_TRUE((err = FileRead(f, (i32)size, buffer)).Ok());

    HexDump(buffer, 64);

    MemFree(MEM_ALLOC_HEAP, buffer);
  }
}

#include "../test/test.h"

#include "cli.hh"

#include <cstdlib>

using namespace game;

int main(int argc, char* argv[]) {
  test_init(argc, argv);

  TEST_CASE("CliTest") {
    CommandLineInterface cli{};

    const char* foo;
    cli.AddStr(&foo, "foo");
    const char* argv[] = { "", "-foo", "bar" };
    ASSERT_TRUE(cli._Parse(_countof(argv), argv) == CommandLineInterface::PARSE_OK);
    ASSERT_EQUAL_STR("bar", foo);

    cli.Destroy();
  }
}

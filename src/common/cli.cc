#include "cli.hh"

#include "string.hh"

#include <cstdio>
#include <cstdlib>

using namespace game;

CommandLineInterface::Error CommandLineInterface::_Parse(int argc, const char* const* argv) {
  int i = 1;
_ParseNext:
  if (i < argc) {
    const char* name = argv[i];
    if (stringHasPrefix(name, "-")) {
      name++;
      if (stringHasPrefix(name, "-")) {
        name++;
        if (stringHasPrefix(name, "-")) {
          fprintf(stderr, "option %s has too many -", argv[i]);
          return PARSE_TOO_MANY;
        }
      }
      if (strlen(name) == 0) {
        return PARSE_OK;
      }
      for (auto opt : options_) {
        if (strcmp(name, opt.name_) == 0) {
          // boolean flag may be exempt from this rule, or if we support -opt=val form
          if (!(i + 1 < argc)) {
            fprintf(stderr, "option -%s is missing required argument", name);
            return PARSE_MISSING_ARGUMENT;
          }
          // parse!
          switch (opt.typ_) {
          case CommandLineOption::TYPE_STR:
            ((const char**)opt.ptr_)[0] = argv[i + 1];
            break;
          default:
            return PARSE_ERROR;
          }
          i += 2;
          goto _ParseNext;
        }
      }
      fprintf(stderr, "option -%s is not recognized", name);
      return PARSE_UNKNOWN_OPTION;
    }
  }
  return PARSE_OK;
}

void CommandLineInterface::MustParse(int argc, char** argv) {
  if (Parse(argc, argv) != PARSE_OK) {
    exit(2);
  }
}
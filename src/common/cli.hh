#pragma once

#include "list.hh"

namespace game {
struct CommandLineOption {
  enum Type {
    TYPE_STR,
  };

  Type        typ_;
  void*       ptr_;
  const char* name_;
  const char* description_;
};

struct CommandLineInterface {
  enum Error {
    PARSE_OK,
    PARSE_TOO_MANY,
    PARSE_UNKNOWN_OPTION,
    PARSE_MISSING_ARGUMENT,
    PARSE_BAD_ARGUMENT,
    PARSE_ERROR,
  };

  List<CommandLineOption> options_;

  void _Add(CommandLineOption::Type typ, void* ptr, const char* name, const char* description = nullptr) {
    options_.Add({ typ, ptr, name, description });
  }

  void AddStr(const char** s, const char* name, const char* description = nullptr) {
    _Add(CommandLineOption::TYPE_STR, s, name, description);
  }

  Error _Parse(int argc, const char* const* argv);

  Error Parse(int argc, char** argv) { return _Parse(argc, argv); }

  void MustParse(int argc, char** argv);

  void Destroy() { options_.Destroy(); }
};
} // namespace game
#pragma once

#include "type-system.hh"

namespace game {
enum ErrorCode {
  GRR_OK = 0, // not an error
  GRR_ERROR,  // generic error
  GRR_NOT_FOUND,
  GRR_FILE_OPEN, // cannot open file
  GRR_FILE_READ, // cannot read file
  GRR_FILE_SIZE, // file size is too small (or too large)
  GRR_FILE_TYPE, // file type is unrecognized
  GRR_PNG,       // error from libpng
};

struct Error {
  ErrorCode code_;

  Error() { code_ = GRR_OK; }
  Error(ErrorCode code) { code_ = code; }

  bool Ok() const { return code_ == 0; }
  bool HasError() const { return !(code_ == 0); }
};
}; // namespace game
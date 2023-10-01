#pragma once

#include "type-system.hh"

// #ifndef _HRESULT_DEFINED
// #define _HRESULT_DEFINED
// #ifdef __midl
// typedef LONG HRESULT;
// #else
// typedef _Return_type_success_(return >= 0) long HRESULT;
// #endif // __midl
// #endif // !_HRESULT_DEFINED

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

  // u32 result_; // depends on code_

  Error() { code_ = GRR_OK; }
  Error(ErrorCode code) { code_ = code; }

  // ifdef _WINNT_
  // Error(HRESULT result) { code_ = code; }

  bool Ok() const { return code_ == 0; }
  bool HasError() const { return !(code_ == 0); }
};
}; // namespace game
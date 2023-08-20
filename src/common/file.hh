#pragma once

#include "errors.hh"

namespace game {
struct File;

Error FileOpenRead(const char* filename, File** file);

// Get file size
Error FileSize(File* file, i64* size);

// Read N bytes from file into buffer.
Error FileRead(File* file, const i32 n, void* buf);

// Close file
Error FileClose(File* file);

// a non-copyable file ptr
struct ScopedFile {
  File* file_;
  ScopedFile() { file_ = nullptr; }
  ScopedFile(File* file) { file_ = file; }
  ScopedFile(const ScopedFile&) = delete;
  ~ScopedFile() { FileClose(file_); }
  File* Get() const {
    File* tmp = file_;
    assert(tmp);
    return tmp;
  }
  File* Reset() {
    File* tmp = file_;
    file_     = nullptr;
    return tmp;
  }
};

inline Error FileOpenRead(const char* filename, ScopedFile* file) {
  return FileOpenRead(filename, &file->file_);
}

inline Error FileSize(const ScopedFile& file, i64* size) {
  return FileSize(file.Get(), size);
}

inline Error FileRead(const ScopedFile& file, const i32 n, void* buf) {
  return FileRead(file.Get(), n, buf);
}
} // namespace game
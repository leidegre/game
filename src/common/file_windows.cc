#include "file.hh"

#include "mem.hh"

#include <Windows.h> // todo: use a common include file for windows...

using namespace game;

struct game::File {
  const char* filename_;
  HANDLE      handle_;
};

Error game::FileOpenRead(const char* filename, File** file) {
  WCHAR w_filename[512];
  MultiByteToWideChar(CP_UTF8, 0, filename, -1, w_filename, 512);

  WCHAR w_filename_abs[512];
  GetFullPathName(w_filename, 512, w_filename_abs, NULL);

  // expand file name

  const HANDLE h = CreateFile(w_filename_abs, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, nullptr);
  if (h == INVALID_HANDLE_VALUE) {
    const DWORD win32_err = GetLastError();
    if (win32_err == ERROR_FILE_NOT_FOUND) {
      return GRR_NOT_FOUND;
    }
    // todo: log this Win32 error
    return GRR_FILE_OPEN;
  }

  *file              = MemAlloc<File>(MEM_ALLOC_HEAP);
  file[0]->filename_ = filename;
  file[0]->handle_   = h;

  return GRR_OK;
}

Error game::FileSize(File* file, i64* size) {
  BY_HANDLE_FILE_INFORMATION file_info;
  GetFileInformationByHandle(file->handle_, &file_info);
  ULARGE_INTEGER u_file_size;
  u_file_size.LowPart  = file_info.nFileSizeLow;
  u_file_size.HighPart = file_info.nFileSizeHigh;
  *size                = (i64)u_file_size.QuadPart;
  return GRR_OK;
}

Error game::FileRead(File* file, i32 n, void* buffer) {
  DWORD read;
  if (::ReadFile(file->handle_, buffer, n, &read, nullptr)) {
    if (read == (DWORD)n) {
      return GRR_OK;
    }
  }
  return GRR_FILE_READ;
}

Error game::FileClose(File* file) {
  if (file != nullptr) {
    if (file->handle_ != nullptr) {
      CloseHandle(file->handle_);
      file->handle_ = nullptr;
    }
    MemFree(MEM_ALLOC_HEAP, file);
  }
  return GRR_OK;
}
#include "tex-loader.hh"

#include "../common/file.hh"

#include <png.h>

using namespace game;

namespace {
// libpng stuff
struct game_png_read_data {
  const png_byte* src_;
  size_t          off_;
  size_t          len_;
};

static void* game_png_malloc(png_struct* png_ptr, size_t size) {
  Allocator allocator = (Allocator)(intptr_t)png_get_mem_ptr(png_ptr);
  return MemAllocZeroInit(allocator, size, 16);
}

static void game_png_free(png_struct* png_ptr, void* ptr) {
  Allocator allocator = (Allocator)(intptr_t)png_get_mem_ptr(png_ptr);
  MemFree(allocator, ptr);
}

static void game_png_read(png_struct* png_ptr, png_byte* dst, size_t len) {
  game_png_read_data* rd = (game_png_read_data*)png_get_io_ptr(png_ptr);
  memcpy(dst, rd->src_ + rd->off_, len);
  rd->off_ += len;
}
} // namespace

Error game::LoadTextureFromFile(Allocator allocator, const char* filename, TextureAsset* tex_asset) {
  Error      err;
  ScopedFile f;
  i64        f_size;

  err = FileOpenRead(filename, &f);
  if (err.HasError()) {
    return err;
  }

  err = FileSize(f, &f_size);
  if (err.HasError()) {
    return err;
  }

  byte* buf = MemAlloc(allocator, (i32)f_size, 16);

  err = FileRead(f, (i32)f_size, buf);
  if (err.HasError()) {
    MemFree(allocator, buf);
    return err;
  }

  if (f_size < 16) {
    MemFree(allocator, buf);
    return GRR_FILE_SIZE;
  }

  tex_asset->asset_type_      = ASSET_TYPE_TEXTURE;
  tex_asset->asset_allocator_ = allocator;
  tex_asset->asset_filename_  = filename;

  u32 magic_number = ((u32*)buf)[0];
  switch (magic_number) {
  case 0x474E5089U: { // PNG
    return LoadTextureFromPNG(allocator, { buf, 0, (i32)f_size }, tex_asset);
  }
  default: {
    return GRR_FILE_TYPE;
  }
  }
}

Error game::LoadTextureFromPNG(Allocator allocator, Slice<const byte> data, TextureAsset* tex_asset) {
  png_struct* png_ptr = png_create_read_struct_2(
      PNG_LIBPNG_VER_STRING, //
      NULL,
      NULL,
      NULL,
      (png_voidp)allocator,
      game_png_malloc,
      game_png_free);
  if (!png_ptr) {
    return GRR_PNG;
  }

  game_png_read_data rd = { data.ptr_, 0, size_t(data.len_) };
  png_set_read_fn(png_ptr, &rd, game_png_read);

  png_info* info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    return GRR_PNG;
  }

  // png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
  png_read_info(png_ptr, info_ptr);

  png_uint_32 w          = png_get_image_width(png_ptr, info_ptr);
  png_uint_32 h          = png_get_image_height(png_ptr, info_ptr);
  png_byte    color_type = png_get_color_type(png_ptr, info_ptr);
  png_byte    bit_depth  = png_get_bit_depth(png_ptr, info_ptr);
  png_byte    channels   = png_get_channels(png_ptr, info_ptr);
  size_t      rowbytes   = png_get_rowbytes(png_ptr, info_ptr);

  if ((color_type == PNG_COLOR_TYPE_RGBA) & (bit_depth == 8) & (channels == 4)) {
    byte** rows = MemAllocArray<byte*>(MEM_ALLOC_TEMP, h);
    byte*  data = MemAlloc(MEM_ALLOC_HEAP, h * rowbytes, 16);
    for (u32 i = 0; i < h; i++) {
      rows[i] = data + i * rowbytes;
    }

    png_read_image(png_ptr, rows);

    // todo: cleanup all libpng stuff, destroy everything

    tex_asset->tex_type_      = GAME_TEXTURE_RGBA_8;
    tex_asset->tex_width_     = w;
    tex_asset->tex_height_    = h;
    tex_asset->tex_data_      = data;
    tex_asset->tex_data_size_ = w * h * 4;
    return GRR_OK;
  }

  return GRR_PNG;
}

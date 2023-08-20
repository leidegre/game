#pragma once

#include "../common/errors.hh"
#include "../common/mem.hh"

namespace game {
enum AssetType {
  ASSET_TYPE_NONE,
  ASSET_TYPE_TEXTURE,
};

struct Asset {
  AssetType   asset_type_;
  Allocator   asset_allocator_;
  const char* asset_filename_; // if loaded from a file
};

enum TextureType {
  GAME_TEXTURE_NONE,
  GAME_TEXTURE_RGBA_8,
};

struct TextureAsset : public Asset {
  TextureType tex_type_;
  u32         tex_width_;
  u32         tex_height_;
  byte*       tex_data_;
  u32         tex_data_size_; // size of texture data in bytes
};

Error LoadTextureFromFile(Allocator allocator, const char* filename, TextureAsset* tex_asset);

Error LoadTextureFromPNG(Allocator allocator, Slice<const byte> data, TextureAsset* tex_asset);
} // namespace game
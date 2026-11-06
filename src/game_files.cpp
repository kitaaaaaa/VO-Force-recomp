// Locating the player's game files.
//
// The game ships on a disc. Players drop an image of it into assets/; on first
// launch it is unpacked next to itself with the SDK's disc reader, and later
// launches use the unpacked files directly. DLC packages found there are
// unpacked the same way into the content folder.

#include "game_files.h"

#include <rex/filesystem/devices/disc_image_device.h>
#include <rex/filesystem/devices/stfs_container_device.h>
#include <rex/filesystem/entry.h>
#include <rex/filesystem/file.h>
#include <rex/logging.h>

#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

namespace fs = std::filesystem;

namespace force {
namespace {

// The recompiled code only matches this exact default.xex.
constexpr std::string_view kXexSha256 =
    "9FE5B63F76A089804F8B18D7F53F285E2743BFE5D8B43ED9A7E863DAAD5148C0";

constexpr uint32_t kTitleId = 0x5345084C;         // Virtual-On Force
constexpr uint32_t kDlcContentType = 2;           // marketplace content
constexpr uint32_t kStfsContentTypeOffset = 0x344;  // in the package metadata
constexpr uint32_t kStfsTitleIdOffset = 0x360;
constexpr int kMaxSearchDepth = 6;


bool HasGame(const fs::path& dir) {
  std::error_code ec;
  return fs::is_regular_file(dir / "default.xex", ec) && fs::is_directory(dir / "media", ec);
}

fs::path FindImage(const fs::path& dir) {
  std::error_code ec;
  if (!fs::is_directory(dir, ec)) return {};
  for (const auto& entry : fs::directory_iterator(dir, fs::directory_options::skip_permission_denied, ec)) {
    if (entry.is_regular_file(ec) && _wcsicmp(entry.path().extension().c_str(), L".iso") == 0) {
      return entry.path();
    }
  }
  return {};
}

std::string FileSha256(const fs::path& file) {
  std::ifstream in(file, std::ios::binary);
  if (!in) return {};
  BCRYPT_HASH_HANDLE hash = nullptr;
  if (BCryptCreateHash(BCRYPT_SHA256_ALG_HANDLE, &hash, nullptr, 0, nullptr, 0, 0) != 0) return {};
  std::vector<char> buffer(1 << 20);
  while (in.read(buffer.data(), buffer.size()) || in.gcount() > 0) {
    BCryptHashData(hash, reinterpret_cast<PUCHAR>(buffer.data()), static_cast<ULONG>(in.gcount()), 0);
  }
  std::array<uint8_t, 32> digest{};
  BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0);
  BCryptDestroyHash(hash);
  std::string hex;
  char byte[3];
  for (uint8_t b : digest) {
    std::snprintf(byte, sizeof(byte), "%02X", b);
    hex += byte;
  }
  return hex;
}

bool CopyEntry(rex::filesystem::Entry* entry, const fs::path& out_path, std::string* error) {
  rex::filesystem::File* file = nullptr;
  if (entry->Open(0x80000000 /* GENERIC_READ */, &file) != 0 || !file) {
    *error = "cannot read " + entry->path();
    return false;
  }
  std::ofstream out(out_path, std::ios::binary | std::ios::trunc);
  std::vector<uint8_t> buffer(4 << 20);
  size_t offset = 0;
  bool ok = static_cast<bool>(out);
  while (ok && offset < entry->size()) {
    size_t read = 0;
    if (file->ReadSync(buffer, offset, &read) != 0 || read == 0) {
      ok = false;
      break;
    }
    out.write(reinterpret_cast<const char*>(buffer.data()), read);
    ok = static_cast<bool>(out);
    offset += read;
  }
  file->Destroy();
  // The last buffered write is only flushed here (e.g. a full disk).
  out.close();
  ok = ok && !out.fail();
  if (!ok) *error = "cannot write " + out_path.string();
  return ok;
}

// Copies everything under `root` to `out_dir`. If `xex` is given, a default.xex
// in the top folder is handed back there instead of being copied.
bool CopyTree(rex::filesystem::Entry* root, const fs::path& out_dir, rex::filesystem::Entry** xex,
              std::string* error) {
  std::vector<std::pair<rex::filesystem::Entry*, fs::path>> stack{{root, out_dir}};
  while (!stack.empty()) {
    auto [dir, dir_path] = stack.back();
    stack.pop_back();
    std::error_code ec;
    fs::create_directories(dir_path, ec);
    for (const auto& child : dir->children()) {
      const fs::path child_path = dir_path / child->name();
      if (child->attributes() & rex::filesystem::kFileAttributeDirectory) {
        stack.emplace_back(child.get(), child_path);
      } else if (xex && dir == root && child->name() == "default.xex") {
        *xex = child.get();
      } else if (!CopyEntry(child.get(), child_path, error)) {
        return false;
      }
    }
  }
  return true;
}

// Unpacks the image into `out_dir`. default.xex is written last, so an
// interrupted unpack is not mistaken for a complete one next launch.
bool ExtractImage(const fs::path& image, const fs::path& out_dir, std::string* error) {
  REXLOG_INFO("Unpacking disc image {} into {}", image.string(), out_dir.string());
  rex::filesystem::DiscImageDevice device("\\Device\\Import", image);
  if (!device.Initialize()) {
    *error = "the disc image could not be read (is it damaged?):\n" + image.string();
    return false;
  }
  rex::filesystem::Entry* root = device.ResolvePath("");
  if (!root) {
    *error = "the disc image is empty:\n" + image.string();
    return false;
  }
  rex::filesystem::Entry* xex = nullptr;
  if (!CopyTree(root, out_dir, &xex, error)) return false;
  if (!xex) {
    *error = "the disc image does not contain default.xex:\n" + image.string();
    return false;
  }
  return CopyEntry(xex, out_dir / "default.xex", error);
}

fs::path FindOrExtract(const fs::path& exe_dir, std::string* error) {
  // assets/ next to the exe (release layout), then the repo's assets/ for
  // developer builds in out/build/<preset>/.
  const fs::path candidates[] = {exe_dir / "assets", exe_dir / ".." / ".." / ".." / "assets"};
  for (const auto& dir : candidates) {
    if (HasGame(dir)) return fs::weakly_canonical(dir);
  }
  for (const auto& dir : candidates) {
    if (auto image = FindImage(dir); !image.empty()) {
      if (!ExtractImage(image, dir, error)) return {};
      return fs::weakly_canonical(dir);
    }
  }
  *error = "Virtual-On Force game files not found.\n\nPut your disc image (.iso) into:\n\n" +
           (exe_dir / "assets").string() +
           "\n\nThe extracted files (default.xex and the media folder) will work too.";
  return {};
}

uint32_t ReadBe32(const uint8_t* p) {
  return (uint32_t(p[0]) << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

bool IsDlcPackage(const fs::path& file) {
  std::ifstream in(file, std::ios::binary);
  std::array<uint8_t, kStfsTitleIdOffset + 4> header{};
  if (!in.read(reinterpret_cast<char*>(header.data()), header.size())) return false;
  const std::string_view magic(reinterpret_cast<const char*>(header.data()), 4);
  if (magic != "LIVE" && magic != "PIRS" && magic != "CON ") return false;
  return ReadBe32(header.data() + kStfsTitleIdOffset) == kTitleId &&
         ReadBe32(header.data() + kStfsContentTypeOffset) == kDlcContentType;
}

// The content folder is named after the package file. It is filled under a
// temporary name, so an interrupted install is not mistaken for a complete one.
void InstallPackage(const fs::path& package, const fs::path& content_dir) {
  std::error_code ec;
  const fs::path dest = content_dir / package.filename();
  if (fs::exists(dest, ec)) return;
  REXLOG_INFO("Installing DLC package {} into {}", package.string(), dest.string());
  rex::filesystem::StfsContainerDevice device("\\Device\\Import", package);
  rex::filesystem::Entry* root = device.Initialize() ? device.ResolvePath("") : nullptr;
  if (!root) {
    REXLOG_ERROR("the DLC package could not be read: {}", package.string());
    return;
  }
  fs::path temp = dest;
  temp += ".installing";
  fs::remove_all(temp, ec);
  std::string error;
  if (!CopyTree(root, temp, nullptr, &error)) {
    REXLOG_ERROR("the DLC package could not be installed: {}", error);
    return;
  }
  fs::rename(temp, dest, ec);
  if (ec) REXLOG_ERROR("cannot create {}: {}", dest.string(), ec.message());
}

}  // namespace

fs::path LocateGameFiles(const fs::path& exe_dir, std::string* error) {
  fs::path found = FindOrExtract(exe_dir, error);
  if (found.empty()) return {};
  if (FileSha256(found / "default.xex") != kXexSha256) {
    *error = "This is not the Japanese retail Dennou Senki Virtual-On Force that this build was "
             "made from:\n\n" + (found / "default.xex").string();
    return {};
  }
  return found;
}

void InstallDlc(const fs::path& game_dir, const fs::path& user_data_root) {
  const fs::path content_dir = user_data_root / "0000000000000000" / "5345084C" / "00000002";
  std::error_code ec;
  if (!fs::is_directory(game_dir, ec)) return;
  for (auto it = fs::recursive_directory_iterator(game_dir, fs::directory_options::skip_permission_denied, ec);
       !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
    if (it.depth() >= kMaxSearchDepth || (it.depth() == 0 && it->path().filename() == "media")) {
      it.disable_recursion_pending();
      continue;
    }
    if (it->is_regular_file(ec) && IsDlcPackage(it->path())) {
      fs::create_directories(content_dir, ec);
      InstallPackage(it->path(), content_dir);
    }
  }
}

void ShowError(const std::string& message) {
  const int len = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, nullptr, 0);
  std::wstring wide(len, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, wide.data(), len);
  MessageBoxW(nullptr, wide.c_str(), L"Virtual-On Force", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
}

}  // namespace force

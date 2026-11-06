// Developer tools for working on the recompilation (not used in normal play).

#include "debug_tools.h"

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/system/xmemory.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>

#define NOMINMAX
#include <windows.h>

REXCVAR_DEFINE_STRING(dump_image, "", "Force",
                      "Write the loaded XEX image (0x82000000-0x84200000, big-endian) "
                      "to this file after load, for offline analysis");

REXCVAR_DEFINE_STRING(capture_dir, "", "Force",
                      "Developer: F9 toggles a robot part trace (robots.N.bin) and saves "
                      "texture memory and the robot setup (texram.N.bin, slots.N.bin) to this folder");

REXCVAR_DEFINE_BOOL(english, false, "Force",
                    "Use the English text instead of the original Japanese");

REXCVAR_DEFINE_BOOL(unlock_all_vrs, false, "Force",
                    "Own every VR. The change reaches the save the next time the game saves");

REXCVAR_DEFINE_BOOL(unlock_ex_options, false, "Force",
                    "Always have enough EX OPTIONS points to raise LIFE and ATTACK to their limit");

REXCVAR_DEFINE_BOOL(capture_all, false, "Force",
                    "Developer: a capture records every model, not only robot banks 18-38");

REX_EXTERN(__imp__sub_825B3B60);
REX_EXTERN(__imp__sub_825B3870);
REX_EXTERN(__imp__sub_82413918);
REX_EXTERN(__imp__sub_824145A8);
REX_EXTERN(__imp__sub_82413680);
REX_EXTERN(__imp__sub_825B3668);
REX_EXTERN(__imp__sub_825B3460);
REX_EXTERN(__imp__sub_82463110);

namespace {

// Guest addresses.
constexpr uint32_t kDrawState = 0x82FD4790;      // draw list state
constexpr uint32_t kDrawCountOffset = 2652;      // records used this frame
constexpr uint32_t kDrawRecordsOffset = 2656;    // 260-byte records
constexpr uint32_t kDrawRecordSize = 260;
constexpr uint32_t kDrawRecordMax = 4096;
constexpr uint32_t kTexRamBanks = 0x834A5F38;    // two 16-byte entries, +0 = pixels
constexpr uint32_t kTexRamSize = 0x400000;       // 2048x1024, 16-bit
constexpr uint32_t kRobotFamilies = 0x82B1B188;  // family of each of 4 robot slots
constexpr uint32_t kRobotSetup = 0x82B22560;     // setup of each slot, 40 bytes

constexpr uint32_t kStringTable = 0x82F913D0;    // pointer to the loaded string table
constexpr uint32_t kStringFile = 0x82F913D8;     // its file loader object
constexpr uint32_t kStringTableSize = 73112;     // the buffer: the size of the original file

constexpr uint32_t kOwnedVrTable = 3020;         // in the player profile
constexpr uint32_t kOwnedVrCount = 93;
constexpr uint32_t kExPointsTotal = 20;          // LIFE 10 + ATTACK 10
constexpr uint32_t kBlendRecord = 0x80000000;    // trace: record came from sub_825B3870
constexpr uint32_t kFeiWindowModel = 0x1296 + 108;
constexpr float kFeiWindowDepthBias = 0.5f;      // ring layers lie up to 0.27 in front

// Robot model banks 18..38 (model numbers from the bank table).
constexpr uint32_t kFirstRobotModel = 0x0EBC;
constexpr uint32_t kLastRobotModel = 0x1936;

FILE* g_trace = nullptr;
int g_capture = 0;
uint32_t g_frame = 0;
std::mutex g_trace_mutex;

uint32_t ReadU32(const uint8_t* base, uint32_t address) {
  uint32_t v;
  std::memcpy(&v, base + address, sizeof(v));
  return _byteswap_ulong(v);
}

void WriteFile(const std::string& path, const void* data, size_t size) {
  if (FILE* f = std::fopen(path.c_str(), "wb")) {
    std::fwrite(data, 1, size, f);
    std::fclose(f);
  }
}

// Saves texture memory and the robot setup. Robots load their textures after a
// match begins, so this is done at both the start and the end of a capture.
void SaveState(const uint8_t* base, const std::string& dir, const std::string& name) {
  std::string texram;
  for (uint32_t bank = 0; bank < 2; ++bank) {
    const uint32_t pixels = ReadU32(base, kTexRamBanks + bank * 16);
    if (pixels) texram.append(reinterpret_cast<const char*>(base + pixels), kTexRamSize);
    else texram.append(kTexRamSize, '\0');
  }
  WriteFile(dir + "/texram." + name + ".bin", texram.data(), texram.size());
  std::string slots(reinterpret_cast<const char*>(base + kRobotFamilies), 16);
  slots.append(reinterpret_cast<const char*>(base + kRobotSetup), 160);
  WriteFile(dir + "/slots." + name + ".bin", slots.data(), slots.size());
}

// Saves what a part trace needs to be read later, then opens the trace file.
void StartCapture(const uint8_t* base, const std::string& dir) {
  // Never overwrite the captures of an earlier session.
  while (std::filesystem::exists(dir + "/robots." + std::to_string(g_capture) + ".bin")) {
    ++g_capture;
  }
  const std::string n = std::to_string(g_capture);
  SaveState(base, dir, n);
  g_trace = std::fopen((dir + "/robots." + n + ".bin").c_str(), "wb");
  REXLOG_INFO("capture {} started in {}: {}", g_capture, dir, g_trace ? "ok" : "cannot open file");
  Beep(880, 150);
}

void StopCapture(const uint8_t* base, const std::string& dir) {
  std::fclose(g_trace);
  g_trace = nullptr;
  SaveState(base, dir, std::to_string(g_capture) + ".end");
  REXLOG_INFO("capture {} stopped after {} frames", g_capture, g_frame);
  Beep(440, 300);
  ++g_capture;
  g_frame = 0;
}

// F9 works on every screen: the game does not draw models on pause or menu screens.
void PollCaptureKey(const uint8_t* base, const std::string& dir) {
  bool key_down = false;
  while (true) {
    const bool down = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
    if (down && !key_down) {
      std::lock_guard<std::mutex> lock(g_trace_mutex);
      if (g_trace) StopCapture(base, dir);
      else StartCapture(base, dir);
    }
    key_down = down;
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
}

}  // namespace

// Returns a player's profile: fills in the owned VR table (93 entries of 16
// bytes, first word = number owned; the game starts each default VR at 3).
REX_HOOK_RAW(sub_82413680) {
  __imp__sub_82413680(ctx, base);
  const uint32_t profile = ctx.r3.u32;
  if (!profile || !REXCVAR_GET(unlock_all_vrs)) return;
  static constexpr uint32_t kOwned = 0x03000000;  // big-endian 3
  for (uint32_t i = 0; i < kOwnedVrCount; ++i) {
    uint8_t* entry = const_cast<uint8_t*>(base) + profile + kOwnedVrTable + i * 16;
    uint32_t count;
    std::memcpy(&count, entry, sizeof(count));
    if (!count) std::memcpy(entry, &kOwned, sizeof(kOwned));
  }
}

// Returns the EX OPTIONS record: +0 LIFE, +4 ATTACK, +8 unspent points. Tops up
// the unspent points so that LIFE and ATTACK can both reach their limit of 10.
REX_HOOK_RAW(sub_82413918) {
  __imp__sub_82413918(ctx, base);
  if (!REXCVAR_GET(unlock_ex_options)) return;
  const uint32_t record = ctx.r3.u32;
  const uint32_t spent = ReadU32(base, record) + ReadU32(base, record + 4);
  const uint32_t unspent = ReadU32(base, record + 8);
  if (spent + unspent < kExPointsTotal) {
    const uint32_t value = _byteswap_ulong(kExPointsTotal - spent);
    std::memcpy(const_cast<uint8_t*>(base) + record + 8, &value, sizeof(value));
  }
}

// Sort comparator for blended draws: r3, r4 = records, +176 = depth of the part's
// centre, +216 = model number. Fei-Yen's V-converter window quad (bank 23 part 108)
// lies behind the ring layers of part 0 but its centre can come out nearer, and it
// then paints over them. Sorting it as if further away keeps it behind them.
static float SortDepth(const uint8_t* base, uint32_t record) {
  const uint32_t bits = ReadU32(base, record + 176);
  float depth;
  std::memcpy(&depth, &bits, sizeof(depth));
  if (ReadU32(base, record + 216) == kFeiWindowModel) depth -= kFeiWindowDepthBias;
  return depth;
}

REX_HOOK_RAW(sub_825B1568) {
  ctx.r3.u64 = SortDepth(base, ctx.r3.u32) > SortDepth(base, ctx.r4.u32) ? 1 : 0;
}

// Whether a DLC pack is present: r3 = pack number. Pack 0's options (the Fei-Yen and
// Angelan extra parts) are already in the game and only wait for this answer.
REX_HOOK_RAW(sub_824145A8) {
  const uint32_t pack = ctx.r3.u32;
  __imp__sub_824145A8(ctx, base);
  if (pack == 0) ctx.r3.u64 = 1;
}

// Generated by tools/translation/build_strings.py.
static constexpr uint8_t kEnglishText[] = {
#include "english_text.inc"
};
static_assert(sizeof(kEnglishText) <= kStringTableSize);

// Releases a file loader: r3 = loader. sub_8254A930 copies string_array.bin into its
// buffer and releases the loader before relocating the table, so with --english the
// English table goes into the buffer here and is relocated in place of the original.
REX_HOOK_RAW(sub_82463110) {
  if (ctx.r3.u32 == kStringFile && REXCVAR_GET(english)) {
    const uint32_t table = ReadU32(base, kStringTable);
    if (table) std::memcpy(const_cast<uint8_t*>(base) + table, kEnglishText, sizeof(kEnglishText));
  }
  __imp__sub_82463110(ctx, base);
}

// Called after a draw list record was filled.
static void OnDrawRecord(const uint8_t* base, uint32_t model, uint32_t count) {
  std::lock_guard<std::mutex> lock(g_trace_mutex);
  if (!g_trace) return;
  static uint32_t last_count = 0;
  if (count <= last_count) ++g_frame;  // First draw of a frame.
  last_count = count;
  if (count >= kDrawRecordMax) return;
  if (!(model & kBlendRecord) && !REXCVAR_GET(capture_all) &&
      (model < kFirstRobotModel || model > kLastRobotModel)) {
    return;
  }
  const uint8_t* record = base + kDrawState + kDrawRecordsOffset + count * kDrawRecordSize;
  std::fwrite(&g_frame, 4, 1, g_trace);
  std::fwrite(&model, 4, 1, g_trace);
  std::fwrite(record, kDrawRecordSize, 1, g_trace);
}

// Draws a blend between two models: r3, r4 = model numbers, r5 = amount. When their
// meshes differ it goes through sub_825025F8 to sub_825B3870, whose records hold
// model number -1, so the pair is remembered here and written to the trace with them.
static thread_local uint32_t g_blend_model = 0;

REX_HOOK_RAW(sub_825B3B60) {
  g_blend_model = ctx.r3.u32;
  __imp__sub_825B3B60(ctx, base);
  g_blend_model = 0;
}

REX_HOOK_RAW(sub_825B3870) {
  const uint32_t count = ReadU32(base, kDrawState + kDrawCountOffset);
  const uint32_t model = g_blend_model | kBlendRecord;
  __imp__sub_825B3870(ctx, base);
  OnDrawRecord(base, model, count);
}

// The two routines that fill a draw list record: r4 = model number.
REX_HOOK_RAW(sub_825B3668) {
  const uint32_t model = ctx.r4.u32;
  const uint32_t count = ReadU32(base, kDrawState + kDrawCountOffset);
  __imp__sub_825B3668(ctx, base);
  OnDrawRecord(base, model, count);
}

REX_HOOK_RAW(sub_825B3460) {
  const uint32_t model = ctx.r4.u32;
  const uint32_t count = ReadU32(base, kDrawState + kDrawCountOffset);
  __imp__sub_825B3460(ctx, base);
  OnDrawRecord(base, model, count);
}

namespace force {

void DumpGuestImageIfRequested(rex::Runtime* runtime) {
  const std::string dir = REXCVAR_GET(capture_dir);
  if (!dir.empty() && runtime && runtime->memory()) {
    const uint8_t* base = runtime->memory()->TranslateVirtual(0);
    std::thread(PollCaptureKey, base, dir).detach();
    REXLOG_INFO("capture ready: F9 starts and stops, folder {}", dir);
  }

  const std::string path = REXCVAR_GET(dump_image);
  if (path.empty() || !runtime || !runtime->memory()) return;

  constexpr uint32_t kImageBase = 0x82000000;
  constexpr uint32_t kImageSize = 0x02200000;
  const uint8_t* src = runtime->memory()->TranslateVirtual(kImageBase);

  FILE* f = std::fopen(path.c_str(), "wb");
  if (!f) {
    REXLOG_ERROR("dump_image: cannot open {}", path);
    return;
  }
  std::fwrite(src, 1, kImageSize, f);
  std::fclose(f);
  REXLOG_INFO("dump_image: wrote {:#x} bytes to {}", kImageSize, path);
}

}  // namespace force

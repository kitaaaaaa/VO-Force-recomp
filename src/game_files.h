// Locating the player's game files.

#pragma once

#include <filesystem>
#include <string>

namespace force {

// Finds the game for a player who just double-clicked the exe: extracted files
// (default.xex and media/) or a disc image (.iso, unpacked on first launch) in
// assets/ next to the exe, or in the project's assets/ for a developer build.
// Returns the folder holding the files; on failure, an empty path and *error.
std::filesystem::path LocateGameFiles(const std::filesystem::path& exe_dir, std::string* error);

// Installs any DLC packages the player put in the game folder into the content
// folder under `user_data_root`, where the game looks for them.
void InstallDlc(const std::filesystem::path& game_dir, const std::filesystem::path& user_data_root);

void ShowError(const std::string& message);

}  // namespace force

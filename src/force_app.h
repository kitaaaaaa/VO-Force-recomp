// force - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <rex/filesystem.h>
#include <rex/rex_app.h>

#include <cstdlib>
#include <filesystem>
#include <string>

#include "debug_tools.h"
#include "game_files.h"

class ForceApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<ForceApp>(new ForceApp(ctx, "force",
        PPCImageConfig));
  }

  // Override virtual hooks for customization:
  // void OnPostInitLogging() override {}
  void OnPreSetup(rex::RuntimeConfig& config) override {
    if (config.gpu_plugin.empty()) config.gpu_plugin = "xenos";
  }
  // void OnLoadXexImage(std::string& xex_image) override {}
  void OnPostLoadXexImage() override {
    force::DumpGuestImageIfRequested(runtime());
  }
  // void OnPostSetup() override {}
  // void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {}
  // std::unique_ptr<rex::ui::ImGuiDialog> CreateAchievementsOverlay() override;
  // std::unique_ptr<rex::ui::AchievementNotificationDialog>
  // CreateAchievementNotificationDialog() override;
  // void OnShutdown() override {}
  // No --game_data_root: find the game files (extracted, or a disc image, which
  // is unpacked on first launch) so double-clicking works.
  void OnConfigurePaths(rex::PathConfig& paths) override {
    if (paths.game_data_root.empty()) {
      std::string error;
      paths.game_data_root = force::LocateGameFiles(rex::filesystem::GetExecutableFolder(), &error);
      if (paths.game_data_root.empty()) {
        force::ShowError(error);
        std::exit(1);
      }
    }
    force::InstallDlc(paths.game_data_root, paths.user_data_root);
  }
};

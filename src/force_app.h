// force - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <rex/filesystem.h>
#include <rex/rex_app.h>

#include <filesystem>

#include "debug_tools.h"

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
  // No --game_data_root: use assets next to the exe, else the project's assets
  // (the dev build lives in out/build/<preset>).
  void OnConfigurePaths(rex::PathConfig& paths) override {
    if (!paths.game_data_root.empty()) return;
    const auto exe_dir = rex::filesystem::GetExecutableFolder();
    paths.game_data_root = std::filesystem::exists(exe_dir / "assets")
                               ? exe_dir / "assets"
                               : exe_dir / "../../../assets";
  }
};

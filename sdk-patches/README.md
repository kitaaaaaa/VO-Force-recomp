# SDK patches

Changes applied to [ReXGlue SDK v0.10.0](https://github.com/rexglue/rexglue-sdk/releases/tag/v0.10.0)
before it is built from source. `tools/setup.ps1` does this for you.

| Patch | What |
|---|---|
| `0001-force-fixes.patch` | Three fixes this game needs. 1D textures wider than the host limit, which the game uses as colour tables in vertex shaders, are read from shared memory (without this, effects and HUD elements draw black). `ListContent` compares against `X_ERROR_SUCCESS`, because `XSUCCEEDED` let a failed read return uninitialized data and loading a save crashed. `XamContentCreate` refuses a file name that is not text instead of throwing on the dispatch thread, and logs each call. |

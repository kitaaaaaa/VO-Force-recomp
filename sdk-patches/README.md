# SDK patches

Changes applied to [ReXGlue SDK v0.10.0](https://github.com/rexglue/rexglue-sdk/releases/tag/v0.10.0)
before it is built from source. `tools/setup.ps1` does this for you.

| Patch | What |
|---|---|
| `0001-force-fixes.patch` | Four fixes this game needs. 1D textures wider than the host limit, which the game uses as colour tables in vertex shaders, are read from shared memory (without this, effects and HUD elements draw black). `ListContent` compares against `X_ERROR_SUCCESS`, because `XSUCCEEDED` let a failed read return uninitialized data and loading a save crashed. `XamContentCreate` refuses a file name that is not text instead of throwing on the dispatch thread, and logs each call. Blending with constant alpha puts the alpha in the constant's RGB, because Direct3D 12 colour blending reads the constant's RGB (without this, the melee finish blur turns the kill cam black). |
| `0002-native-render-targets.patch` | Native render targets (`native_render_targets`, `native_resolution`, `native_msaa`), ported from Oratan-recomp: resolves become host copies into native textures instead of EDRAM emulation, so the game renders at exactly 1920×1080, 2560×1440 or 3840×2160. For this game: the 1280×720 scene, its 640×360 prepass and the frame scale to the output height, shadow maps and bloom stay 1×; the pixel position shaders read is in guest pixels for every draw (the robots' reflections take their normals from the screen at it); depth resolves sampled as `k_24_8` read as float depth; cube maps and mipmapped textures rendered face by face and level by level are gathered from their native textures; native texture lookups are cached so they cost little per draw. |

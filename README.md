# VO-Force-recomp

**Mind Shift Battle System · M.S.B.S. Ver.7.7 · Windows-host field deployment**

Developed and distributed jointly by **Sentinel Electronics** and **General Aerodyne** with support from **Open Virturon Gate** and the **Limited War Commission**.
Authorized for Virtuaroid pilots of all affiliated Plants.

> [!NOTE]
> Alpha deployment. Frames can be linked and sorties completed, but not every protocol
> has been verified. Please file
> [incident reports](https://github.com/kitaaaaaa/VO-Force-recomp/issues).

---

> ### ⚠ OPERATOR SAFETY NOTICE: READ BEFORE INGRESS
>
> **M.S.B.S. Ver.7.7 IS NOT A SIMULATORY SYSTEM.**
>
> This build establishes a live mind-shift link between the pilot and a Virtuaroid frame
> via Cyber Imaginary Space. Combat outcomes are not rendered, replayed or approximated.
> They occur.
>
> **Ingress is restricted to certified Virturon Positive (V-Positive) personnel.** Unauthorized
> ingress has resulted in neural feedback trauma, permanent loss of consciousness, and
> documented cases of **Shadow Virtuaroid manifestation**.
>
> Sentinel Electronics and General Aerodyne accept no liability for pilots lost to Shadow
> phenomena. Do not engage Shadows. Report all sightings to your Plant's M.S.B.S.
> maintenance desk.

---

## Ingress procedure

Your Plant must hold its own licensed combat data: a disc image (`.iso`) of
**Dennou Senki Virtual-On Force** (Xbox 360, Japan, Title ID `5345084C`).
**Combat data is not distributed with this build.** Unlicensed data cannot establish a link.

1. **Acquire the deployment package.** Download the latest `VO-Force-recomp-…-win64.zip` from
   [Releases](https://github.com/kitaaaaaa/VO-Force-recomp/releases) and unpack it to any secure
   terminal.
2. **Place your disc image in the `assets` folder** inside it.
3. **Double-click `force.exe`.**

The first ingress unpacks your combat data into `assets`, which takes a moment before the window
appears. The disc image is not needed after that. Every later ingress: double-click the same file.

### Terminal requirements

| | |
|---|---|
| Operating system | Windows 10 or 11, 64-bit |
| Display adapter | DirectX 12 |
| Storage | About 1 GB once your combat data is unpacked, plus the disc image until you remove it |

### Supplementary mission data

Mission pack DLC is not distributed with this build either. If your Plant holds the package files,
place them in `assets` under their original file names. They are installed at the next ingress.

## Field modifications

The system deploys in its certified state: Japanese-language briefings and your service record's
own progress. Modifications are issued with a launch argument, for example
`force.exe --english=true` from a shortcut or a command prompt.

| Modification | Argument | Effect |
|---|---|---|
| English-language briefing | `--english=true` | Menus and mission briefings are issued in English, in place of the original Japanese text. |
| Full frame authorization | `--unlock_all_vrs=true` | Every Virtuaroid is issued to the pilot. Reaches your service record the next time it saves. |
| EX OPTIONS allocation | `--unlock_ex_options=true` | Enough points to raise LIFE and ATTACK to their limit of 10. |

Standing provisions, always active:

| Provision | Effect |
|---|---|
| Extra parts clearance | The Fei-Yen and Angelan extra parts option, already held in the combat data, is released. |
| V-converter window repair | The border of Fei-Yen's V-converter window stays visible from every angle. |

## Status board

| Protocol | Status |
|---|---|
| Disc image to running system through `force.exe` | Verified |
| Sorties with AI opponents, training, menus, service record | Working |
| English text in menus and briefings, with `--english=true` | Working |
| English text drawn into images | Not yet translated |
| Kill-cam replay | The feed can stay black for a moment before the kill |
| Campaign, start to finish | Not verified |
| Xbox LIVE and System Link | Not verified |

Pending field work: unsealing the VR viewer camera, and tuning turning, lock-on and dashing to
M.S.B.S. Ver.5.66 standard. See [notes/PATCHES.md](notes/PATCHES.md).

Attach the file from the `logs` folder next to `force.exe` to all incident reports.

## Maintenance desk: building by hand

For technicians. Pilots do not need this section.

`tools\setup.ps1 <your image>.iso`, run in PowerShell, downloads the tools into `.tools` and does all
of this. By hand, you need Visual Studio 2022 with **Desktop development with C++**, Clang 20 or newer,
CMake 3.25 or newer, and Ninja. Then build [ReXGlue SDK v0.10.0](https://github.com/rexglue/rexglue-sdk) with the
patch in [sdk-patches/](sdk-patches/README.md), put the game in [assets/](assets/README.md), and:

```
rexglue codegen force_manifest.toml
cmake --preset win-amd64-release -DCMAKE_PREFIX_PATH=<SDK prefix>
cmake --build out\build\win-amd64-release
```

`python tools/package_release.py <version>` then assembles the release zip in `dist/`.

| Path | What |
|---|---|
| `tools/setup.ps1`, `tools/package_release.py` | Building from source, and assembling a release |
| `force_manifest.toml`, `force_config.toml` | Recompiler settings and hand-fixed function boundaries |
| `src/` | The PC side: startup and patches |
| `sdk-patches/` | Fixes this game needs in the SDK |
| `tools/translation/` | English text and the tool that builds it into `src/english_text.inc` |
| `generated/default/` | Recompiled code, generated locally and never committed |

---



**Legal.** Unofficial fan project, not affiliated with or endorsed by SEGA or Microsoft. Virtual-On
is a trademark of SEGA. Characters copyright of Hajime Katoki. No game files or DLC are
provided here. You need your own copy of the game.

**Credits.**
- [ReXGlue](https://github.com/rexglue/rexglue-sdk), the recompilation SDK
- [Xenia](https://github.com/xenia-project/xenia), whose emulator code the runtime builds on
- [XenonRecomp](https://github.com/hedge-dev/XenonRecomp), which pioneered this approach
- [Oratan-recomp](https://github.com/kitaaaaaa/Oratan-recomp), this project's sibling for M.S.B.S. Ver.5.66

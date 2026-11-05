# VO-Force-recomp

**Mind Shift Battle System · M.S.B.S. Ver.7.7 · Windows-host field deployment**

Developed and distributed jointly by **Sentinel Electronics** and **General Aerodyne**.
Authorized for Virtuaroid pilots of all affiliated Plants.

<sub>Out of character: this is an unofficial fan recompilation of Dennou Senki Virtual-On Force (Xbox 360) for Windows. No game files are included, and you need your own copy. The real details are at the bottom of this page.</sub>

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

1. **Acquire the deployment package.** Use **Code → Download ZIP** at the top of this page and
   unpack it to any secure terminal.
2. **Place your disc image in the unpacked folder.** If you skip this, you will be asked for it.
3. **Double-click `Play Virtual-On Force.bat`.**

The first ingress assembles the system at your terminal from your own combat data. Download
volume, storage and duration are listed below. The console window can be left unattended, unless
your terminal has no Microsoft C++ build tools. In that case it asks once whether to install them.

Every later ingress: double-click the same file. The link opens immediately.

### Terminal requirements

| | |
|---|---|
| Operating system | Windows 10 or 11, 64-bit |
| Display adapter | DirectX 12 |
| Storage | About 8 GB, on the drive holding this folder |
| Download volume | About 1.1 GB, plus about 7 GB if the C++ build tools must be installed |
| First assembly | About 20 minutes on the reference terminal, including downloads. Slower terminals and connections take longer. |
| Build tools | Downloaded into `.tools` in this folder. Only the Microsoft C++ build tools are installed on the system, and only with your approval. |

### If assembly fails

The full record is in `logs\setup.log`. Attach it to your incident report. Running the same file
again resumes from where assembly stopped.

## Field modifications

Active by default in this deployment. Each can be withdrawn with a launch argument, for example
`"Play Virtual-On Force.bat" --unlock_all_vrs=false` once assembly is complete.

| Modification | Argument | Effect |
|---|---|---|
| Full frame authorization | `--unlock_all_vrs` | Every Virtuaroid is issued to the pilot. Reaches your service record the next time it saves. |
| Extra parts clearance | `--unlock_extra_parts` | The Fei-Yen and Angelan extra parts option, already held in the combat data, is released. |
| EX OPTIONS allocation | `--unlock_ex_options` | Enough points to raise LIFE and ATTACK to their limit of 10. |
| V-converter window repair | `--fix_fei_window` | The border of Fei-Yen's V-converter window stays visible from every angle. |
| English-language briefing | none | Menus and mission briefings are issued in English, in place of the original Japanese text. |

## Status board

| Protocol | Status |
|---|---|
| Disc to running system through the one-click procedure | Verified |
| Sorties with AI opponents, training, menus, service record | Working |
| English text in menus and briefings | Working |
| English text drawn into images | Not yet translated |
| Kill-cam replay | The feed can stay black for a moment before the kill |
| Campaign, start to finish | Not verified |
| Xbox LIVE and System Link | Not verified |

Pending field work: unsealing the VR viewer camera, and tuning turning, lock-on and dashing to
M.S.B.S. Ver.5.66 standard. See [notes/PATCHES.md](notes/PATCHES.md).

Attach `logs\force.log` to all incident reports.

## Maintenance desk: building by hand

For technicians. Pilots do not need this section.

You need Visual Studio 2022 with **Desktop development with C++**, Clang 20 or newer, CMake 3.25 or
newer, and Ninja. Then build [ReXGlue SDK v0.10.0](https://github.com/rexglue/rexglue-sdk) with the
patch in [sdk-patches/](sdk-patches/README.md), put the game in [assets/](assets/README.md), and:

```
rexglue codegen force_manifest.toml
cmake --preset win-amd64-release -DCMAKE_PREFIX_PATH=<SDK prefix>
cmake --build out\build\win-amd64-release
```

| Path | What |
|---|---|
| `Play Virtual-On Force.bat`, `tools/setup.ps1` | The one-click procedure |
| `force_manifest.toml`, `force_config.toml` | Recompiler settings and hand-fixed function boundaries |
| `src/` | The PC side: startup and patches |
| `sdk-patches/` | Fixes this game needs in the SDK |
| `tools/translation/` | English text and the tool that builds it into the game's string table |
| `generated/default/` | Recompiled code, generated locally and never committed |

---

## Out of character

**Legal.** Unofficial fan project, not affiliated with or endorsed by SEGA or Microsoft. Virtual-On
is a trademark of SEGA. Sentinel Electronics, General Aerodyne, the safety notice and everything
else in-universe on this page are fiction. No copyrighted game data or recompiled game code is
provided here; both stay on your own PC. You need your own copy of the game.

**Credits.**
- [ReXGlue](https://github.com/rexglue/rexglue-sdk), the recompilation SDK
- [Xenia](https://github.com/xenia-project/xenia), whose emulator code the runtime builds on
- [XenonRecomp](https://github.com/hedge-dev/XenonRecomp), which pioneered this approach
- [Oratan-recomp](https://github.com/kitaaaaaa/Oratan-recomp), this project's sibling for M.S.B.S. Ver.5.66

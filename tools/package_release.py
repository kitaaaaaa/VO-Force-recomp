"""Assemble the player-facing release zip from a release build.

  python tools/package_release.py 0.1.0

Produces dist/VO-Force-recomp-<version>-win64.zip containing:
  VO-Force-recomp/
    force.exe, rexruntime.dll, rexgpu-xenos.dll
    Visual C++ runtime DLLs (app-local, so no redistributable install is needed)
    assets/HOW TO ADD THE GAME.txt   (players put their game files here)
    README.txt

No game files are included; players supply their own.
"""

import os
import shutil
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "out" / "build" / "win-amd64-release"
BINARIES = ["force.exe", "rexruntime.dll", "rexgpu-xenos.dll"]
VC_RUNTIME = ["msvcp140.dll", "msvcp140_atomic_wait.dll", "vcruntime140.dll", "vcruntime140_1.dll"]

ASSETS_TXT = r"""Put your Dennou Senki Virtual-On Force game in THIS folder, then start force.exe.

Easiest: copy your disc image (.iso) straight in here. It is unpacked
automatically the first time you start the game, and is not needed after that.

Already-extracted files (default.xex and the media folder) work too.

DLC: if you have the game's DLC package files, copy them in here as well, with
their original file names. They are installed the next time you start the game.
"""

README_TXT = """\
Dennou Senki Virtual-On Force (M.S.B.S. Ver.7.7) - PC recompilation  v{version}
=====================================================================

1. Copy your own copy of the game into the "assets" folder
   (see assets\\HOW TO ADD THE GAME.txt).
2. Double-click force.exe.

The game starts in Japanese, with the original unlock progress. Options, given
when starting force.exe from a shortcut or a command prompt:

  --english=true            English text in menus and briefings
  --unlock_all_vrs=true     Own every VR. Reaches your save the next time it saves
  --unlock_ex_options=true  Enough EX OPTIONS points to raise LIFE and ATTACK to 10

Alpha. Bug reports and logs are welcome at
https://github.com/kitaaaaaa/VO-Force-recomp/issues
A log is written to the "logs" folder next to force.exe.

This is an unofficial fan project, not affiliated with SEGA or Microsoft.
No game files are included.
"""


def find_vc_runtime(name: str) -> Path:
    system32 = Path(os.environ.get("SystemRoot", r"C:\Windows")) / "System32"
    path = system32 / name
    if not path.exists():
        sys.exit(f"missing {path}; install the latest VC++ redistributable")
    return path


def main() -> int:
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    version = sys.argv[1]

    stage = ROOT / "dist" / "VO-Force-recomp"
    if stage.exists():
        shutil.rmtree(stage)
    (stage / "assets").mkdir(parents=True)

    for name in BINARIES:
        src = BUILD / name
        if not src.exists():
            sys.exit(f"missing {src}; build the release preset first")
        shutil.copy2(src, stage / name)
    for name in VC_RUNTIME:
        shutil.copy2(find_vc_runtime(name), stage / name)

    (stage / "assets" / "HOW TO ADD THE GAME.txt").write_text(ASSETS_TXT, newline="\r\n")
    (stage / "README.txt").write_text(README_TXT.format(version=version), newline="\r\n")

    out = ROOT / "dist" / f"VO-Force-recomp-{version}-win64.zip"
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for f in sorted(stage.rglob("*")):
            z.write(f, f.relative_to(stage.parent))
    print(f"{out} ({out.stat().st_size / 1e6:.1f} MB)")
    return 0


if __name__ == "__main__":
    sys.exit(main())

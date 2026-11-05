"""List or extract the game partition of an Xbox 360 disc image (XDVDFS).

Usage:
  python xiso_extract.py <image.iso>              list files
  python xiso_extract.py <image.iso> <out dir>    extract files
"""

import struct
import sys
from pathlib import Path

SECTOR = 2048
MAGIC = b"MICROSOFT*XBOX*MEDIA"
# Game partition start for XGD2, XGD1, XGD3, and a plain XISO.
PARTITIONS = [0xFD90000, 0x18300000, 0x2080000, 0]
ATTR_DIRECTORY = 0x10
CHUNK = 16 * 1024 * 1024


def find_partition(f) -> int:
    for base in PARTITIONS:
        f.seek(base + 32 * SECTOR)
        if f.read(len(MAGIC)) == MAGIC:
            return base
    raise SystemExit("no XDVDFS volume descriptor found")


def walk(f, base: int, sector: int, size: int, prefix: str = ""):
    """Yield (path, sector, size, is_dir). Entries in a directory form a tree."""
    f.seek(base + sector * SECTOR)
    data = f.read(size)
    stack = [0]
    seen = set()
    while stack:
        off = stack.pop()
        if off in seen or off + 14 > len(data):
            continue
        seen.add(off)
        left, right, sec, length, attr, namelen = struct.unpack_from("<HHIIBB", data, off)
        if left == 0xFFFF and right == 0xFFFF:
            continue
        name = data[off + 14:off + 14 + namelen].decode("latin-1")
        if name in ("", ".", "..") or "/" in name or "\\" in name or ":" in name:
            raise SystemExit(f"unsafe file name in image: {name!r}")
        path = prefix + name
        is_dir = bool(attr & ATTR_DIRECTORY)
        yield path, sec, length, is_dir
        if is_dir and length:
            yield from walk(f, base, sec, length, path + "/")
        if left:
            stack.append(left * 4)
        if right:
            stack.append(right * 4)


def main() -> int:
    image = sys.argv[1]
    out = Path(sys.argv[2]) if len(sys.argv) > 2 else None
    with open(image, "rb") as f:
        base = find_partition(f)
        f.seek(base + 32 * SECTOR + len(MAGIC))
        root_sector, root_size = struct.unpack("<II", f.read(8))
        print(f"partition at {base:#x}, root sector {root_sector}, size {root_size}")
        files = dirs = total = 0
        for path, sec, length, is_dir in sorted(walk(f, base, root_sector, root_size)):
            if is_dir:
                dirs += 1
                if out:
                    (out / path).mkdir(parents=True, exist_ok=True)
                continue
            files += 1
            total += length
            print(f"{length:>12} {path}")
            if out:
                dest = out / path
                dest.parent.mkdir(parents=True, exist_ok=True)
                f.seek(base + sec * SECTOR)
                left = length
                with open(dest, "wb") as w:
                    while left:
                        chunk = f.read(min(left, CHUNK))
                        if not chunk:
                            raise SystemExit(f"image ended early in {path}")
                        w.write(chunk)
                        left -= len(chunk)
        print(f"{files} files, {dirs} directories, {total} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())

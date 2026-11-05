"""Unpack a .tar.xz, dropping its top folder. Windows 10's tar.exe cannot read xz.

Usage: python untar.py <archive> <out dir>
"""

import sys
import tarfile


def strip_top(member, path):
    member = tarfile.data_filter(member, path)
    _, _, rest = member.name.partition("/")
    return member.replace(name=rest, deep=False) if rest else None


with tarfile.open(sys.argv[1]) as archive:
    archive.extractall(sys.argv[2], filter=strip_top)

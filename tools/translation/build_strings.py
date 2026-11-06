"""Build the English media/string_array.bin from the original and strings_en.py.

Usage: python build_strings.py <original string_array.bin> <output> [report.txt]

The output's bytes also go to src/english_text.inc, which is compiled into the game
and replaces the table it loads when run with --english.
"""

import re
import struct
import sys
import textwrap
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from strings_en import EN

JP = re.compile(r"[぀-ヿ㐀-鿿]")

NAMES = [
    ("テン・エイティ", "Ten-Eighty"), ("テムジン", "Temjin"), ("フェイ・イェン", "Fei-Yen"), ("ボックス", "Vox"),
    ("エンジェラン", "Angelan"), ("バル・", "Bal "), ("景清", "Kagekiyo"), ("アファームド", "Apharmd"),
    ("マイザー・", "Myzr "), ("スペシネフ１３", "Specineff 13"), ("ライデン", "Raiden"), ("ガラヤカ", "Guarayakha"),
    ("ヤガランデ", "Jaguarandi"), ("ランダム", "Random"), ("タイプ", "Type "), ("ハッター", "Hatter"),
    ("ディ・メオラ", "d Meora"), ("バ・チスタ", "b Cista"), ("バ・ティグラ", "v Tigla"), ("メ・リーノ", "m Rino"),
    ("ミ・ランダ", "m Landa"), ("セ・リムゾ", "c Rimzo"), ("デルタ", "Delta"), ("イータ", "Eta"), ("ガンマ", "Gamma"),
    ("ダン", "Dan"), ("エイジ", "Eiji"), ("ジョー", "Joe"), ("ジェーン", "Jane"), ("ダニー", "Danny"), ("ボブ", "Bob"),
    ("ユウタ", "Yuta"), ("マリコ", "Mariko"), ("テツオ", "Tetsuo"), ("慈愛", "Mercy"), ("治癒", "Healing"), ("慰撫", "Solace"),
    ("風", "Wind"), ("林", "Forest"), ("火", "Fire"), ("山", "Mountain"), ("凬", "Storm"), ("罪", "Sin"), ("戦", "War"), ("終", "End"),
]
SERIES = {"テムジン": "Temjin", "アファームドＴ": "Apharmd T", "アファームドＪ": "Apharmd J", "ライデン": "Raiden", "エンジェラン": "Angelan",
          "マイザー": "Myzr", "景清": "Kagekiyo", "ボックス": "Vox", "フェイ・イェン": "Fei-Yen", "バル": "Bal", "スペシネフ１３": "Specineff 13", "ガラヤカ": "Guarayakha"}
ORDINAL = {"第一": "1", "第二": "2", "第三": "3", "第四": "4", "第五": "5", "第六": "6"}


def to_ascii(s):
    out = []
    for c in s:
        o = ord(c)
        if 0xFF01 <= o <= 0xFF5E:
            out.append(chr(o - 0xFEE0))
        elif c == "　":
            out.append(" ")
        elif c == "．":
            out.append(".")
        elif c == "／":
            out.append("/")
        elif c == "ー":
            out.append("-")
        else:
            out.append(c)
    return "".join(out)


def vr_name(s):
    for jp, en in NAMES:
        s = s.replace(jp, en)
    s = s.replace("「", ' "').replace("」", '"')
    s = to_ascii(s).replace('"Type', '" Type').replace('"+', '" +').replace("\n ", "\n")
    return s.replace(" \n", "\n").strip()


def pattern(s):
    m = re.match(r"特別演習　対(.+?)系統(第.)?$", s)
    if m:
        part = f" {ORDINAL[m.group(2)]}" if m.group(2) else ""
        return f"Drill: vs {SERIES[m.group(1)]}{part}"
    m = re.match(r"特別演習　(第.)限定状況想定$", s)
    if m:
        return f"Drill: Limited {ORDINAL[m.group(1)]}"
    m = re.match(r"これより、対(.+?)系統機体を\n想定した実地訓練を行う。\n敵リーダーを撃破せよ！$", s)
    if m:
        return f"Live training against {SERIES[m.group(1)]} series VRs. Destroy the enemy leader!"
    return None


def display_len(s):
    """Width in half-width characters."""
    return sum(1 if ord(c) < 0x80 else 2 for c in s)


def wrap(en, src):
    """Break English text to fit the box the Japanese text used."""
    if "\n" in en or "\n" not in src:
        return en
    width = max(display_len(line) for line in src.split("\n"))
    return "\n".join(textwrap.wrap(en, width, break_long_words=False))


def check_relocation(data, strings):
    """Run the game's load loop (sub_8254A930) and confirm every string still reads back."""
    base = 0x10000000
    table = list(struct.unpack_from(f">{len(data) // 4}I", data, 0))
    i = 0
    while True:
        table[i] = (table[i] + base) & 0xFFFFFFFF
        i += 1
        if table[i] == 0:
            break
    if i != len(strings) - 1:
        raise SystemExit(f"relocation stops at entry {i}, expected {len(strings) - 1}")
    for k in range(len(strings) - 1):
        o = table[k] - base
        if data[o:data.find(b"\0", o)].decode("utf-8") != strings[k]:
            raise SystemExit(f"string {k} does not read back")


def main():
    d = Path(sys.argv[1]).read_bytes()
    count = struct.unpack(">I", d[:4])[0] // 4
    src = []
    for i in range(count):
        o = struct.unpack_from(">I", d, i * 4)[0]
        src.append(d[o:d.find(b"\0", o)].decode("utf-8"))
    by_source = {}
    for i, s in enumerate(src):
        if i in EN:
            by_source.setdefault(s, EN[i])
    out, report = [], []
    for i, s in enumerate(src):
        if i in EN:
            en = EN[i]
        elif s in by_source:
            en = by_source[s]
        elif 236 <= i <= 423:
            en = vr_name(s)
        else:
            en = pattern(s) or to_ascii(s)
        if JP.search(en):
            report.append(f"UNTRANSLATED {i}: {s!r}")
        en = wrap(en, s)
        src_lines, en_lines = s.count("\n") + 1, en.count("\n") + 1
        if en_lines > src_lines:
            report.append(f"MORE LINES {i}: {src_lines} -> {en_lines}: {en!r}")
        if "\n" not in s and display_len(en) > max(display_len(s), 8) and len(s) < 30 and display_len(s) < 40:
            report.append(f"WIDER {i}: {display_len(s)} -> {display_len(en)}: {en!r}")
        if re.findall(r"%[-0-9]*[a-z]", s) != re.findall(r"%[-0-9]*[a-z]", en):
            report.append(f"FORMAT {i}: {s!r} -> {en!r}")
        out.append(en)
    head = 4 * count
    blob = bytearray()
    offsets = []
    for en in out:
        offsets.append(head + len(blob))
        blob += en.encode("utf-8") + b"\0"
    # The game relocates the table by walking it until an offset of 0, so the
    # last entry (an empty string) must be 0, as in the original file.
    offsets[-1] = 0
    data = struct.pack(f">{count}I", *offsets) + bytes(blob)
    check_relocation(data, out)
    Path(sys.argv[2]).write_bytes(data)
    rows = [data[i:i + 32] for i in range(0, len(data), 32)]
    (Path(__file__).parents[2] / "src" / "english_text.inc").write_text(
        "".join("".join(f"{b}," for b in row) + "\n" for row in rows))
    print(f"{count} strings, {len(data)} bytes (original {len(d)}), {len(report)} notes; relocation check passed")
    if len(sys.argv) > 3:
        Path(sys.argv[3]).write_text("\n".join(report), encoding="utf-8")
    Path(sys.argv[2]).with_suffix(".en.txt").write_text(
        "\n".join(f"{i}\t{t!r}" for i, t in enumerate(out)), encoding="utf-8")


if __name__ == "__main__":
    main()

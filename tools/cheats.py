"""Code cheats of the player's mods, built into the game as code patch sets (runtime/cheats.c).

The lifted game cannot be byte-patched while it runs, so a cheat that writes main's instructions (Atmosphere opcode 0
into the main region, below the end of its code) becomes a patch set `cheat_<hash>` of the generator (recomp/patches.py)
that the Cheats tab switches at once (rt_patch_switch). This tool reads the cheat file of every enabled mod of each game
folder, writes the patch lines of the cheats it can turn into sets, and writes build-cache/cheats.status into each game
folder: one line per code cheat, tab separated,

    <name hash> <content hash> <kind> <buttons> <note>

name hash: FNV-1a 32 of "<mod folder>/<cheat name>" ("#<n>" after the name for its n-th repeat in the file), content hash:
FNV-1a 32 of its words as "%08X" joined by spaces, both upper-case hex, as runtime/cheats.c computes them; kind: code
(a set the build has), restore (it only writes the game's own words back) or unsupported (note says why). Cheats
without code writes get no line: the runtime runs data cheats in its VM by itself.

A write of one instruction becomes a site with that word. A branch (bl, or b) into words the same cheat writes after the
end of the code or over zero padding (a code cave) is inlined: the cave's words up to its `ret` (after bl) or its `b`
back to the instruction after the site (after b) replace the site, lifted at the site's address. A cave word that
depends on its own address (adr, adrp, a literal load, a branch) cannot move and makes the cheat unsupported. Each
alternative is checked as the generator checks it (recomp/gen.py replacements, patched_effect): only the last word may
branch, and it writes the flags the original writes.

    python3 tools/cheats.py --exefs build/v130/exefs --dist dist/steamdeck [--dist dist/windows] --out FILE
"""
import argparse
import configparser
import os
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "recomp"))

import flags  # noqa: E402
from layout import layout  # noqa: E402
from lifter import Lifter, Unsupported  # noqa: E402
from nso import Nso  # noqa: E402

RET, NOP = 0xD65F03C0, 0xD503201F
CAVE_MAX = 64   # words of one cave


def fnv(data: bytes) -> int:
    h = 0x811C9DC5
    for b in data:
        h = ((h ^ b) * 0x01000193) & 0xFFFFFFFF
    return h


def parse(text: str) -> list[dict]:
    """The cheats of a file as runtime/cheats.c parse reads them: name, master, words, dup (1, 2 ... per repeat)."""
    out = []
    for raw in text.split("\n"):
        line = raw.strip(" \t").rstrip("\r \t")
        if len(line) >= 2 and line[0] in "[{" and line[-1] in "]}":
            name = line[1:-1].strip(" \t")
            dup = 1 + sum(c["name"] == name for c in out)
            out.append({"name": name, "master": line[0] == "{", "words": [], "dup": dup})
        elif out:
            for tok in line.replace("\t", " ").split(" "):
                if len(tok) == 8:
                    try:
                        out[-1]["words"].append(int(tok, 16))
                    except ValueError:
                        pass
    return out


def op_len(w: list[int], i: int) -> int:
    """Words of the opcode at w[i] (runtime/cheats.c op_len), 0 when cut short."""
    f = w[i]
    t, width = f >> 28, (f >> 24) & 0xF
    if t in (0, 1):
        n = 4 if width == 8 else 3
    elif t in (2, 8):
        n = 1
    elif t == 3:
        n = 2 if width == 0 else 1
    elif t in (4, 6):
        n = 3
    elif t in (5, 7):
        n = 2
    elif t == 9:
        n = 1 + ((2 if width == 8 else 1) if (f >> 8) & 0xF else 0)
    elif t == 0xA:
        n = 2 if (f >> 8) & 0xF in (2, 4, 5) else 1
    else:
        n = 1
    return n if i + n <= len(w) else 0


def mods_disabled(dist: Path) -> set[str]:
    v = os.environ.get("ACR_MODS_DISABLED")
    if v is None:
        cp = configparser.ConfigParser(interpolation=None, strict=False)
        try:
            cp.read(dist / "settings.ini", encoding="utf-8")
        except (configparser.Error, OSError):
            return set()
        v = next((cp[s][k] for s in cp.sections() for k in cp[s] if k.lower() in ("mods_disabled", "acr_mods_disabled")),
                 "")
    return {n for n in v.split("|") if n}


def cheat_files(dist: Path, bid: str):
    """(mod folder name, cheat file) of each enabled mod of a game folder that has the file for this build."""
    mods = dist / "mods"
    if not mods.is_dir():
        return
    off = mods_disabled(dist)
    for m in sorted(mods.iterdir()):
        cheats = m / "cheats"
        if not m.is_dir() or m.name in off or "|" in m.name or not cheats.is_dir():
            continue
        for f in cheats.iterdir():
            if f.is_file() and f.name.lower() == f"{bid}.txt".lower():
                yield m.name, f


class Main:
    def __init__(self, exefs: Path):
        mods = layout(exefs)
        self.base = next(b for n, b, _ in mods if n == "main")
        self.nso = Nso(str(exefs / "main"))
        self.text_end = self.nso.text[0] + self.nso.text[1]
        self.code_end = (self.text_end + 0xFFF) & ~0xFFF    # runtime/cheats.c g_code_end
        self.lifter = Lifter(self.base, "c")

    def word(self, off: int) -> int:
        return self.nso.u32(off)

    def lift(self, w: int, off: int):
        return self.lifter.lift(w, self.base + off)


def branch(w: int, off: int):
    """('bl' | 'b', target offset) of an immediate b / bl, else None."""
    if w & 0x7C000000 != 0x14000000:
        return None
    imm = w & 0x3FFFFFF
    imm -= (imm & 0x2000000) << 1
    return ("bl" if w >> 31 else "b"), off + 4 * imm


def movable(w: int) -> bool:
    """A cave word that means the same lifted at another address: no branch, adr / adrp or literal load."""
    if w & 0x1F000000 == 0x10000000:          # adr, adrp
        return False
    if w & 0x3B000000 == 0x18000000:          # ldr / ldrsw / prfm (literal), also SIMD
        return False
    if w & 0x1C000000 == 0x14000000:          # b, bl, b.cond, cbz/cbnz, tbz/tbnz, br/blr/ret ...
        return False
    return True


def code_writes(words: list[int], main: Main):
    """{main offset: word} of the cheat's writes into main's code, and whether it has any other opcode."""
    writes, other, i = {}, False, 0
    while i < len(words):
        n = op_len(words, i)
        if not n:
            raise ValueError(f"opcode {words[i]:08X} is cut short")
        f = words[i]
        if f >> 28 == 0 and (f >> 20) & 0xF == 0:
            off = (f & 0xFF) << 32 | words[i + 1]
            width = (f >> 24) & 0xF
            if off < main.code_end:
                if width == 4:
                    writes[off] = words[i + 2]
                elif width == 8:
                    writes[off], writes[off + 4] = words[i + 3], words[i + 2]
                else:
                    raise ValueError(f"a {width}-byte write into the game's code")
                if off & 3:
                    raise ValueError(f"an unaligned write into the game's code at main+{off:#x}")
                i += n
                continue
        other = True
        i += n
    return writes, other


def flag_effect(ins):
    if ins.kind == "op":
        return flags.effect(ins.code or "")
    if ins.kind == "bcond":
        return flags.effect(ins.cond)
    return (False, None)


def check_site(main: Main, off: int, repl: list[int]) -> None:
    """recomp/gen.py's checks of one alternative against the original word."""
    orig = main.lift(main.word(off), off)
    if orig.kind in ("br", "blr", "ret", "asm"):
        raise ValueError(f"main+{off:#x}: the patched instruction is a {orig.kind}")
    lifted = [main.lift(w, off) for w in repl]
    for j, r in enumerate(lifted):
        if r.kind != "op" and not (j == len(lifted) - 1 and r.kind in ("bl", "b", "bcond", "ret")):
            raise ValueError(f"main+{off:#x}: replacement word {repl[j]:08x} is a {r.kind}")
    alts = []
    for ws in ([orig], lifted):
        fx = [e for e in (flag_effect(w) for w in ws) if e[0] or e[1]]
        if len(fx) > 1:
            raise ValueError(f"main+{off:#x}: several flag-using words in one alternative")
        alts.append(fx[0] if fx else (False, None))
    if alts[0][1] != alts[1][1]:
        raise ValueError(f"main+{off:#x}: writes other flags than the game's instruction")
    if (alts[0][1] or alts[0][0] or alts[1][0]) and (orig.kind == "bl" or lifted[-1].kind == "bl"):
        raise ValueError(f"main+{off:#x}: a patched call that uses flags")


def convert(writes: dict[int, int], main: Main) -> list[tuple[int, int, list[int]]]:
    """[(site offset, original word, replacement words)]; [] when every write puts back the game's own word."""
    cave = {a for a in writes if a >= main.text_end or main.word(a) == 0}
    sites = []
    for off in sorted(a for a in writes if a not in cave):
        w, orig = writes[off], main.word(off)
        if w == orig:
            continue
        br = branch(w, off)
        if br and br[1] in cave:
            kind, a = br
            body = []
            while True:
                if a not in writes or len(body) > CAVE_MAX:
                    raise ValueError(f"main+{off:#x}: its code cave at main+{br[1]:#x} has no end")
                cw = writes[a]
                if kind == "bl" and cw == RET:
                    break
                back = branch(cw, a)
                if kind == "b" and back and back[0] == "b" and back[1] == off + 4:
                    break
                if not movable(cw):
                    raise ValueError(f"main+{a:#x}: cave word {cw:08x} depends on its address")
                body.append(cw)
                a += 4
            repl = body or [NOP]
        else:
            repl = [w]   # in place: lifted at its own address, so a branch or adrp keeps its meaning
        check_site(main, off, repl)
        sites.append((off, orig, repl))
    return sites


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--exefs", type=Path, default=ROOT / "build" / "v130" / "exefs")
    ap.add_argument("--dist", type=Path, action="append", default=[], help="game folder (its mods/ and settings.ini)")
    ap.add_argument("--out", type=Path, required=True, help="patch lines for recomp/gen.py (set / site)")
    args = ap.parse_args(argv)
    gm = Main(args.exefs)
    bid = gm.nso.build_id[:16].upper()
    lines, sets, total = [], {}, 0
    for dist in args.dist:
        status = []
        for mod, path in cheat_files(dist, bid):
            for c in parse(path.read_text(encoding="utf-8", errors="replace")):
                named = f"{mod}/{c['name']}" + (f"#{c['dup']}" if c["dup"] > 1 else "")
                h = fnv(named.encode("utf-8"))
                content = fnv(" ".join(f"{w:08X}" for w in c["words"]).encode())
                try:
                    writes, other = code_writes(c["words"], gm)
                except ValueError as e:
                    status.append(f"{h:08X}\t{content:08X}\tunsupported\t0\t{e}")
                    continue
                if not writes:
                    continue
                if other:
                    status.append(f"{h:08X}\t{content:08X}\tunsupported\t0\tit mixes code writes with other opcodes")
                    continue
                try:
                    sites = convert(writes, gm)
                except (ValueError, Unsupported) as e:
                    status.append(f"{h:08X}\t{content:08X}\tunsupported\t0\t{e}")
                    print(f"cheats: {named}: not built: {e}")
                    continue
                if not sites:
                    status.append(f"{h:08X}\t{content:08X}\trestore\t0\t")
                    continue
                kind = "master" if c["master"] else "code"
                status.append(f"{h:08X}\t{content:08X}\t{kind}\t0\t")
                name = f"cheat_{h:08x}"
                if name not in sets:
                    sets[name] = [f"set  {name} ACR_CHEAT_{h:08X} {1 if c['master'] else 0}   # {named}"]
                    sets[name] += [f"site {name} main {off:x} {orig:08x} " + " ".join(f"{w:08x}" for w in repl)
                                   for off, orig, repl in sites]
                    total += 1
                    print(f"cheats: {named}: {len(sites)} sites")
        cache = dist / "build-cache"
        if status or (cache / "cheats.status").exists():
            cache.mkdir(parents=True, exist_ok=True)
            (cache / "cheats.status").write_text("".join(s + "\n" for s in status), encoding="utf-8")
    for v in sets.values():
        lines += v
    text = "# Code cheats of the mods (tools/cheats.py), generated: do not edit\n" + "".join(l + "\n" for l in lines)
    if not args.out.exists() or args.out.read_text(encoding="utf-8") != text:   # the mtime tells gen it changed
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(text, encoding="utf-8")
    print(f"cheats: {total} code cheats built into the game")
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""Age of Calamity Recomp build on Linux (WSL2 or native Linux): generates the game's C and compiles it with clang + ninja.

Steps (each one skips what is already done):

0. `extract` — when build/v130/exefs does not exist (new machine), extracts the ExeFS from the update NSP in "Game Files/".
1. `gen`   — recomp/gen.py: ExeFS 1.3.0 to C in <work>/generated/ (~600 MB, ~3 min).
2. `guest` — compiles all the generated C into objects and packs them into <work>/lib/libacr_guest.a. The ~35 giant
             functions of `main` (one file of 1-7 MB of C each) get flags that cut the LLVM passes that grow
             superlinearly (measured: ~40 % less time); they are compiled first so they do not hold up the end.
3. `dist`  — links the executable and assembles the single folder for the Deck.

Keep the work folder on the Linux disk (e.g. ~/AgeOfCalamityRecomp-build), not under /mnt/c or /mnt/d: compiling on the Windows
disk through WSL is many times slower. Only the finished folder (dist) goes to the Windows disk.

Usage (at the repository root):
    python3 tools/build.py --work ~/AgeOfCalamityRecomp-build            # gen + guest, on every core
    python3 tools/build.py --work ~/AgeOfCalamityRecomp-build --jobs 8   # limits the cores (each giant uses up to ~2-4 GB of RAM)
"""
from __future__ import annotations

import argparse
import json
import os
import shlex
import shutil
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
# Windows (MSYS2 UCRT64 Python + clang): ninja starts commands with CreateProcess, not a shell, so arguments take
# double quotes and there is no rm; the runtime takes its Win32 files (platform/win32.c, *_win.*). For tests on a
# Windows PC with a Vulkan GPU; the Steam Deck build is the Linux one (dist).
WIN = os.name == "nt"
EXE = "AgeOfCalamityRecomp.exe" if WIN else "AgeOfCalamityRecomp"

# Flags of the generated code, as in Astral Chain (tools/build_game.py GEN_FLAGS).
GEN_FLAGS = ["-O1", "-fno-strict-aliasing", "-Wno-return-type", "-Wno-unused-label", "-fvisibility=hidden",
             "-march=x86-64-v3"]
# For single-function files with more than BIG_BYTES of C: no machine sinking, no interval coalescing and no loop
# rotation. On main_0053.c (1 MB, 1 function): 89 s to 51 s, no error.
BIG_BYTES = 900_000
BIG_EXTRA = ["-mllvm", "-disable-machine-sink", "-mllvm", "-join-liveintervals=false",
             "-mllvm", "-rotation-max-header-size=0"]
MODULES = ("main", "subsdk0", "sdk")
# Lifter patch table (spin waits, then the game patches). Required: without the spin waits the game can hang.
PATCHES = ROOT / "recomp" / "patches-v130.txt"
# RAM a giant function compile can use (measured: 1.8 GB for the 5 MB one; the 6.6 MB one goes beyond).
BIG_MEM = 4 << 30
GEN_WORKER_MEM = 1536 << 20   # recomp/gen.py WORKER_MEM


def mem_available() -> int:
    """Free memory in bytes: Linux MemAvailable, or on Windows what tools/hostmem.py measures (0 when unknown). It sizes
    the generator workers and the pool of giant functions; at 0 both fall back to one at a time."""
    if WIN:
        import hostmem
        return hostmem.available_ram() or 0
    try:
        for line in Path("/proc/meminfo").read_text().splitlines():
            if line.startswith("MemAvailable:"):
                return int(line.split()[1]) * 1024
    except OSError:
        pass
    return 0


def need(tool: str) -> str:
    path = shutil.which(tool)
    if not path:
        sys.exit(f"ERROR: '{tool}' not found in PATH (run tools/setup_wsl.sh)")
    return path


def step_extract(args) -> None:
    """On a new machine the ExeFS does not exist yet: extracts it from the update NSP in "Game Files/" (tools/extract.py)."""
    game, keys = ROOT / "Game Files", ROOT / "Game Files" / "prod.keys"
    if args.exefs != ROOT / "build" / "v130" / "exefs":
        sys.exit(f"ERROR: ExeFS not found in {args.exefs}")
    if not keys.exists() or not any(game.rglob("*.nsp")):
        sys.exit("ERROR: the game code is not extracted yet and the dump was not found.\n"
                 f"  Put the NSPs (base, update 1.3.0 and DLCs) and prod.keys in: {game}\n"
                 "  (the 'Game Files' folder sits at the repository root and is ignored by git)")
    print("extract: ExeFS missing, extracting it from update 1.3.0 in 'Game Files/'")
    subprocess.run([sys.executable, str(ROOT / "tools" / "extract.py"), "inventory"], check=True)
    subprocess.run([sys.executable, str(ROOT / "tools" / "extract.py"), "exefs"], check=True)
    subprocess.run([sys.executable, str(ROOT / "tools" / "inspect_dump.py"), "--exefs", str(args.exefs)], check=True)


def cheat_patches(args) -> Path:
    """The patch table for the generator: recomp/patches-v130.txt and the code cheats of the game folders' enabled mods
    (tools/cheats.py, which also writes each folder's build-cache/cheats.status). Rewritten only when it changes, so
    its time tells step_gen whether the C is out of date."""
    cheats = args.work / "cheats-patches.txt"
    dists = [Path(args.dist)] + [d for d in (ROOT / "dist" / "windows",) if d.is_dir() and d != Path(args.dist)]
    cmd = [sys.executable, str(ROOT / "tools" / "cheats.py"), "--exefs", str(args.exefs), "--out", str(cheats)]
    for d in dists:
        cmd += ["--dist", str(d)]
    subprocess.run(cmd, check=True)
    table = args.work / "patches.txt"
    text = PATCHES.read_text(encoding="utf-8") + "\n" + cheats.read_text(encoding="utf-8")
    if not table.exists() or table.read_text(encoding="utf-8") != text:
        table.write_text(text, encoding="utf-8")
    return table


def step_gen(args) -> Path:
    out = args.work / "generated"
    manifests = [out / m / "manifest.json" for m in MODULES]
    table = cheat_patches(args)
    # Changes to the generator or the patch table call for new C; the generator only rewrites the files that changed.
    inputs = [table] + list((ROOT / "recomp").glob("*.py"))
    fresh = all(p.exists() for p in manifests) and \
        min(p.stat().st_mtime for p in manifests) > max(p.stat().st_mtime for p in inputs)
    if fresh and not args.regen:
        print("gen: C already generated and newer than recomp/ (use --regen to redo it)")
        return out
    cmd = [sys.executable, str(ROOT / "recomp" / "gen.py"), "--exefs", str(args.exefs), "--out", str(out),
           "--modules", ",".join(MODULES), "--patches", str(table)]
    # gen.py only measures free memory on Windows; here it limits the workers (each loads the modules: ~1.5 GB).
    jobs = args.jobs or max(1, min(os.cpu_count() or 1, int(mem_available() * 0.8) // GEN_WORKER_MEM or 1))
    cmd += ["--jobs", str(jobs)]
    print("gen:", " ".join(cmd))
    subprocess.run(cmd, check=True)
    return out


def ninja_escape(p: Path | str) -> str:
    """A path in a ninja `build` line (outputs and inputs)."""
    return str(p).replace("$", "$$").replace(" ", "$ ").replace(":", "$:")


def shell_arg(p: Path | str) -> str:
    """An argument inside a ninja variable value, which ninja passes to the shell (to CreateProcess on Windows)."""
    if WIN:
        return ('"' + str(p).replace("\\", "/") + '"').replace("$", "$$")
    return shlex.quote(str(p)).replace("$", "$$")


def step_guest(args, gen: Path) -> Path:
    cc, ar = need("clang"), shutil.which("llvm-ar") or need("ar")
    ninja = need("ninja")
    obj_dir = args.work / "obj"
    lib = args.work / "lib" / "libacr_guest.a"
    sources = []
    for m in MODULES:
        manifest = json.loads((gen / m / "manifest.json").read_text())
        info = {c["file"]: c for c in manifest["chunk_info"]}
        for name in manifest["chunks"] + [manifest["table"]]:
            c = info.get(name, {"functions": 0, "bytes": (gen / m / name).stat().st_size})
            big = c["functions"] == 1 and c["bytes"] > BIG_BYTES
            sources.append((gen / m / name, obj_dir / m / (Path(name).stem + ".o"), big, c["bytes"]))
    # Largest first: the giants start right away and do not hold up the end of the build.
    sources.sort(key=lambda s: -s[3])
    # Ninja variable values reach the shell as text: paths (the repository often has spaces, as in
    # "Age of Calamity") need shell quotes, and only "$" needs a ninja escape.
    flags = " ".join(GEN_FLAGS + ["-I", shell_arg(ROOT / "runtime"), "-I", shell_arg(gen)])
    # The giants run in a pool limited by free RAM, so several of them together do not exhaust WSL's memory.
    big_depth = args.big_jobs or max(1, int(mem_available() * 0.8) // BIG_MEM)
    lines = [
        "ninja_required_version = 1.10",
        f"cc = {cc}",
        f"ar = {ar}",
        f"cflags = {flags}",
        f"bigflags = {' '.join(BIG_EXTRA)}",
        "pool big",
        f"  depth = {big_depth}",
        "rule cc",
        "  command = $cc $cflags $extra -c $in -o $out",
        "  description = cc $in",
        "rule ar",
        "  command = $ar rcs $out @$out.rsp" if WIN else "  command = rm -f $out && $ar rcs $out @$out.rsp",
        "  rspfile = $out.rsp",
        "  rspfile_content = $in",
        "  description = ar $out",
    ]
    for src, obj, big, _ in sources:
        lines.append(f"build {ninja_escape(obj)}: cc {ninja_escape(src)}")
        if big:
            lines.append("  extra = $bigflags")
            lines.append("  pool = big")
    lines.append(f"build {ninja_escape(lib)}: ar " + " ".join(ninja_escape(o) for _, o, _, _ in sources))
    lines.append(f"default {ninja_escape(lib)}")
    for d in {o.parent for _, o, _, _ in sources} | {lib.parent}:
        d.mkdir(parents=True, exist_ok=True)
    build_file = args.work / "guest.ninja"
    build_file.write_text("\n".join(lines) + "\n")
    big = sum(1 for s in sources if s[2])
    print(f"guest: {len(sources)} C files ({big} with a giant function, up to {big_depth} at once) -> {lib}")
    if args.ninja_only:
        print(f"guest: only the ninja file was written ({build_file})")
        return lib
    t0 = time.time()
    cmd = [ninja, "-f", str(build_file)] + (["-j", str(args.jobs)] if args.jobs else [])
    subprocess.run(cmd, check=True, cwd=args.work)
    print(f"guest: done in {time.time() - t0:.0f} s ({lib.stat().st_size / 2**20:.0f} MiB)")
    return lib


# ---- host: runtime + executable (adapted from astral-chain-pc-recomp / totk-pc-recomp tools/build_game.py, MIT)
RUNTIME = ROOT / "runtime"
RT_FLAGS = ["-O2", "-g", "-Wall", "-fno-strict-aliasing", "-march=x86-64-v3"]
# -no-pie: the modules' function tables (hundreds of thousands of pointers) need no relocation at load.
# $ORIGIN: a libSDL3.so.0 next to the executable comes before the system's (the Deck folder).
LINK_FLAGS = ["-fuse-ld=lld", "-no-pie", "-Wl,-z,noexecstack", "'-Wl,-rpath,$$ORIGIN/lib'"]
LINK_LIBS = ["-lvulkan", "-lSDL3", "-lopus", "-lvpx", "-l:libstdc++.a", "-lm", "-lpthread", "-ldl"]
if WIN:   # as astral-chain-pc-recomp tools/build_game.py (MSYS2 UCRT64), plus the libraries folder_pick_win.cpp needs
    # MSYS2's VMA (newer than 3.4.0) marks nullability on some pointers only: hundreds of warnings per file.
    RT_FLAGS += ["-D_WIN32_WINNT=0x0A00", "-Wno-nullability-completeness"]
    LINK_FLAGS = ["-fuse-ld=lld", "-Wl,--stack,16777216"]
    LINK_LIBS = ["-lpsapi", "-lbcrypt", "-lvulkan-1", "-luser32", "-lgdi32", "-ldwmapi", "-limm32", "-lwinmm", "-lole32",
                 "-lshell32", "-luuid", "-lSDL3", "-lopus", "-l:libvpx.a", "-l:libstdc++.a",
                 "-l:libwinpthread.a"]
# vk_mem_alloc.h: the system's, else the version TotK Recomp pins (VMA 3.4.0, MIT), downloaded once to
# toolchain/vma and checked by hash.
VMA_FETCH = ("https://raw.githubusercontent.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/v3.4.0/include/vk_mem_alloc.h",
             "8487b7995ad3b263eb73bc5b9a77d71aa69b6bef5d58a715c02d2663afd81f1a")
GAME_VERSION = 130   # host.h rt_game_version
ROMFS_IMAGE = ROOT / "build" / "v130" / "romfs.bin"


def vma_include() -> Path:
    import hashlib
    import urllib.request
    if Path("/usr/include/vk_mem_alloc.h").is_file():
        return Path("/usr/include")
    if WIN:
        ucrt = Path(shutil.which("clang") or "C:/msys64/ucrt64/bin/clang.exe").parent.parent / "include" / "vma"
        if (ucrt / "vk_mem_alloc.h").is_file():
            return ucrt
    local = ROOT / "toolchain" / "vma"
    dst = local / "vk_mem_alloc.h"
    if not dst.is_file():
        url, sha = VMA_FETCH
        print(f"host: downloading VMA 3.4.0 to {local}")
        data = urllib.request.urlopen(url, timeout=60).read()
        if hashlib.sha256(data).hexdigest() != sha:
            sys.exit(f"ERROR: {url} does not match the pinned hash")
        local.mkdir(parents=True, exist_ok=True)
        dst.write_bytes(data)
    return local


def runtime_sources() -> list[Path]:
    """Every runtime/*.c, *.cpp, platform/*.c, ui/*.cpp and Dear ImGui, without the files of the other system (Win32); on
    Windows also runtime/AgeOfCalamityRecomp.rc (the AgeOfCalamityRecomp.exe manifest: per-monitor DPI and the UTF-8 code page)."""
    pats = ("*.c", "*.cpp", "*.rc", "platform/*.c", "ui/*.cpp", "third_party/imgui/*.cpp",
            "third_party/imgui/backends/*.cpp", "third_party/FidelityFX/sdk/src/shared/*.cpp",
            "third_party/FidelityFX/sdk/src/backends/vk/*.cpp", "third_party/FidelityFX/sdk/src/components/*/*.cpp")

    def other_os(p: Path) -> bool:
        if WIN:
            return p.stem.endswith("_sdl") or p.name == "linux.c"
        return p.stem.endswith("_win") or p.name == "win32.c" or p.suffix == ".rc"
    return sorted(p for pat in pats for p in RUNTIME.glob(pat) if not other_os(p))


FFX = RUNTIME / "third_party" / "FidelityFX"   # AMD's FidelityFX SDK, the FSR 3.1 upscaler's subset (its README.md)


def ffx_include() -> list[str]:
    """The SDK's public headers, ffx_shader_blobs.h and the upscaler's SPIR-V (runtime/vk_fsr3.cpp includes them)."""
    return [a for d in (FFX / "sdk" / "include", FFX / "sdk" / "src" / "backends" / "shared", FFX / "spirv")
            for a in ("-isystem", shell_arg(d))]


def ffx_rule() -> list[str]:
    """The SDK's own sources: ffx_compat.h force-included so they build with clang unchanged, their warnings off."""
    return ["rule cc_ffx",
            f"  command = $cc $rtflags -include ffx_compat.h -I {shell_arg(FFX)} "
            f"-I {shell_arg(FFX / 'sdk' / 'src' / 'shared')} -w -MD -MF $out.d -c $in -o $out",
            "  deps = gcc", "  depfile = $out.d", "  description = cc $in"]


def runtime_rule(src: Path) -> str:
    return "cc_ffx" if src.is_relative_to(FFX / "sdk") else "cc_rt"


def runtime_object(out: Path, src: Path) -> Path:
    """Object of a runtime source in <out>/obj; a .rc one gets .res.o (no clash with a .c of the same name)."""
    rel = src.relative_to(RUNTIME) if src.is_relative_to(RUNTIME) else Path(src.name)
    o = out / "obj" / rel.with_suffix(".res.o" if src.suffix == ".rc" else ".o")
    o.parent.mkdir(parents=True, exist_ok=True)
    return o


def c_string(s: str) -> str:
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def module_glue(gen: Path, exefs: Path) -> str:
    """C of modules.c: layout, module function tables (dispatch.h), expected ExeFS SHA-256, game version
    and default paths (host.h). The paths are this machine's; in the Deck folder the runtime finds exefs/, romfs/ and
    profile/ next to the executable (main.c portable_paths)."""
    sys.path.insert(0, str(ROOT / "tools"))
    from check_content import GENERATED_MARKER
    layout = json.loads((gen / "layout.json").read_text())
    manifest = json.loads((exefs.parent / "exefs-manifest.json").read_text())
    hashes = {f["name"]: f["sha256"] for f in manifest["files"]}
    out = [f"// Module glue written by tools/build.py; do not edit. {GENERATED_MARKER}",
           '#include "dispatch.h"', '#include "host.h"', '#include "layout.h"', ""]
    for m in layout:
        u = m["name"].upper()
        out.append(f'_Static_assert(MOD_{u}_BASE == {m["base"]}ull && MOD_{u}_SIZE == {m["size"]}ull, '
                   f'"layout.h e layout.json discordam em {m["name"]}");')
    out += ["", "const RtLayoutEntry rt_layout[] = {"]
    out += [f'    {{"{m["name"]}", MOD_{m["name"].upper()}_BASE, MOD_{m["name"].upper()}_SIZE}},' for m in layout]
    out += ["};", f"const int rt_layout_count = {len(layout)};", ""]
    for m in MODULES:
        out += [f"extern const uint64_t {m}_fn_addrs[];", f"extern const GuestFn {m}_fn_ptrs[];",
                f"extern const uint64_t {m}_fn_count;"]
    out += ["", "int rt_linked_modules(RtModule *out, int max) {", "    int n = 0;"]
    for m in MODULES:
        u = m.upper()
        out.append(f'    if (n < max) out[n++] = (RtModule){{"{m}", MOD_{u}_BASE, MOD_{u}_SIZE, {m}_fn_addrs, '
                   f"{m}_fn_ptrs, {m}_fn_count}};")
    out += ["    return n;", "}", "", "const RtFileHash rt_exefs_sha256[] = {"]
    out += [f"    {{{c_string(f)}, {c_string(h)}}}," for f, h in sorted(hashes.items())]
    out += ["};", f"const int rt_exefs_sha256_count = {len(hashes)};", ""]
    paths = {"exefs": exefs, "romfs": ROMFS_IMAGE.parent, "profile": ROMFS_IMAGE.parent / "profile"}
    for name, p in paths.items():
        out.append(f"const char rt_default_{name}[] = {c_string(str(p))};")
    version = (ROOT / "VERSION").read_text().strip() if (ROOT / "VERSION").is_file() else "unknown"
    out += [f"const int rt_game_version = {GAME_VERSION};", f"const char rt_port_version[] = {c_string(version)};", ""]
    return "\n".join(out)


def write_if_changed(path: Path, text: str) -> None:
    # Always UTF-8: on Windows the default depends on Python's UTF-8 mode (tools/make_build.py turns it on), and a file
    # written in the old code page does not decode; unreadable counts as changed.
    data = text.encode("utf-8")
    if path.exists() and path.read_bytes() == data:
        return
    path.write_bytes(data)


def step_host(args, gen: Path, lib: Path) -> Path:
    """Compiles the runtime and links <work>/host/AgeOfCalamityRecomp with libacr_guest.a."""
    cc, ninja = need("clang"), need("ninja")
    out = args.work / "host"
    out.mkdir(parents=True, exist_ok=True)
    write_if_changed(out / "modules.c", module_glue(gen, args.exefs))
    sys.path.insert(0, str(ROOT / "tools"))
    import romfs_index
    if ROMFS_IMAGE.is_file():
        n, size, dt = romfs_index.write(ROMFS_IMAGE, out)
        print(f"host: RomFS index with {n} files ({dt:.1f} s)")
    else:
        romfs_index.write_placeholder(out)
        print(f"host: WARNING: {ROMFS_IMAGE} does not exist; the executable compiles but does not start the game")
    inc = ["-I", shell_arg(RUNTIME), "-I", shell_arg(gen), "-I", shell_arg(out), "-I", shell_arg(vma_include()),
           "-I", shell_arg(RUNTIME / "third_party" / "imgui")] + ffx_include()
    lines = [
        "ninja_required_version = 1.10",
        f"builddir = {ninja_escape(out)}",
        f"cc = {cc}",
        f"rtflags = {' '.join(RT_FLAGS + inc)}",
        f"ldflags = {' '.join(LINK_FLAGS)}",
        f"libs = {' '.join(LINK_LIBS)}",
        "rule cc_rt",
        "  command = $cc $rtflags -MD -MF $out.d -c $in -o $out",
        "  deps = gcc",
        "  depfile = $out.d",
        "  description = cc $in",
        "rule link",
        "  command = $cc $ldflags @$out.rsp $libs -o $out",
        "  rspfile = $out.rsp",
        "  rspfile_content = $in_newline",
        "  description = link $out",
    ] + ffx_rule()
    if WIN:   # runtime/AgeOfCalamityRecomp.rc, as in Astral Chain's tools/build_game.py (MIT); windres writes no depfile
        windres = shutil.which("llvm-windres") or need("windres")
        lines += ["rule rc", f"  command = {windres} -I {shell_arg(RUNTIME)} -I {shell_arg(ROOT / 'assets')} -O coff $in -o $out",
                  "  description = rc $in"]
    objs = []
    for src in runtime_sources() + [out / "modules.c"]:
        o = runtime_object(out, src)
        if src.suffix == ".rc":
            lines.append(f"build {ninja_escape(o)}: rc {ninja_escape(src)} | {ninja_escape(RUNTIME / 'AgeOfCalamityRecomp.manifest')} "
                         f"{ninja_escape(ROOT / 'assets' / 'icon.ico')}")
        else:
            lines.append(f"build {ninja_escape(o)}: {runtime_rule(src)} {ninja_escape(src)}")
        objs.append(o)
    exe = out / EXE
    lines.append(f"build {ninja_escape(exe)}: link " + " ".join(ninja_escape(o) for o in objs) + f" {ninja_escape(lib)}")
    lines.append(f"build runtime: phony " + " ".join(ninja_escape(o) for o in objs))
    lines.append(f"default {ninja_escape(exe)}")
    build_file = args.work / "host.ninja"
    build_file.write_text("\n".join(lines) + "\n")
    target = ["runtime"] if args.runtime_only else []
    t0 = time.time()
    cmd = [ninja, "-f", str(build_file)] + (["-j", str(args.jobs)] if args.jobs else []) + ["-k", "0"] + target
    subprocess.run(cmd, check=True, cwd=args.work)
    print(f"host: done in {time.time() - t0:.0f} s -> {exe if not args.runtime_only else 'runtime objects'}")
    return exe


# ---- dist: the single Steam Deck folder, compiled against the Steam Runtime 3 "sniper" sysroot (glibc 2.31), as
# TotK Recomp's tools/make_release_linux.py (MIT). WSL's Arch/Ubuntu has a newer glibc than SteamOS; an executable
# linked against it would ask for GLIBC_2.3x/2.4x symbols the Deck does not have.
SNIPER = "3.0.20261021.266025"
SNIPER_TAR = (f"https://repo.steampowered.com/steamrt-images-sniper/snapshots/{SNIPER}/"
              "com.valvesoftware.SteamRuntime.Sdk-amd64%2Ci386-sniper-sysroot.tar.gz",
              "8f71947035f97925a311b2ed753f22efcf2f13d6f1631945b8e537f8903ce2cf")
SNIPER_MEMBERS = ["usr/include", "usr/share/pkgconfig", "usr/lib/x86_64-linux-gnu", "usr/lib/gcc/x86_64-linux-gnu",
                  "lib/x86_64-linux-gnu", "lib64"]
# Opus and libvpx static from the sysroot (SteamOS has another libvpx.so), SDL3 dynamic
# copied to lib/ next to the executable, Vulkan from the system (the loader is stable).
DIST_LIBS = ["-lvulkan", "-lSDL3", "-l:libopus.a", "-l:libvpx.a", "-l:libstdc++.a", "-lm", "-lpthread",
             "-ldl"]
GLIBC_MAX = (2, 31)


def fetch(url_sha: tuple[str, str], dst: Path) -> Path:
    import hashlib
    import urllib.request
    if dst.is_file():
        return dst
    url, sha = url_sha
    dst.parent.mkdir(parents=True, exist_ok=True)
    print(f"dist: downloading {url}")
    part = dst.with_suffix(dst.suffix + ".part")
    subprocess.run(["curl", "-sL", "--http1.1", "--retry", "5", "-C", "-", "-o", str(part), url], check=True)
    h = hashlib.sha256()
    with part.open("rb") as f:
        while chunk := f.read(16 << 20):
            h.update(chunk)
    if h.hexdigest() != sha:
        part.unlink()
        sys.exit(f"ERROR: {url} does not match the pinned hash")
    part.rename(dst)
    return dst


def sniper_sysroot(work: Path) -> Path:
    """The sysroot extracted in <work>/toolchain/sniper, with absolute symbolic links made relative (the SDK points
    some at /lib and /usr/lib, which outside the container would be the system's)."""
    root = work / "toolchain" / "sniper"
    if (root / ".relinked").is_file():
        return root
    tar = fetch(SNIPER_TAR, work / "dl" / "sniper-sysroot.tar.gz")
    shutil.rmtree(root, ignore_errors=True)
    root.mkdir(parents=True)
    print("dist: extracting the sniper sysroot")
    subprocess.run(["tar", "-xf", str(tar), "-C", str(root), "--wildcards", "--no-wildcards-match-slash",
                    *SNIPER_MEMBERS], check=True)
    for p in root.rglob("*"):
        if p.is_symlink() and os.readlink(p).startswith("/"):
            target = root / os.readlink(p).lstrip("/")
            p.unlink()
            p.symlink_to(os.path.relpath(target, p.parent))
    (root / ".relinked").write_text("")
    return root


def dist_deps(work: Path, cc: list[str]) -> Path:
    """<work>/deps: Vulkan 1.4.363 headers and VMA (headers only)."""
    deps = work / "deps"
    inc, lib = deps / "include", deps / "lib"
    inc.mkdir(parents=True, exist_ok=True)
    lib.mkdir(parents=True, exist_ok=True)
    for d in ("vulkan", "vk_video"):
        if not (inc / d).is_dir():
            if not Path(f"/usr/include/{d}").is_dir():
                sys.exit("ERROR: the Vulkan headers are missing (pacman -S vulkan-headers / apt install libvulkan-dev)")
            shutil.copytree(f"/usr/include/{d}", inc / d)
    if not (inc / "vk_mem_alloc.h").is_file():
        shutil.copy(vma_include() / "vk_mem_alloc.h", inc / "vk_mem_alloc.h")
    return deps


def glibc_needed(exe: Path) -> tuple[int, int]:
    """The highest GLIBC_x.y version the executable asks for."""
    out = subprocess.run(["llvm-readelf" if shutil.which("llvm-readelf") else "readelf", "-V", str(exe)],
                         capture_output=True, text=True, check=True).stdout
    import re
    vers = [tuple(map(int, m)) for m in re.findall(r"GLIBC_(\d+)\.(\d+)", out)]
    return max(vers) if vers else (0, 0)


def step_dist(args, gen: Path, lib: Path) -> Path:
    """Compiles the runtime, links it against the sniper and assembles dist/steamdeck/ (layout in docs/NOTES.md)."""
    clang, ninja = need("clang"), need("ninja")
    sysroot = sniper_sysroot(args.work)
    cc = [clang, "--target=x86_64-linux-gnu", f"--sysroot={sysroot}"]
    deps = dist_deps(args.work, cc)
    out = args.work / "dist"
    out.mkdir(parents=True, exist_ok=True)
    host = args.work / "host"   # modules.c and romfs_config.h of the host step
    host.mkdir(parents=True, exist_ok=True)
    write_if_changed(host / "modules.c", module_glue(gen, args.exefs))
    sys.path.insert(0, str(ROOT / "tools"))
    import romfs_index
    if not ROMFS_IMAGE.is_file():
        sys.exit(f"ERROR: {ROMFS_IMAGE} does not exist (tools/merge_romfs.py)")
    romfs_index.write(ROMFS_IMAGE, host)
    inc = ["-I", shell_arg(deps / "include"), "-I", shell_arg(RUNTIME), "-I", shell_arg(gen), "-I", shell_arg(host),
           "-I", shell_arg(RUNTIME / "third_party" / "imgui")] + ffx_include()
    ccs = " ".join(shell_arg(a) for a in cc)
    lines = [
        "ninja_required_version = 1.10",
        f"builddir = {ninja_escape(out)}",
        f"cc = {ccs}",
        f"rtflags = {' '.join(RT_FLAGS + inc)}",
        f"ldflags = -fuse-ld=lld -no-pie -Wl,-z,noexecstack '-Wl,-rpath,$$ORIGIN/lib' -L {shell_arg(deps / 'lib')}",
        f"libs = {' '.join(DIST_LIBS)}",
        "rule cc_rt",
        "  command = $cc $rtflags -MD -MF $out.d -c $in -o $out",
        "  deps = gcc",
        "  depfile = $out.d",
        "  description = cc $in",
        "rule link",
        "  command = $cc $ldflags @$out.rsp $libs -o $out",
        "  rspfile = $out.rsp",
        "  rspfile_content = $in_newline",
        "  description = link $out",
    ] + ffx_rule()
    objs = []
    for src in runtime_sources() + [host / "modules.c"]:
        o = runtime_object(out, src)
        lines.append(f"build {ninja_escape(o)}: {runtime_rule(src)} {ninja_escape(src)}")
        objs.append(o)
    exe = out / "AgeOfCalamityRecomp"
    lines.append(f"build {ninja_escape(exe)}: link " + " ".join(ninja_escape(o) for o in objs) + f" {ninja_escape(lib)}")
    lines.append(f"default {ninja_escape(exe)}")
    build_file = args.work / "dist.ninja"
    build_file.write_text("\n".join(lines) + "\n")
    t0 = time.time()
    subprocess.run([ninja, "-f", str(build_file)] + (["-j", str(args.jobs)] if args.jobs else []), check=True,
                   cwd=args.work)
    need_glibc = glibc_needed(exe)
    if need_glibc > GLIBC_MAX:
        sys.exit(f"ERROR: {exe} needs GLIBC_{need_glibc[0]}.{need_glibc[1]}, above the sniper's {GLIBC_MAX}")
    print(f"dist: executable ready in {time.time() - t0:.0f} s, needs GLIBC_{need_glibc[0]}.{need_glibc[1]} at most")
    # The shader translator (Ryujinx 1.3.2, NativeAOT), also against the sniper: the runtime looks for it next to AgeOfCalamityRecomp
    # (gfx_shader.c init_paths). tools/build_xlate.py redoes the identity on each build; here only when missing.
    xl = args.work / "xlate"
    # The launcher package carries the translator ready (prebuilt/xlate/linux-x64, made by tools/make_release.py from the
    # same commit): whoever forges the game needs no .NET. With --xlate it is always rebuilt.
    pre = ROOT / "prebuilt" / "xlate" / "linux-x64"
    if not args.xlate and (pre / "xlate").is_file() and (pre / "xlate.id").is_file():
        xl.mkdir(parents=True, exist_ok=True)
        if not (xl / "xlate.id").is_file() or (xl / "xlate.id").read_bytes() != (pre / "xlate.id").read_bytes():
            shutil.copy2(pre / "xlate", xl / "xlate")
            shutil.copy2(pre / "xlate.id", xl / "xlate.id")
            print(f"dist: shader translator ready from {pre}")
    if not (xl / "xlate").is_file() or args.xlate:
        subprocess.run([sys.executable, str(ROOT / "tools" / "build_xlate.py"), "--out", str(xl),
                        "--sysroot", str(sysroot)], check=True)
    need_glibc = glibc_needed(xl / "xlate")
    if need_glibc > GLIBC_MAX:
        sys.exit(f"ERROR: xlate needs GLIBC_{need_glibc[0]}.{need_glibc[1]}")
    return assemble_dist(args, exe, sysroot, host, xl)


LAUNCHER = """#!/bin/sh
# Age of Calamity Recomp starter in the Deck folder (tools/build.py dist). Driver caches go next to the game, on the
# SD card, not in ~/.cache (internal storage).
here="$(cd "$(dirname "$0")" && pwd)"
export MESA_SHADER_CACHE_DIR="$here/mesa-cache"
export MESA_SHADER_CACHE_MAX_SIZE=4G
mkdir -p "$here/mesa-cache" "$here/logs"
# FTP from Windows drops the executable bit: make sure the executables run
[ -x "$here/AgeOfCalamityRecomp" ] || chmod +x "$here/AgeOfCalamityRecomp" 2>/dev/null
[ -x "$here/xlate" ] || chmod +x "$here/xlate" 2>/dev/null
# the previous session goes compressed to logs/sessions/ (named after the time it ended; the 60 newest are kept);
# ./logs.sh lists and sums them up
if [ -s "$here/logs/game.log" ]; then
    mkdir -p "$here/logs/sessions"
    ts=$(date -r "$here/logs/game.log" +%Y%m%d-%H%M%S)
    [ -f "$here/logs/sessions/$ts.log.gz" ] || gzip -c "$here/logs/game.log" > "$here/logs/sessions/$ts.log.gz"
    ls -1t "$here/logs/sessions/"*.log.gz 2>/dev/null | tail -n +61 | xargs -r rm -f
fi
cd "$here"
exec "$here/AgeOfCalamityRecomp" "$@"
"""


# First settings of the Deck folder (cfg.c: "key = value", without the ACR_ prefix). Frame rate and mode come from the
# runtime's "deck" profile (30 fps, handheld).
INI_DECK = """# Age of Calamity Recomp settings. The menu (Minus+Plus) and the quick menu (hold L3+R3) write here.
"""

README_DECK = """Hyrule Warriors: Age of Calamity - native port for the Steam Deck (AgeOfCalamityRecomp)
=====================================================================================

Personal use only: this folder contains the game (from your own console). Do not share it.

INSTALL (once)
1. On the Deck, in Desktop Mode, copy the whole "steamdeck" folder to the SD card (FTP, cloud or cable).
   Example: /run/media/mmcblk0p1/steamdeck   (romfs/romfs.bin is 10.9 GiB)
2. Open Konsole inside the folder and run once:   sh install-shortcut.sh
   (it makes the files executable and puts the game in the SteamOS application menu)
3. Steam > Games > "Add a Non-Steam Game" > tick
   "Hyrule Warriors: Age of Calamity (AgeOfCalamityRecomp)" > Add Selected Programs.
4. Back in Game Mode, the game is in your library under "Non-Steam".

FIRST TEST (Konsole, inside the folder)
  ./AgeOfCalamityRecomp --selftest                         should end with "selftest: passed"
  ACR_GPU=null ACR_TRACE=perf ./AgeOfCalamityRecomp.sh     no picture; should print "fps: ... (30.0 fps)" lines (Ctrl+C to quit)
  ./AgeOfCalamityRecomp.sh                                 the game (the first time translates shaders: stutters until the cache fills)

LOGS (power, temperature and fps of each session)
  ./logs.sh list                       one line per session played: date, minutes, fps, watts, temperatures
  ./logs.sh                            summary of the last session (./logs.sh summary 2: the one before)
  ./logs.sh live                       follows the running session (from another terminal or over SSH)
  ./logs.sh system                     TDP, watts, temperatures and clocks right now
  ./logs.sh detailed on|off            more performance detail in the next sessions
  ./logs.sh performance                where the last session lost fps: stutters, causes, power, marks
  ./logs.sh diag                       reads the freeze and black screen reports of the last session
  ./logs.sh bundle                     packs everything into logs/bundle-<date>.tar.gz to send

REPORTING A PROBLEM
  Run ./logs.sh bundle and attach the file it makes to a report at
  https://github.com/eliasm4r/AgeOfCalamityRecomp/issues
  (never attach game files, keys or saves)

PORT CONTROLS
  Hold L3+R3 (~1.5 s)    quick menu: frame rate, internal resolution, performance overlay and
                         "Mark this moment" (marks a stutter in the performance log)
  Tap L3+R3              FPS overlay on or off
  Minus + Plus together  full menu
  B                      closes the menus

GOING EASY ON THE DECK
  - Default: 30 FPS (the game's own rate) and "Handheld" internal resolution (the game's lightest).
  - Steam's performance menu ("..." button > battery) can also limit the TDP and the refresh rate.

FILES
  AgeOfCalamityRecomp, AgeOfCalamityRecomp.sh, lib/            the program
  exefs/, romfs/, romfs-index.bin   the game (romfs/dlc-*.bin are the DLCs; without them the game runs without DLC)
  settings.ini                      your settings (never overwritten by updates)
  profile/, logs/, mesa-cache/ saves, logs and shader cache (created while playing)
  logs.sh, tools/          log readers (logs/sessions/ keeps the earlier sessions)
UPDATING: copy only "AgeOfCalamityRecomp" over the old one (and "lib/" if it changed). The romfs does not change.
"""


# Registers the game as a desktop application (~/.local/share/applications) with the folder's current path: the
# "Add a Non-Steam Game" window lists those applications, and a loose script on the SD card does not show in it.
INSTALL_SH = """#!/bin/sh
# Creates the Age of Calamity Recomp shortcut in the SteamOS application menu. Run once (Desktop Mode, Konsole):
#   sh install-shortcut.sh
# Then: Steam > Games > "Add a Non-Steam Game" > tick "Hyrule Warriors: Age of Calamity".
here="$(cd "$(dirname "$0")" && pwd)"
chmod +x "$here/AgeOfCalamityRecomp" "$here/AgeOfCalamityRecomp.sh" "$here/xlate"
mkdir -p "$HOME/.local/share/applications"
f="$HOME/.local/share/applications/AgeOfCalamityRecomp.desktop"
printf '%s\\n' '[Desktop Entry]' 'Type=Application' 'Name=Hyrule Warriors: Age of Calamity (AgeOfCalamityRecomp)' \\
    'Comment=Native port (personal use)' "Exec=$here/AgeOfCalamityRecomp.sh" "Path=$here" 'Terminal=false' 'Categories=Game;' \\
    'Icon=applications-games' > "$f"
chmod +x "$f"
update-desktop-database "$HOME/.local/share/applications" 2>/dev/null
echo "Shortcut created: $f"
echo "Now: Steam > Games > Add a Non-Steam Game > tick Hyrule Warriors: Age of Calamity"
"""


def assemble_dist(args, exe: Path, sysroot: Path, host: Path, xl: Path) -> Path:
    """dist/steamdeck/: AgeOfCalamityRecomp, AgeOfCalamityRecomp.sh, lib/libSDL3.so.0, exefs/, romfs-index.bin and romfs/ (the 10.9 GiB romfs.bin is only
    copied with --with-romfs: on the Deck copying it once is enough; the DLC images always go). Saves, settings.ini, logs/ and mesa-cache/ are never touched."""
    dist = Path(args.dist).expanduser().resolve()
    (dist / "lib").mkdir(parents=True, exist_ok=True)
    (dist / "exefs").mkdir(exist_ok=True)
    (dist / "romfs").mkdir(exist_ok=True)

    def copy(src: Path, dst: Path) -> None:
        if dst.exists() and dst.stat().st_size == src.stat().st_size and dst.stat().st_mtime >= src.stat().st_mtime:
            return
        shutil.copy2(src, dst)

    copy(exe, dist / "AgeOfCalamityRecomp")
    (dist / "AgeOfCalamityRecomp").chmod(0o755)
    copy(xl / "xlate", dist / "xlate")
    (dist / "xlate").chmod(0o755)
    shutil.copy2(xl / "xlate.id", dist / "xlate.id")
    copy((sysroot / "usr/lib/x86_64-linux-gnu/libSDL3.so.0").resolve(), dist / "lib" / "libSDL3.so.0")
    for f in ("main", "subsdk0", "sdk", "main.npdm"):
        copy(args.exefs / f, dist / "exefs" / f)
    copy(host / "romfs-index.bin", dist / "romfs-index.bin")
    (dist / "AgeOfCalamityRecomp.sh").write_text(LAUNCHER)
    (dist / "AgeOfCalamityRecomp.sh").chmod(0o755)
    (dist / "README.txt").write_text(README_DECK, encoding="utf-8")
    (dist / "install-shortcut.sh").write_text(INSTALL_SH, encoding="utf-8")
    (dist / "install-shortcut.sh").chmod(0o755)
    # Deck logs: logs.sh (list, summary, live, bundle) and the readers it uses.
    (dist / "tools").mkdir(exist_ok=True)
    shutil.copy2(ROOT / "tools" / "deck" / "logsum.py", dist / "tools" / "logsum.py")
    shutil.copy2(ROOT / "tools" / "diag_view.py", dist / "tools" / "diag_view.py")   # logs.sh diag
    shutil.copy2(ROOT / "tools" / "perf_view.py", dist / "tools" / "perf_view.py")   # logs.sh performance
    shutil.copy2(ROOT / "tools" / "deck" / "logs.sh", dist / "logs.sh")
    (dist / "logs.sh").chmod(0o755)
    if not (dist / "settings.ini").exists():   # the first time only: afterwards the file is the player's (the menu writes it)
        (dist / "settings.ini").write_text(INI_DECK, encoding="utf-8")
    if args.with_romfs:
        dst = dist / "romfs" / "romfs.bin"
        if not (dst.exists() and dst.stat().st_size == ROMFS_IMAGE.stat().st_size):
            dst.unlink(missing_ok=True)
            try:   # same disk: a hard link does not take another 10.9 GiB
                os.link(ROMFS_IMAGE, dst)
                print(f"dist: romfs.bin linked (hard link) to {ROMFS_IMAGE}")
            except OSError:
                print(f"dist: copying {ROMFS_IMAGE} ({ROMFS_IMAGE.stat().st_size / 2**30:.1f} GiB)")
                shutil.copy2(ROMFS_IMAGE, dst)
    elif not (dist / "romfs" / "romfs.bin").exists():
        print(f"dist: WARNING: copy {ROMFS_IMAGE} to {dist / 'romfs' / 'romfs.bin'} once (or use --with-romfs)")
    # DLCs (tools/extract_dlc.py): the images next to romfs.bin always go (1.4 GiB in all); the runtime
    # (runtime/dlc.c) shows the game the DLCs it finds in romfs/.
    dlcs = sorted(ROMFS_IMAGE.parent.glob("dlc-*.bin"))
    for src in dlcs:
        dst = dist / "romfs" / src.name
        if dst.exists() and dst.stat().st_size == src.stat().st_size and dst.stat().st_mtime >= src.stat().st_mtime:
            continue
        dst.unlink(missing_ok=True)
        try:
            os.link(src, dst)
        except OSError:
            shutil.copy2(src, dst)
    print(f"dist: {len(dlcs)} DLCs in romfs/" if dlcs else
          "dist: no DLC extracted (python3 tools/extract_dlc.py); the game runs without them")
    print(f"dist: folder ready in {dist}")
    return dist


def touch_changed_sources(work: Path) -> None:
    """Gives a fresh date to every source whose content changed since the last build in this work folder. The build
    steps compare dates (step_gen, ninja), but a release unpacked over an older folder keeps the dates of the archive,
    which can be older than the previous build's outputs (another time zone, a clock behind): nothing was rebuilt and
    the old executable ran on (issue #13). <work>/source-hashes.json keeps size, date and SHA-256 per file; a file
    whose size or date moved is hashed again, and touched if its content differs. The first run touches everything."""
    import hashlib
    stamp = work / "source-hashes.json"
    try:
        old = json.loads(stamp.read_text())
    except (OSError, ValueError):
        old = {}
    new, touched = {}, 0
    for top in (RUNTIME, ROOT / "recomp", ROOT / "assets"):
        for p in top.rglob("*"):
            if not p.is_file() or "__pycache__" in p.parts:
                continue
            key = p.relative_to(ROOT).as_posix()
            st = p.stat()
            prev = old.get(key)
            if prev and prev[0] == st.st_size and prev[1] == st.st_mtime_ns:
                new[key] = prev
                continue
            digest = hashlib.sha256(p.read_bytes()).hexdigest()
            if not prev or prev[2] != digest:
                os.utime(p)
                touched += 1
                st = p.stat()
            new[key] = [st.st_size, st.st_mtime_ns, digest]
    stamp.write_text(json.dumps(new, sort_keys=True))
    if touched:
        print(f"sources: {touched} files new or changed since the last build here, dated now so they build again")


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--work", type=Path, required=True, help="build folder on the Linux disk (e.g. ~/AgeOfCalamityRecomp-build)")
    ap.add_argument("--exefs", type=Path, default=ROOT / "build" / "v130" / "exefs")
    ap.add_argument("--jobs", type=int, default=0, help="cores (default: all)")
    ap.add_argument("--regen", action="store_true", help="generate the C again")
    ap.add_argument("--big-jobs", type=int, default=0, help="giant functions compiled at once (default: from free RAM)")
    ap.add_argument("--ninja-only", action="store_true", help="only writes guest.ninja, without compiling")
    ap.add_argument("--runtime-only", action="store_true", help="host: only compiles the runtime, without linking")
    ap.add_argument("--dist", default=str(ROOT / "dist" / "steamdeck"), help="dist: output folder for the Deck")
    ap.add_argument("--xlate", action="store_true", help="dist: rebuilds the shader translator")
    ap.add_argument("--with-romfs", action="store_true", help="dist: also puts romfs.bin in (hard link on the same disk, else a copy)")
    ap.add_argument("steps", nargs="*", help="gen, guest, host, dist (default: gen, guest, host)")
    args = ap.parse_args(argv)
    args.steps = args.steps or ["gen", "guest", "host"]
    if set(args.steps) - {"gen", "guest", "host", "dist"}:
        ap.error("valid steps: gen, guest, host, dist")
    args.work = args.work.expanduser().resolve()
    args.work.mkdir(parents=True, exist_ok=True)
    if not (args.exefs / "main").exists():
        step_extract(args)
    touch_changed_sources(args.work)
    gen = step_gen(args) if "gen" in args.steps else args.work / "generated"
    lib = step_guest(args, gen) if "guest" in args.steps else args.work / "lib" / "libacr_guest.a"
    if "host" in args.steps:
        step_host(args, gen, lib)
    if "dist" in args.steps:
        step_dist(args, gen, lib)
    return 0


if __name__ == "__main__":
    sys.exit(main())

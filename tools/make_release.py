"""Builds the downloadable packages for a GitHub release: one for Windows, one for Linux.

The sources are the repository at HEAD (git archive: our code, the setup in setup/, no game data) plus what a player's
PC cannot be asked to build before the setup runs: prebuilt/hactool/ (hactool 1.4.0, ISC, for the RomFS merge) and
prebuilt/xlate/ (the shader translator, Ryujinx 1.3.2 + our patches, MIT, so nobody needs .NET).

  Windows  AgeOfCalamityRecomp-<v>-Windows.zip: an installer, as WondersBeyondRecomp's is
             AgeOfCalamityRecomp Setup.exe     setup/launcher.c with the game icon: starts python/Setup (if blocked).exe
             python/Setup (if blocked).exe     the setup itself: pythonw.exe under another name (its signature kept, so
                                               Smart App Control lets it start), whose name makes sitecustomize.py open
                                               installer/setup.py
             python/                           Python 3.13 embeddable + Tcl/Tk (tcltk.msi) + cryptography, lz4, numpy
                                               (runs the setup, make_build.py, the dump extraction, the C generator)
             installer/                        setup.py, texts.py, sitecustomize.py, icon.ico, and sources.zip: the
                                               sources with prebuilt/{hactool/win-x64, xlate/win-x64, xlate/linux-x64}
                                               (linux-x64 for the Deck build in WSL), unpacked by the setup into the
                                               install folder the player picks
             LICENSE, THIRD_PARTY_NOTICES.md, recomplauncher.json
  Linux    AgeOfCalamityRecomp-<v>-Linux.tar.gz: the sources as they are, with prebuilt/hactool/linux-x64 and
           prebuilt/xlate/linux-x64 (the setup is setup.sh, with the system python3)

Run on Windows in MSYS2 UCRT64, from a clean commit, with the development WSL distro set up (its ~/AgeOfCalamityRecomp-build has the
Linux translator, its venv has pip):

  python3 tools/make_release.py [--wsl-distro archlinux] [--win-xlate C:/AgeOfCalamityRecomp-win/host] [--allow-dirty]
  python tools/make_release.py --windows-only [--win-xlate DIR]   # no WSL: the Windows zip without the Linux
                                                                  # translator (no Deck build), with a Python that has pip
  python tools/make_release.py --local    # only AgeOfCalamityRecomp Setup.exe and python/ in this folder, to try the setup

Output in build/release/; the sha256 of each package is printed and written next to it.
"""
import argparse
import hashlib
import io
import shutil
import subprocess
import sys
import tarfile
import time
import urllib.request
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "build" / "release"
DL = OUT / "dl"
TOP = "AgeOfCalamityRecomp"   # the folder each package unpacks into
PY_EMBED = ("https://www.python.org/ftp/python/3.13.17/python-3.13.17-embed-amd64.zip",
            "9e35319bffedab8b36ad44d8f377cb4f1733bcec257a0b6d04d94f01486ddb38")
# Tcl/Tk for tkinter (the setup window), from the same Python release; unpacked with msiexec /a, never installed.
TCLTK = ("https://www.python.org/ftp/python/3.13.17/amd64/tcltk.msi",
         "ae3e5f9dd998b36d14655cd3444d0af906c1849c60aee40994eeb1e460a3cabf")
WHEELS = ["cryptography", "lz4", "numpy", "zstandard"]
# MSYS2 has no git of its own by default: Git for Windows, else the one on PATH.
GIT = next((g for g in ("C:/Program Files/Git/cmd/git.exe",) if Path(g).is_file()), None) or shutil.which("git") or "git"
# Python's search path for every exe in python/: with a ._pth file the embeddable Python ignores the script's folder, so
# tools/ and recomp/ are listed (no module name is in both). Python reads python313._pth (the DLL's name) before a
# ._pth named after the exe, so this one file also serves "Setup (if blocked).exe": setup/ is listed for its
# sitecustomize.py, which opens the setup only when the exe's name does not start with "python".
PTH = "python313.zip\n.\nLib\nLib\\site-packages\n..\\installer\n..\\setup\n..\\tools\n..\\recomp\nimport site\n"
SETUP_EXE = "AgeOfCalamityRecomp Setup"   # setup/launcher.c, with the game icon
SIGNED = "Setup (if blocked)"             # python/pythonw.exe under another name, its signature kept
# What the Windows installer carries besides python/ and the sources: (path at HEAD, path in the package)
INSTALLER = [("setup/setup.py", "installer/setup.py"), ("setup/texts.py", "installer/texts.py"),
             ("setup/sitecustomize.py", "installer/sitecustomize.py"), ("assets/icon.ico", "installer/icon.ico"),
             ("LICENSE", "LICENSE"), ("THIRD_PARTY_NOTICES.md", "THIRD_PARTY_NOTICES.md"),
             ("recomplauncher.json", "recomplauncher.json")]


def sh(cmd, **kw):
    print("+", " ".join(str(c) for c in cmd), flush=True)
    return subprocess.run([str(c) for c in cmd], check=True, **kw)


def wsl(distro: str, script: str) -> None:
    sh(["wsl", "-d", distro, "-e", "bash", "-lc", script])


def wsl_path(p: Path) -> str:
    s = str(p.resolve()).replace("\\", "/")
    return f"/mnt/{s[0].lower()}{s[2:]}" if len(s) > 1 and s[1] == ":" else s


def fetch(url: str, sha: str) -> bytes:
    dst = DL / url.rsplit("/", 1)[1]
    if not dst.is_file() or hashlib.sha256(dst.read_bytes()).hexdigest() != sha:
        DL.mkdir(parents=True, exist_ok=True)
        print(f"downloading {url}", flush=True)
        data = urllib.request.urlopen(url, timeout=120).read()
        if hashlib.sha256(data).hexdigest() != sha:
            sys.exit(f"ERROR: {url} does not match the pinned hash")
        dst.write_bytes(data)
    return dst.read_bytes()


def git_tree() -> list:
    """(path, mode, bytes) of every file at HEAD, from git archive (keeps the executable bits)."""
    tar = subprocess.run([GIT, "-C", str(ROOT), "archive", "--format=tar", "HEAD"], check=True,
                         capture_output=True).stdout
    files = []
    with tarfile.open(fileobj=io.BytesIO(tar)) as t:
        for m in t.getmembers():
            if m.isfile():
                files.append((m.name, m.mode, t.extractfile(m).read()))
    return files


def tcltk(stage: Path) -> list:
    """(path, mode, bytes) of Tcl/Tk for python/: _tkinter.pyd and its DLLs, Lib/tkinter and tcl/ (tcltk.msi)."""
    fetch(*TCLTK)
    out = (stage / "tcltk").resolve()
    shutil.rmtree(out, ignore_errors=True)
    msi = (DL / TCLTK[0].rsplit("/", 1)[1]).resolve()
    # msiexec opens no path with forward slashes (error 1619), which MSYS2's Python gives
    win = lambda p: str(p).replace("/", "\\")
    subprocess.run(["msiexec", "/a", win(msi), "/qn", f"TARGETDIR={win(out)}"], check=True)
    files = [(f"python/{f.name}", 0o644, f.read_bytes()) for f in sorted((out / "DLLs").iterdir()) if f.is_file()]
    for top in ("Lib/tkinter", "tcl/tcl8", "tcl/tcl8.6", "tcl/tk8.6"):
        for f in sorted((out / top).rglob("*")):
            if f.is_file() and "__pycache__" not in f.parts and "demos" not in f.parts:
                files.append((f"python/{f.relative_to(out).as_posix()}", 0o644, f.read_bytes()))
    return files


def setup_exe(python: list, stage: Path) -> list:
    """The two ways into the setup, as WondersBeyondRecomp's package has them: "AgeOfCalamityRecomp Setup.exe" at the
    top (setup/launcher.c with the game icon, compiled here with clang and llvm-windres) and, for a PC whose Smart App
    Control refuses that unsigned program, python/Setup (if blocked).exe (pythonw.exe renamed, which keeps Python's
    signature; python313._pth lists setup/, whose sitecustomize.py opens the setup)."""
    work = stage / "launcher"
    work.mkdir(parents=True, exist_ok=True)
    shutil.copy2(ROOT / "assets" / "icon.ico", work / "icon.ico")
    (work / "launcher.rc").write_text('1 ICON "icon.ico"\n', encoding="ascii")
    windres = shutil.which("llvm-windres") or shutil.which("windres") or "llvm-windres"
    sh([windres, "launcher.rc", "-O", "coff", "-o", "launcher.res.o"], cwd=work)
    sh(["clang", "--target=x86_64-w64-mingw32", "-Os", "-s", "-mwindows", "-municode", "-fuse-ld=lld",
        ROOT / "setup" / "launcher.c", work / "launcher.res.o", "-o", work / "setup.exe"])
    by_name = {n: d for n, _, d in python}
    return [(f"{SETUP_EXE}.exe", 0o755, (work / "setup.exe").read_bytes()),
            (f"python/{SIGNED}.exe", 0o755, by_name["python/pythonw.exe"])]


def python_embed(wheel_dir: Path, stage: Path) -> list:
    """(path, mode, bytes) of python/: the embeddable zip, Tcl/Tk, our ._pth, and the wheels in Lib/site-packages."""
    files = []
    with zipfile.ZipFile(io.BytesIO(fetch(*PY_EMBED))) as z:
        for n in z.namelist():
            if not n.endswith("/") and not n.endswith("._pth"):
                files.append((f"python/{n}", 0o644, z.read(n)))
    files.append(("python/python313._pth", 0o644, PTH.encode()))
    files += tcltk(stage)
    wheels = sorted(wheel_dir.glob("*.whl"))
    names = {w.name.split("-")[0].lower() for w in wheels}
    for need in WHEELS:
        if need not in names:
            sys.exit(f"ERROR: the {need} wheel is missing in {wheel_dir}")
    for w in wheels:
        with zipfile.ZipFile(w) as z:
            for n in z.namelist():
                if not n.endswith("/"):
                    files.append((f"python/Lib/site-packages/{n}", 0o644, z.read(n)))
    return files


def write_zip(path: Path, files: list) -> None:
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for name, mode, data in files:
            zi = zipfile.ZipInfo(f"{TOP}/{name}", date_time=time.localtime()[:6])
            zi.external_attr = (0o100000 | mode) << 16
            zi.compress_type = zipfile.ZIP_DEFLATED
            z.writestr(zi, data)


def zip_bytes(files: list) -> bytes:
    """A zip of (path, mode, bytes) as it is, without the top folder: installer/sources.zip."""
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for name, mode, data in sorted(files):
            zi = zipfile.ZipInfo(name, date_time=time.localtime()[:6])
            zi.external_attr = (0o100000 | mode) << 16
            zi.compress_type = zipfile.ZIP_DEFLATED
            z.writestr(zi, data)
    return buf.getvalue()


def download_wheels(stage: Path) -> Path:
    """The Windows wheels, with this Python's pip."""
    wheels = stage / "wheels"
    shutil.rmtree(wheels, ignore_errors=True)
    sh([sys.executable, "-m", "pip", "download", "-q", "--only-binary=:all:", "--platform", "win_amd64",
        "--python-version", "3.13", "--implementation", "cp", "-d", wheels, *WHEELS])
    return wheels


def windows_package(tree: list, prebuilt: list, wheels: Path, stage: Path) -> list:
    """The installer: python/, the two setup exes, installer/ with sources.zip (tree + prebuilt), the licences."""
    by_name = {n: d for n, _, d in tree}
    python = python_embed(wheels, stage)
    files = python + setup_exe(python, stage)
    files += [(dst, 0o644, by_name[src]) for src, dst in INSTALLER]
    files.append(("installer/sources.zip", 0o644, zip_bytes(tree + prebuilt)))
    return files


def write_tgz(path: Path, files: list) -> None:
    now = int(time.time())
    with tarfile.open(path, "w:gz", compresslevel=9) as t:
        for name, mode, data in files:
            ti = tarfile.TarInfo(f"{TOP}/{name}")
            ti.size, ti.mode, ti.mtime = len(data), mode, now
            t.addfile(ti, io.BytesIO(data))


def read(p: Path, mode: int = 0o644) -> tuple:
    return p.read_bytes(), mode


def install_local() -> int:
    """AgeOfCalamityRecomp Setup.exe and python/ in this folder, as the Windows package has them, so a copy of the repository
    starts the setup the same way (both are ignored by git)."""
    stage = OUT / "stage"
    python = python_embed(download_wheels(stage), stage)
    for name, _, data in python + setup_exe(python, stage):
        dst = ROOT / name
        dst.parent.mkdir(parents=True, exist_ok=True)
        dst.write_bytes(data)
    print(f"{SETUP_EXE}.exe and python/ are in {ROOT}")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--wsl-distro", default="archlinux", help="the development WSL distro")
    ap.add_argument("--win-xlate", type=Path, default=Path("C:/AgeOfCalamityRecomp-win/host"),
                    help="folder with xlate.exe and xlate.id for Windows")
    ap.add_argument("--allow-dirty", action="store_true", help="package HEAD even with uncommitted changes")
    ap.add_argument("--local", action="store_true",
                    help="only put AgeOfCalamityRecomp Setup.exe and python/ into this folder (Windows, any Python with pip)")
    ap.add_argument("--windows-only", action="store_true",
                    help="no WSL: only the Windows zip, without the Linux translator (any Python with pip)")
    args = ap.parse_args()
    if args.local:
        return install_local()
    version = (ROOT / "VERSION").read_text().strip()
    dirty = subprocess.run([GIT, "-C", str(ROOT), "status", "--porcelain", "--untracked-files=no"],
                           capture_output=True, text=True).stdout.strip()
    if dirty and not args.allow_dirty:
        sys.exit(f"ERROR: there are uncommitted changes (the package takes HEAD):\n{dirty}")
    OUT.mkdir(parents=True, exist_ok=True)
    stage = OUT / "stage"
    stage.mkdir(exist_ok=True)

    # The translators must match the sources at HEAD (their identity is a hash of them; tools/build_xlate.py).
    sys.path.insert(0, str(ROOT / "tools"))
    import build_xlate
    ident = build_xlate.identity()
    print(f"translator identity: {ident[:16]}")

    if args.windows_only:
        wheels = download_wheels(stage)
    else:
        # Linux parts, made in WSL: hactool, the Linux translator, and the Windows wheels (pip download)
        s = wsl_path(stage)
        hsrc = wsl_path(ROOT / "toolchain/hactool-1.4.0/hactool-1.4.0")
        # On Linux build_xlate.py's exe "xlate" lands inside its project folder "xlate/": take it from publish/
        wsl(args.wsl_distro, f"cd '{wsl_path(ROOT)}' && "
                             f"bash tools/build_hactool_linux.sh '{hsrc}' '{s}/hactool-linux' && "
                             f"mkdir -p '{s}/xlate-linux' && cp ~/AgeOfCalamityRecomp-build/xlate/publish/xlate ~/AgeOfCalamityRecomp-build/xlate/xlate.id "
                             f"'{s}/xlate-linux/' && rm -rf '{s}/wheels' && ~/.venvs/AgeOfCalamityRecomp/bin/python -m pip download -q "
                             f"--only-binary=:all: --platform win_amd64 --python-version 3.13 --implementation cp "
                             f"-d '{s}/wheels' {' '.join(WHEELS)}")
        wheels = stage / "wheels"
    ids = [args.win_xlate / "xlate.id"] + ([] if args.windows_only else [stage / "xlate-linux/xlate.id"])
    for idf in ids:
        got = idf.read_text().strip()
        if got != ident:
            sys.exit(f"ERROR: {idf} is {got[:16]}, the translator sources at HEAD give {ident[:16]}; rebuild the translator")
    tree = git_tree()
    if not any(n == "recomplauncher.json" for n, _, _ in tree):   # RecompLauncher's file
        tree.append(("recomplauncher.json", 0o644, (ROOT / "recomplauncher.json").read_bytes()))
    print(f"repository at HEAD: {len(tree)} files, {sum(len(d) for _, _, d in tree) / 1e6:.1f} MB")
    lic = (ROOT / "toolchain/hactool-1.4.0/hactool-1.4.0/LICENSE").read_bytes()

    # ---- Windows: the installer, whose sources.zip carries the prebuilt tools
    prebuilt = [("prebuilt/hactool/win-x64/hactool.exe", 0o755,
                 (ROOT / "toolchain/hactool-1.4.0-win/hactool.exe").read_bytes()),
                ("prebuilt/hactool/LICENSE", 0o644, lic)]
    for f in ("xlate.exe", "xlate.id"):
        prebuilt.append((f"prebuilt/xlate/win-x64/{f}", 0o755, (args.win_xlate / f).read_bytes()))
    if not args.windows_only:
        for f in ("xlate", "xlate.id"):
            prebuilt.append((f"prebuilt/xlate/linux-x64/{f}", 0o755, (stage / "xlate-linux" / f).read_bytes()))
    win = windows_package(tree, prebuilt, wheels, stage)
    outs = [(OUT / f"AgeOfCalamityRecomp-{version}-Windows.zip", write_zip, win)]

    # ---- Linux: the sources as they are
    if not args.windows_only:
        lin = list(tree)
        lin.append(("prebuilt/hactool/linux-x64/hactool", 0o755, (stage / "hactool-linux/hactool").read_bytes()))
        lin.append(("prebuilt/hactool/LICENSE", 0o644, lic))
        for f in ("xlate", "xlate.id"):
            lin.append((f"prebuilt/xlate/linux-x64/{f}", 0o755, (stage / "xlate-linux" / f).read_bytes()))
        outs.append((OUT / f"AgeOfCalamityRecomp-{version}-Linux.tar.gz", write_tgz, lin))
    for path, writer, files in outs:
        writer(path, files)
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        path.with_name(path.name + ".sha256").write_text(f"{digest}  {path.name}\n")
        print(f"{path.name}: {path.stat().st_size / 1e6:.1f} MB, {len(files)} files, sha256 {digest}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

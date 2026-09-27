#!/usr/bin/env python3
"""Build and audit a game-data-free PortMaster test ZIP."""
from __future__ import annotations

import hashlib
import json
import shutil
import stat
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PACKAGE = ROOT / "package"
BUILD = ROOT / "build"
STAGE = BUILD / "package-stage"
OUTPUT = ROOT / "dist" / "popclassic.zip"


def copy_file(source: Path, target: Path) -> None:
    if not source.is_file():
        raise SystemExit(f"Missing package input: {source}")
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, target)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> None:
    binary = BUILD / "popclassic"
    libs = BUILD / "libs.armhf"
    if not binary.is_file() or not (libs / "MANIFEST.txt").is_file():
        raise SystemExit("Build the ARM loader and run `make libs` before packaging.")
    if STAGE.exists():
        shutil.rmtree(STAGE)
    game = STAGE / "popclassic"
    game.mkdir(parents=True)
    copy_file(PACKAGE / "Prince of Persia Classic.sh", STAGE / "Prince of Persia Classic.sh")
    for name in ("port.json", "README.md", "gameinfo.xml", "testing_thread.txt", "cover.png", "screenshot.png"):
        copy_file(PACKAGE / name, game / name)
    copy_file(binary, game / "pop.armhf")
    copy_file(ROOT / "tools" / "eapx.py", game / "eapx.py")
    copy_file(PACKAGE / "popclassic" / "popclassic.eapx.json", game / "popclassic.eapx.json")
    copy_file(PACKAGE / "popclassic" / "popclassic.ini", game / "popclassic.ini")
    copy_file(game / "README.md", game / "gamedata" / "PUT_GAME_FILES_HERE.txt")
    shutil.copytree(PACKAGE / "popclassic" / "licenses", game / "licenses")
    shutil.copytree(libs, game / "libs.armhf", ignore=shutil.ignore_patterns("licenses"))
    for license_file in sorted((libs / "licenses").glob("*.copyright")):
        copy_file(license_file, game / "licenses" / "libraries" / license_file.name)

    descriptor = json.loads((game / "port.json").read_text(encoding="utf-8"))
    if descriptor["version"] != 4 or descriptor["name"] != OUTPUT.name or descriptor["items"] != ["Prince of Persia Classic.sh", "popclassic"]:
        raise SystemExit("PortMaster descriptor does not match the staged ZIP.")
    if descriptor.get("attr", {}).get("image") != {} or not (game / "cover.png").is_file():
        raise SystemExit("PortMaster cover metadata or cover.png is missing.")
    launcher = (STAGE / "Prince of Persia Classic.sh").read_bytes()
    if b"\r" in launcher or launcher.splitlines()[1] != b"# PORTMASTER: popclassic.zip, Prince of Persia Classic.sh":
        raise SystemExit("Launcher signature or line endings are invalid.")
    forbidden = {".apk", ".obb", ".dat", ".mp3", ".m4a", ".mp4"}
    for path in STAGE.rglob("*"):
        if path.is_file() and (path.suffix.lower() in forbidden or path.name in {"libgame_logic.so", "libcocos2d.so", "libcocosdenshion.so"}):
            raise SystemExit(f"Refusing to package game data: {path}")

    OUTPUT.parent.mkdir(exist_ok=True)
    with zipfile.ZipFile(OUTPUT, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for path in sorted(STAGE.rglob("*")):
            if not path.is_file():
                continue
            relative = path.relative_to(STAGE).as_posix()
            info = zipfile.ZipInfo(relative)
            info.create_system = 3
            mode = 0o755 if relative in {"Prince of Persia Classic.sh", "popclassic/pop.armhf", "popclassic/eapx.py"} else 0o644
            info.external_attr = (stat.S_IFREG | mode) << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            with path.open("rb") as stream, archive.open(info, "w") as target:
                shutil.copyfileobj(stream, target)
    with zipfile.ZipFile(OUTPUT) as archive:
        if archive.testzip() is not None:
            raise SystemExit("ZIP integrity check failed")
        roots = {name.split("/", 1)[0] for name in archive.namelist()}
        if roots != {"Prince of Persia Classic.sh", "popclassic"}:
            raise SystemExit(f"Unexpected ZIP root layout: {sorted(roots)}")
        packaged_hash = hashlib.sha256(archive.read("popclassic/pop.armhf")).hexdigest()
        if packaged_hash != sha256(binary):
            raise SystemExit("Packaged binary differs from the build output")
    print(f"Built {OUTPUT} ({OUTPUT.stat().st_size:,} bytes, data-free)")


if __name__ == "__main__":
    main()

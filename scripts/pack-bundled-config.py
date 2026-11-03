#!/usr/bin/env python3
"""Bundle Jot's local HTML configuration into a Pebble watchapp PBW.

The 2026 Pebble SDK does not include configPage in watchapp PBWs itself.
Run after pebble build; preserve every watch binary and resource.
"""
import json
import os
from pathlib import Path
import tempfile
import zipfile
import sys

ROOT = Path(__file__).resolve().parent.parent


def locate_pbw(build_dir):
    """Find the single PBW produced by Pebble; names vary by SDK/build."""
    build_dir = Path(build_dir)
    files = sorted(p for p in build_dir.glob("*.pbw") if p.is_file())
    if not files:
        raise ValueError("No .pbw found in " + str(build_dir) +
                         "; run 'pebble build' first")
    if len(files) != 1:
        raise ValueError("Multiple .pbw files in " + str(build_dir) +
                         "; pass the desired .pbw path explicitly")
    return files[0]


def package(pbw, html):
    pbw, html = Path(pbw), Path(html)
    if not pbw.is_file():
        raise ValueError("Run 'pebble build' first: missing " + str(pbw))
    if not html.is_file() or html.name != "config.html":
        raise ValueError("Bundled config.html is missing")
    with zipfile.ZipFile(pbw, "r") as source:
        manifest = json.loads(source.read("appinfo.json"))
        manifest["manifestVersion"] = 2
        manifest["configPage"] = "config.html"
        entries = [(item, source.read(item.filename))
                   for item in source.infolist()
                   if item.filename not in ("appinfo.json", "config.html")]
    fd, temp = tempfile.mkstemp(dir=str(pbw.parent), suffix=".pbw.tmp")
    try:
        with os.fdopen(fd, "wb") as raw:
            with zipfile.ZipFile(raw, "w", zipfile.ZIP_DEFLATED) as archive:
                archive.writestr("appinfo.json", json.dumps(manifest))
                for item, content in entries:
                    archive.writestr(item, content)
                archive.write(html, "config.html")
        with zipfile.ZipFile(temp) as check:
            assert check.testzip() is None, "broken PBW"
            assert json.loads(check.read("appinfo.json"))["configPage"] == "config.html"
            assert check.read("config.html") == html.read_bytes()
        os.replace(temp, pbw)
    finally:
        if os.path.exists(temp):
            os.unlink(temp)


def self_test():
    with tempfile.TemporaryDirectory() as directory:
        d = Path(directory)
        pbw, page = d / "test.pbw", d / "config.html"
        page.write_text("<html>Jot</html>", encoding="utf-8")
        with zipfile.ZipFile(pbw, "w") as z:
            z.writestr("appinfo.json", '{"uuid":"test","versionLabel":"1.19.7"}')
            z.writestr("pebble-app.bin", b"\x00\x01")
        package(pbw, page)
        package(pbw, page)
        with zipfile.ZipFile(pbw) as z:
            assert z.namelist().count("config.html") == 1
            assert z.read("pebble-app.bin") == b"\x00\x01"
            assert json.loads(z.read("appinfo.json"))["manifestVersion"] == 2
    with tempfile.TemporaryDirectory() as directory:
        build = Path(directory)
        try:
            locate_pbw(build)
            raise AssertionError("missing .pbw must fail")
        except ValueError as error:
            assert "No .pbw" in str(error)
        a = build / "jot.pbw"
        a.write_bytes(b"test")
        assert locate_pbw(build) == a
        b = build / "pebble-jot.pbw"
        b.write_bytes(b"test")
        try:
            locate_pbw(build)
            raise AssertionError("ambiguous .pbw must fail")
        except ValueError as error:
            assert "Multiple .pbw" in str(error)
    print("Bundled config PBW packaging and idempotence passed")


if __name__ == "__main__":
    if sys.argv[1:] == ["--self-test"]:
        self_test()
    elif len(sys.argv) <= 2:
        project = json.loads((ROOT / "package.json").read_text())
        pbw = Path(sys.argv[1]) if len(sys.argv) == 2 else locate_pbw(ROOT / "build")
        page = ROOT / project["pebble"]["configPage"]
        package(pbw, page)
        print("Bundled " + str(page) + " into " + str(pbw))
    else:
        sys.exit("usage: python3 scripts/pack-bundled-config.py [--self-test | path/to/jot.pbw]")

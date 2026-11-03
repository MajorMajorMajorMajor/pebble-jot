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
    print("Bundled config PBW packaging and idempotence passed")


if __name__ == "__main__":
    if sys.argv[1:] == ["--self-test"]:
        self_test()
    elif len(sys.argv) == 1:
        project = json.loads((ROOT / "package.json").read_text())
        pbw = ROOT / "build" / (project["name"] + ".pbw")
        page = ROOT / project["pebble"]["configPage"]
        package(pbw, page)
        print("Bundled " + str(page) + " into " + str(pbw))
    else:
        sys.exit("usage: python3 scripts/pack-bundled-config.py [--self-test]")

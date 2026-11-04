#!/bin/sh
# Jot development builds must include config.html in the installed PBW.
# Plain 'pebble build' produces a watch binary but not the bundled phone UI.
set -eu
cd "$(dirname "$0")/.."

if [ "${1:-}" = "--clean" ]; then
  pebble clean
  shift
fi
if [ "${1:-}" = "--build-only" ]; then
  build_only=1
  shift
else
  build_only=0
fi
if [ "$#" -ne 0 ]; then
  echo "usage: sh scripts/build-jot.sh [--clean] [--build-only]" >&2
  exit 2
fi

python3 scripts/sync-settings-fallback.py
pebble build
python3 scripts/pack-bundled-config.py
python3 - <<'PY'
import json
from pathlib import Path
import sys
import zipfile

root = Path("build")
paths = list(root.glob("*.pbw"))
if len(paths) != 1:
    sys.exit("Expected a single packaged PBW under build/")
with zipfile.ZipFile(paths[0]) as pbw:
    info = json.loads(pbw.read("appinfo.json"))
    if info.get("configPage") != "config.html" or info.get("manifestVersion") != 2:
        sys.exit("PBW does not advertise the bundled Jot Settings page")
    if pbw.read("config.html") != Path("config.html").read_bytes():
        sys.exit("The bundled Settings page does not match config.html")
print("Verified: bundled Jot Settings page is inside", paths[0])
PY
if [ "$build_only" -eq 0 ]; then
  pebble install --cloudpebble
fi

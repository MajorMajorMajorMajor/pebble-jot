#!/usr/bin/env python3
"""Detect message keys absent from a cached Pebble SDK-generated C header.

This protects incremental builds after switching to branches with new
package.json message keys. Run --self-test for independent behavior checks.
"""
import json
from pathlib import Path
import re
import sys
from tempfile import TemporaryDirectory

ROOT = Path(__file__).resolve().parent.parent


def missing_keys(manifest: Path, header: Path):
    declared = json.loads(manifest.read_text(encoding="utf-8"))["pebble"]["messageKeys"]
    generated = set(re.findall(
        r"\\bMESSAGE_KEY_([A-Za-z0-9_]+)\\b",
        header.read_text(encoding="utf-8"),
    ))
    return [name for name in declared if name not in generated]


def self_test():
    with TemporaryDirectory() as temp:
        manifest = Path(temp) / "package.json"
        header = Path(temp) / "message_keys.auto.h"
        manifest.write_text(json.dumps({
            "pebble": {"messageKeys": ["BackupAck", "ImportKind", "ImportTokenHi"]}
        }), encoding="utf-8")
        header.write_text(
            "enum { MESSAGE_KEY_BackupAck = 1, MESSAGE_KEY_ImportKind = 2, "
            "MESSAGE_KEY_ImportTokenHi = 3 };",
            encoding="utf-8",
        )
        assert missing_keys(manifest, header) == []
        header.write_text(
            "enum { MESSAGE_KEY_BackupAck = 1 };",
            encoding="utf-8",
        )
        assert missing_keys(manifest, header) == ["ImportKind", "ImportTokenHi"]
    print("Cached message-key detection tests passed")


def main():
    if sys.argv[1:] == ["--self-test"]:
        self_test()
        return 0
    if len(sys.argv) > 1:
        sys.exit("usage: python3 scripts/check-generated-message-keys.py [--self-test]")

    header = ROOT / "build/include/message_keys.auto.h"
    if not header.exists():
        return 0  # A fresh SDK build will generate the header.
    missing = missing_keys(ROOT / "package.json", header)
    if missing:
        print("Stale SDK-generated message key header; missing: " +
              ", ".join(missing), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

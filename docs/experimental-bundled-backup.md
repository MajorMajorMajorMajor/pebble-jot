# Jot #39 self-contained bundled phone viewer

Branch: feature/issue-39-bundled-backup-viewer
Status: hardware-tested feature for Pebble mobile 1.16.0.5;
Pebble's bundled config-page API is still in preview, so not yet
approved for public appstore release.

## Goal

Make the Settings viewer self-contained and durable, independent of a
remote page that could disappear. The existing watch-to-phone backup
transfer, atomic persistence, and snapshot key remain unchanged.

The original viewer was on GitHub Pages and moved all notes in a Base64
URL fragment. Besides depending on hosting, a full notebook could
produce an extremely long URL. This experiment bundles config.html
inside the PBW and requests small Unicode-safe note chunks directly
from the PebbleKit JS localStorage snapshot via configmessage.

No note text is sent in a URL. The phone companion no longer has a
GitHub Pages URL or URL-fragment backup transport: the hosted fallback
has deliberately been removed. The bundled configuration page requires
Pebble mobile 1.16.0.5 with the Plugins/permissions preview enabled.
Older companion versions are not supported by this feature branch.

## Build and install on Emery

Requires Pebble Tool 5.1.0+ and the project's pinned SDK toolchain.

    git fetch origin
    git switch feature/issue-39-bundled-backup-viewer
    pebble build
    python3 scripts/pack-bundled-config.py
    pebble install --cloudpebble

IMPORTANT: the present SDK's normal pebble build does not bundle
config.html. Always run the local Python packer after building and
before installing this branch. The packer selects the only .pbw in build/
(e.g. build/jot.pbw); it no longer assumes a package-name-based filename.
If you have multiple PBWs, specify the exact one as an argument:

    python3 scripts/pack-bundled-config.py build/jot.pbw

The packer sets the preview appinfo.json manifestVersion to 2 and inserts
the bundled page while preserving the watch binary and resources.

On the Android Pebble app 1.16.0.5, enable the Plugins/permissions
preview at Settings > Plugins before opening Jot Settings.

## Wrist + phone QA

1. With a few notes on your watch, use Send To Phone until it reports
   "Phone copy saved". Open Jot Settings and verify all notes appear.
2. Copy notes and save .txt and .json. Verify Unicode characters and
   long lines in the actual files/clipboard, not just on the screen.
3. Disable internet or temporarily make the GitHub Pages viewer
   unavailable and reopen Settings. The bundled page must still open
   without trying to fetch that website.
4. Test no backup, a completed empty backup and a very large backup.
   Check beginning/end content and ensure no partial export succeeds.
5. Restart the phone companion and repeat. Compare the existing
   localStorage snapshot; the viewer must never erase or overwrite it.

Do not equate a "Phone copy saved" watch receipt with an independently
retrievable backup until export is confirmed on the phone. Preserve an
independent file copy of real notes before QA.

No #51 merge semantics, #70 phone editing/sync, or watch-storage change
is introduced here. The Plugin Preview API is unstable and is not yet
approved for public appstore builds.

SDK reference: https://developer.repebble.com/guides/plugins/preview/

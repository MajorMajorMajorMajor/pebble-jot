#!/usr/bin/env python3
"""Validate all bitmap resources before Pebble SDK's PNG converter sees them.

No third-party modules: checks PNG chunk CRCs, IDAT decompression, and per-row
filter bytes. A file with a valid PNG signature but corrupted image data must
fail CI (the SDK otherwise fails only at on-watch build time).
"""
import json
import struct
import zlib
from pathlib import Path

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
COLOR_CHANNELS = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}


def verify_png(path):
    content = path.read_bytes()
    assert content.startswith(PNG_SIGNATURE), f"{path}: invalid PNG signature"
    offset = len(PNG_SIGNATURE)
    info = None
    compressed = bytearray()
    ended = False

    while offset < len(content):
        assert offset + 12 <= len(content), f"{path}: truncated PNG chunk"
        length = struct.unpack_from(">I", content, offset)[0]
        kind = content[offset + 4:offset + 8]
        data_start = offset + 8
        data_end = data_start + length
        assert data_end + 4 <= len(content), f"{path}: truncated {kind!r}"
        chunk = content[data_start:data_end]
        crc = struct.unpack_from(">I", content, data_end)[0]
        actual = zlib.crc32(kind + chunk) & 0xffffffff
        assert crc == actual, f"{path}: {kind!r} CRC mismatch"

        if kind == b"IHDR":
            assert len(chunk) == 13, f"{path}: invalid IHDR"
            info = struct.unpack(">IIBBBBB", chunk)
        elif kind == b"IDAT":
            compressed.extend(chunk)
        elif kind == b"IEND":
            assert not chunk, f"{path}: invalid IEND"
            ended = True
            offset = data_end + 4
            break
        offset = data_end + 4

    assert ended and offset == len(content), f"{path}: invalid PNG end"
    assert info is not None and compressed, f"{path}: missing pixel data"
    width, height, depth, color, compression, filtering, interlace = info
    assert width > 0 and height > 0, f"{path}: empty image"
    assert color in COLOR_CHANNELS, f"{path}: unsupported color type"
    assert depth in (1, 2, 4, 8, 16), f"{path}: invalid color depth"
    assert compression == 0 and filtering == 0 and interlace == 0, (
        f"{path}: unsupported PNG format"
    )
    pixels = zlib.decompress(compressed)
    rowbytes = (width * COLOR_CHANNELS[color] * depth + 7) // 8
    expected = (rowbytes + 1) * height
    assert len(pixels) == expected, (
        f"{path}: decompressed {len(pixels)} bytes, expected {expected}"
    )
    for y in range(height):
        filter_code = pixels[y * (rowbytes + 1)]
        assert filter_code <= 4, (
            f"{path}: invalid PNG filter {filter_code} on row {y}"
        )


def main():
    package = json.loads(Path("package.json").read_text())
    resources = package["pebble"]["resources"]["media"]
    images = [
        Path("resources") / item["file"]
        for item in resources if item["type"] == "bitmap"
    ]
    assert images, "No PNG bitmap resources found"
    for image in images:
        verify_png(image)
        print(f"Valid PNG: {image}")
    print(f"Validated {len(images)} Pebble bitmap assets")


if __name__ == "__main__":
    main()

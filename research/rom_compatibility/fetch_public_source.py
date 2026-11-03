#!/usr/bin/env python3
"""Fetch pinned PUBLIC source to an external cache; never fetch ROMs or saves."""
import argparse
import hashlib
import json
from pathlib import Path
from urllib.request import urlopen

MANIFEST = Path(__file__).with_name('source_manifest.json')


def validate(data, expected):
    actual = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
    if actual != expected:
        raise ValueError('public source blob identity mismatch')


def read_sources(root):
    spec = json.loads(MANIFEST.read_text())
    result = {}
    for name, digest in spec['files'].items():
        raw = (Path(root) / name).read_bytes()
        validate(raw, digest)
        # Upstream has a few legacy non-UTF-8 comments. Identity checks use raw bytes.
        result[name] = raw.decode('utf-8', errors='replace')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cache', type=Path, help='external public-source cache, e.g. /tmp/rogue-source')
    args = parser.parse_args()
    spec = json.loads(MANIFEST.read_text())
    for name, digest in spec['files'].items():
        target = args.cache / name
        if target.exists():
            validate(target.read_bytes(), digest)
            continue
        url = f"https://raw.githubusercontent.com/{spec['repository']}/{spec['ref']}/{name}"
        with urlopen(url, timeout=45) as response:
            raw = response.read()
        validate(raw, digest)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(raw)
    read_sources(args.cache)
    print(f"Verified {len(spec['files'])} public source blobs at {spec['ref']}")


if __name__ == '__main__':
    main()

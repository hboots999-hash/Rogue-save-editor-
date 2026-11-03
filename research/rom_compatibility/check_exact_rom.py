#!/usr/bin/env python3
"""Recheck prior PR14 ROM measurements locally; output metadata only, no tables.

Requires a legally held raw GBA dump. Does not read saves or accept a CIA,
write ROM bytes, authorize editing, or infer table symbol names/reachability.
"""
import argparse
import hashlib
import json
from pathlib import Path

ROM_SIZE = 32 * 1024 * 1024
ROM_SHA256 = '111b0008bcec519c59a02de57895a924fee5d3633c8b8fbb394a497153778ca3'
RECORDS = 936
TABLES = {
    'names': (0x4C283C, 13, 'e91817b5658420469ccce73e6e2b59075339ac8a326edea3b6d2136e8fe768d0'),
    'A': (0xBD27D0, 28, 'b68bb9f44e18a4461ba0e230724c0947294559ffe3d675e9618c6918b91cb8ca'),
    'B': (0xBD8E30, 28, '323c8d3dfa069066a5ddf121e2e6ba36ab70c0bbdecd2dcb9aa96419e5c5c071'),
}


def region_digest(data, offset, stride, count):
    if offset < 0 or stride <= 0 or count <= 0 or offset + stride * count > len(data):
        raise ValueError('invalid/truncated region geometry')
    return hashlib.sha256(memoryview(data)[offset:offset + stride * count]).hexdigest()


def verify_image(data):
    if len(data) != ROM_SIZE:
        raise ValueError('expected raw 32 MiB GBA ROM; CIA/container input unsupported')
    if hashlib.sha256(data).hexdigest() != ROM_SHA256:
        raise ValueError('whole-ROM SHA-256 mismatch; refusing all address interpretation')
    for name, (offset, stride, digest) in TABLES.items():
        if region_digest(data, offset, stride, RECORDS) != digest:
            raise ValueError(f'{name} region SHA-256 mismatch')
    a, b = TABLES['A'][0], TABLES['B'][0]
    disagreements = sum(data[a + i * 28 + 5] != data[b + i * 28 + 5] for i in range(1, 849))
    if disagreements != 40:
        raise ValueError('PP disagreement count differs from prior ROM measurement')
    if data[a + 58 * 28 + 5] != 10 or data[b + 58 * 28 + 5] != 15:
        raise ValueError('Ice Beam sample differs from prior ROM measurement')
    # Recheck prior constant-false selector claim without returning executable bytes.
    if data[0x1D11B8:0x1D11BC] != bytes.fromhex('00207047'):
        raise ValueError('prior selector stub measurement mismatch')
    return {
        'rom_sha256': ROM_SHA256,
        'prior_region_hashes_match': True,
        'ordinary_pp_disagreements_including_struggle_range': disagreements,
        'selector_0x081d11b8_matches_prior_constant_false_stub': True,
        'table_A_offset': '0x00bd27d0', 'table_B_offset': '0x00bd8e30',
        'table_symbol_assignment': 'UNVERIFIED',
        'all_reachable_PP_consumers_traced': False,
        'nature_and_ability_compiled_semantics_verified': False,
        'gameplay_load_resave_verified': False,
        'editing_authorized_by_this_report': False,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path)
    args = parser.parse_args()
    if args.rom.stat().st_size != ROM_SIZE:
        parser.error('expected raw 32 MiB GBA ROM')
    try:
        report = verify_image(args.rom.read_bytes())
    except ValueError as error:
        parser.error(str(error))
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()

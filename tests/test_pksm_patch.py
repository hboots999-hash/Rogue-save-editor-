#!/usr/bin/env python3
"""Host regression test for conservative PKSM overlay application."""
from pathlib import Path
from tempfile import TemporaryDirectory
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from apply_to_pksm import patch_tree


def main():
    with TemporaryDirectory() as d:
        root = Path(d)
        title = root / '3ds/source/gui/screen/TitleLoadScreen.cpp'
        title.parent.mkdir(parents=True)
        title.write_text('#include "TitleLoadScreen.hpp"\nvoid update(){\n    u32 buttonsDown = hidKeysDown();\n}\n')
        mk = root / 'Makefile'
        mk.write_text('export PKSM_TITLE\t\t:= \tPKSM\n')
        ds = root / '3ds/Makefile'
        ds.write_text('''UNIQUE_ID\t\t:=\t0xEC100
PRODUCT_CODE\t:=\tCTR-HB-PKSM
source/gui/screen \\
include/gui/screen \\
''')
        patch_tree(root)
        t = title.read_text()
        assert 'KEY_SELECT' in t and 'RogueInspectScreen' in t
        assert '0xF73A2' in ds.read_text()
        assert 'source/rogue' in ds.read_text() and 'include/rogue' in ds.read_text()
        assert (root / '3ds/source/gui/screen/RogueInspectScreen.cpp').exists()
        assert (root / '3ds/source/rogue/rogue_bridge.cpp').exists()
        assert (root / '3ds/include/rogue/sector.hpp').exists()
        try:
            patch_tree(root)
        except ValueError:
            pass
        else:
            raise AssertionError('Second patch unexpectedly succeeded')
        print('PASS: PKSM overlay integration and duplicate-patch rejection')
    with TemporaryDirectory() as d:
        root = Path(d)
        title = root / '3ds/source/gui/screen/TitleLoadScreen.cpp'
        title.parent.mkdir(parents=True)
        title.write_text('#include "TitleLoadScreen.hpp"\nvoid update(){}\n')
        (root/'Makefile').write_text('export PKSM_TITLE\t\t:= \tPKSM\n')
        (root/'3ds/Makefile').write_text('UNIQUE_ID\t\t:=\t0xEC100\n')
        original = title.read_bytes()
        try:
            patch_tree(root)
        except ValueError:
            pass
        else:
            raise AssertionError('Changed upstream unexpectedly accepted')
        assert title.read_bytes() == original
        print('PASS: unexpected upstream format fails before edits')


if __name__ == '__main__':
    main()

"""Infrastructure tests use fabricated bytes; no private files or ROM assertions."""
import hashlib
import unittest
from unittest.mock import patch
from check_exact_rom import ROM_SIZE, region_digest, verify_image
from fetch_public_source import validate


class AuditToolsTests(unittest.TestCase):
    def test_public_blob_identity(self):
        content = b'public synthetic fixture\n'
        digest = hashlib.sha1(b'blob 25\0' + content).hexdigest()
        validate(content, digest)
        with self.assertRaises(ValueError):
            validate(content + b'changed', digest)

    def test_region_boundaries(self):
        content = bytes(range(64))
        self.assertEqual(region_digest(content, 8, 4, 14), hashlib.sha256(content[8:64]).hexdigest())
        for offset, stride, count in [(-1, 4, 1), (0, 0, 2), (0, 4, 0), (8, 4, 15)]:
            with self.subTest(geometry=(offset, stride, count)), self.assertRaises(ValueError):
                region_digest(content, offset, stride, count)

    def test_wrong_geometry_refused_before_table_interpretation(self):
        with patch('check_exact_rom.region_digest', side_effect=AssertionError('must not read tables')):
            for content in [b'', b'CIA', bytes(1024)]:
                with self.subTest(size=len(content)), self.assertRaisesRegex(ValueError, '32 MiB'):
                    verify_image(content)

    def test_forged_full_size_rom_refused_before_table_interpretation(self):
        # No patching of identity constants to manufacture a successful certification.
        with patch('check_exact_rom.region_digest', side_effect=AssertionError('must not read tables')):
            with self.assertRaisesRegex(ValueError, 'whole-ROM SHA-256 mismatch'):
                verify_image(bytes(ROM_SIZE))


if __name__ == '__main__':
    unittest.main()

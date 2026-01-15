#!/usr/bin/env python3
"""
Tests for the extract_minidump.py script.

Uses the minidump.envelope fixture which contains the minidump.dmp from
tests/fixtures to verify that extraction produces identical output.
"""

import hashlib
import os
import sys
import tempfile
import unittest
from pathlib import Path

# Add the scripts directory to the path so we can import extract_minidump
SCRIPT_DIR = Path(__file__).parent
sys.path.insert(0, str(SCRIPT_DIR))

from extract_minidump import parse_envelope, extract_minidump, list_envelope_contents


class TestExtractMinidump(unittest.TestCase):
    """Test cases for minidump extraction from Sentry envelopes."""

    @classmethod
    def setUpClass(cls):
        """Set up test fixtures paths."""
        cls.repo_root = SCRIPT_DIR.parent
        cls.fixtures_dir = cls.repo_root / "tests" / "fixtures"
        cls.envelope_path = cls.fixtures_dir / "minidump.envelope"
        cls.original_minidump_path = cls.fixtures_dir / "minidump.dmp"

        # Verify fixtures exist
        if not cls.envelope_path.exists():
            raise FileNotFoundError(
                f"Envelope fixture not found: {cls.envelope_path}\n"
                "Run create_envelope_fixture.py to create it."
            )
        if not cls.original_minidump_path.exists():
            raise FileNotFoundError(
                f"Original minidump not found: {cls.original_minidump_path}"
            )

    def test_parse_envelope_structure(self):
        """Test that envelope parsing returns correct structure."""
        with open(self.envelope_path, 'rb') as f:
            data = f.read()

        envelope_header, items = parse_envelope(data)

        # Check envelope header
        self.assertIn('dsn', envelope_header)
        self.assertIn('event_id', envelope_header)

        # Check we have at least 2 items (event + attachment)
        self.assertGreaterEqual(len(items), 2)

        # Check item types
        item_types = [item[0].get('type') for item in items]
        self.assertIn('event', item_types)
        self.assertIn('attachment', item_types)

    def test_parse_envelope_minidump_header(self):
        """Test that the minidump attachment header is correct."""
        with open(self.envelope_path, 'rb') as f:
            data = f.read()

        envelope_header, items = parse_envelope(data)

        # Find minidump item
        minidump_item = None
        for item_header, item_payload in items:
            if item_header.get('attachment_type') == 'event.minidump':
                minidump_item = (item_header, item_payload)
                break

        self.assertIsNotNone(minidump_item, "No minidump attachment found")

        header, payload = minidump_item
        self.assertEqual(header['type'], 'attachment')
        self.assertEqual(header['attachment_type'], 'event.minidump')
        self.assertIn('filename', header)
        self.assertEqual(header['length'], len(payload))

    def test_minidump_magic_bytes(self):
        """Test that extracted minidump has correct magic bytes."""
        with open(self.envelope_path, 'rb') as f:
            data = f.read()

        envelope_header, items = parse_envelope(data)

        # Find minidump payload
        minidump_payload = None
        for item_header, item_payload in items:
            if item_header.get('attachment_type') == 'event.minidump':
                minidump_payload = item_payload
                break

        self.assertIsNotNone(minidump_payload)
        # MDMP is the minidump magic signature
        self.assertEqual(minidump_payload[:4], b'MDMP',
                         "Minidump should start with MDMP magic bytes")

    def test_extract_minidump_matches_original(self):
        """Test that extracted minidump is identical to original."""
        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "extracted.dmp"

            # Extract the minidump
            result_path = extract_minidump(str(self.envelope_path), str(output_path))

            self.assertEqual(result_path, str(output_path))
            self.assertTrue(output_path.exists())

            # Compare with original
            with open(self.original_minidump_path, 'rb') as f:
                original_data = f.read()
            with open(output_path, 'rb') as f:
                extracted_data = f.read()

            # Compare sizes
            self.assertEqual(len(extracted_data), len(original_data),
                             f"Size mismatch: extracted={len(extracted_data)}, "
                             f"original={len(original_data)}")

            # Compare content
            self.assertEqual(extracted_data, original_data,
                             "Extracted minidump content differs from original")

    def test_extract_minidump_hash_comparison(self):
        """Test extraction using hash comparison for additional verification."""
        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "extracted.dmp"

            extract_minidump(str(self.envelope_path), str(output_path))

            # Calculate hashes
            with open(self.original_minidump_path, 'rb') as f:
                original_hash = hashlib.md5(f.read()).hexdigest()
            with open(output_path, 'rb') as f:
                extracted_hash = hashlib.md5(f.read()).hexdigest()

            self.assertEqual(extracted_hash, original_hash,
                             f"MD5 hash mismatch: extracted={extracted_hash}, "
                             f"original={original_hash}")

    def test_extract_minidump_default_filename(self):
        """Test that extraction uses filename from envelope when not specified."""
        with tempfile.TemporaryDirectory() as tmpdir:
            # Copy envelope to temp dir so output goes there
            import shutil
            temp_envelope = Path(tmpdir) / "test.envelope"
            shutil.copy(self.envelope_path, temp_envelope)

            # Extract without specifying output path
            result_path = extract_minidump(str(temp_envelope))

            # Should use filename from envelope header (minidump.dmp)
            self.assertTrue(Path(result_path).exists())
            self.assertEqual(Path(result_path).name, "minidump.dmp")

    def test_extract_minidump_nonexistent_file(self):
        """Test that extraction fails gracefully for nonexistent file."""
        with self.assertRaises(FileNotFoundError):
            extract_minidump("/nonexistent/path/to/envelope.envelope")

    def test_envelope_without_minidump(self):
        """Test that extraction fails gracefully when no minidump present."""
        with tempfile.TemporaryDirectory() as tmpdir:
            # Create envelope without minidump
            import json
            envelope_path = Path(tmpdir) / "no_minidump.envelope"

            envelope_header = {"dsn": "https://test@sentry.invalid/42"}
            event_payload = {"event_id": "test", "level": "info"}
            event_bytes = json.dumps(event_payload).encode('utf-8')
            event_header = {"type": "event", "length": len(event_bytes)}

            with open(envelope_path, 'wb') as f:
                f.write(json.dumps(envelope_header).encode('utf-8'))
                f.write(b'\n')
                f.write(json.dumps(event_header).encode('utf-8'))
                f.write(b'\n')
                f.write(event_bytes)

            with self.assertRaises(ValueError) as ctx:
                extract_minidump(str(envelope_path))

            self.assertIn("No minidump", str(ctx.exception))


class TestParseEnvelope(unittest.TestCase):
    """Test cases for envelope parsing edge cases."""

    def test_parse_empty_envelope(self):
        """Test parsing empty data."""
        with self.assertRaises(Exception):
            parse_envelope(b'')

    def test_parse_header_only(self):
        """Test parsing envelope with only header."""
        import json
        data = json.dumps({"dsn": "test"}).encode('utf-8') + b'\n'
        header, items = parse_envelope(data)
        self.assertEqual(header['dsn'], 'test')
        self.assertEqual(len(items), 0)

    def test_parse_multiple_items(self):
        """Test parsing envelope with multiple items."""
        import json

        envelope_header = {"dsn": "test"}
        item1_payload = b"payload1"
        item1_header = {"type": "event", "length": len(item1_payload)}
        item2_payload = b"payload2"
        item2_header = {"type": "attachment", "length": len(item2_payload)}

        data = b''
        data += json.dumps(envelope_header).encode('utf-8') + b'\n'
        data += json.dumps(item1_header).encode('utf-8') + b'\n'
        data += item1_payload
        data += b'\n'
        data += json.dumps(item2_header).encode('utf-8') + b'\n'
        data += item2_payload

        header, items = parse_envelope(data)

        self.assertEqual(len(items), 2)
        self.assertEqual(items[0][1], item1_payload)
        self.assertEqual(items[1][1], item2_payload)


if __name__ == '__main__':
    # Run tests with verbosity
    unittest.main(verbosity=2)

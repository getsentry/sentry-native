#!/usr/bin/env python3
"""
Extract minidump (.dmp) attachments from Sentry envelope files.

Sentry envelope format:
- Line 1: Envelope header (JSON)
- For each item:
  - Item header (JSON with "type", "length", and optional metadata)
  - Item payload (raw bytes of specified length)
  - Items are separated by newlines

Usage:
    python extract_minidump.py <envelope_file> [output_file]

If output_file is not specified, the filename from the envelope will be used.
"""

import argparse
import json
import os
import sys
from pathlib import Path


def parse_envelope(data: bytes) -> tuple[dict, list[tuple[dict, bytes]]]:
    """
    Parse a Sentry envelope and return the header and list of items.
    
    Args:
        data: Raw envelope file contents
        
    Returns:
        Tuple of (envelope_header, list of (item_header, item_payload) tuples)
    """
    pos = 0
    
    # Parse envelope header (first line)
    newline_pos = data.find(b'\n', pos)
    if newline_pos == -1:
        raise ValueError("Invalid envelope: missing newline after header")
    
    envelope_header = json.loads(data[pos:newline_pos].decode('utf-8'))
    pos = newline_pos + 1
    
    items = []
    
    # Parse items
    while pos < len(data):
        # Skip any extra newlines between items
        while pos < len(data) and data[pos:pos+1] == b'\n':
            pos += 1
        
        if pos >= len(data):
            break
            
        # Parse item header
        newline_pos = data.find(b'\n', pos)
        if newline_pos == -1:
            # No more complete items
            break
        
        item_header_bytes = data[pos:newline_pos]
        try:
            item_header = json.loads(item_header_bytes.decode('utf-8'))
        except json.JSONDecodeError as e:
            print(f"Warning: Failed to parse item header at position {pos}: {e}")
            break
            
        pos = newline_pos + 1
        
        # Get payload length
        payload_len = item_header.get('length')
        
        if payload_len is None:
            # Length omitted: read until next newline or end
            next_newline = data.find(b'\n', pos)
            if next_newline == -1:
                payload_len = len(data) - pos
            else:
                payload_len = next_newline - pos
        
        # Extract payload
        payload = data[pos:pos + payload_len]
        pos += payload_len
        
        items.append((item_header, payload))
    
    return envelope_header, items


def extract_minidump(envelope_path: str, output_path: str = None) -> str:
    """
    Extract the minidump attachment from a Sentry envelope file.
    
    Args:
        envelope_path: Path to the envelope file
        output_path: Optional output path for the minidump. If not specified,
                    uses the filename from the envelope metadata.
                    
    Returns:
        Path to the extracted minidump file
        
    Raises:
        FileNotFoundError: If envelope file doesn't exist
        ValueError: If no minidump found in envelope
    """
    envelope_path = Path(envelope_path)
    
    if not envelope_path.exists():
        raise FileNotFoundError(f"Envelope file not found: {envelope_path}")
    
    # Read envelope file
    with open(envelope_path, 'rb') as f:
        data = f.read()
    
    print(f"Read {len(data)} bytes from {envelope_path}")
    
    # Parse envelope
    envelope_header, items = parse_envelope(data)
    
    print(f"Envelope event_id: {envelope_header.get('event_id', 'N/A')}")
    print(f"Found {len(items)} item(s) in envelope")
    
    # Find minidump attachment
    minidump_item = None
    minidump_header = None
    
    for item_header, item_payload in items:
        item_type = item_header.get('type', '')
        attachment_type = item_header.get('attachment_type', '')
        
        print(f"  - Item type: {item_type}, attachment_type: {attachment_type}, "
              f"length: {len(item_payload)} bytes")
        
        if item_type == 'attachment' and attachment_type == 'event.minidump':
            minidump_item = item_payload
            minidump_header = item_header
            print(f"    -> Found minidump!")
    
    if minidump_item is None:
        raise ValueError("No minidump attachment found in envelope")
    
    # Determine output path
    if output_path is None:
        filename = minidump_header.get('filename', 'minidump.dmp')
        output_path = envelope_path.parent / filename
    else:
        output_path = Path(output_path)
    
    # Verify minidump magic bytes (optional sanity check)
    if minidump_item[:4] == b'MDMP':
        print(f"Minidump magic verified: MDMP")
    else:
        print(f"Warning: Unexpected magic bytes: {minidump_item[:4]}")
    
    # Write minidump
    with open(output_path, 'wb') as f:
        f.write(minidump_item)
    
    print(f"\nExtracted minidump to: {output_path}")
    print(f"Size: {len(minidump_item)} bytes")
    
    return str(output_path)


def list_envelope_contents(envelope_path: str) -> None:
    """
    List the contents of a Sentry envelope file without extracting.
    """
    envelope_path = Path(envelope_path)
    
    if not envelope_path.exists():
        raise FileNotFoundError(f"Envelope file not found: {envelope_path}")
    
    with open(envelope_path, 'rb') as f:
        data = f.read()
    
    envelope_header, items = parse_envelope(data)
    
    print(f"Envelope: {envelope_path}")
    print(f"  Size: {len(data)} bytes")
    print(f"  Event ID: {envelope_header.get('event_id', 'N/A')}")
    print(f"  DSN: {envelope_header.get('dsn', 'N/A')}")
    print()
    print(f"Items ({len(items)}):")
    
    for i, (item_header, item_payload) in enumerate(items):
        print(f"  [{i}] Type: {item_header.get('type', 'unknown')}")
        print(f"      Length: {len(item_payload)} bytes")
        
        if item_header.get('attachment_type'):
            print(f"      Attachment Type: {item_header['attachment_type']}")
        if item_header.get('filename'):
            print(f"      Filename: {item_header['filename']}")
        if item_header.get('content_type'):
            print(f"      Content-Type: {item_header['content_type']}")
        
        # Show preview for text items
        if item_header.get('type') in ('event', 'session', 'transaction'):
            try:
                preview = item_payload[:200].decode('utf-8')
                if len(item_payload) > 200:
                    preview += '...'
                print(f"      Preview: {preview}")
            except UnicodeDecodeError:
                pass
        
        print()


def main():
    parser = argparse.ArgumentParser(
        description='Extract minidump attachments from Sentry envelope files'
    )
    parser.add_argument(
        'envelope',
        help='Path to the Sentry envelope file'
    )
    parser.add_argument(
        'output',
        nargs='?',
        help='Output path for the minidump (default: use filename from envelope)'
    )
    parser.add_argument(
        '-l', '--list',
        action='store_true',
        help='List envelope contents without extracting'
    )
    
    args = parser.parse_args()
    
    try:
        if args.list:
            list_envelope_contents(args.envelope)
        else:
            extract_minidump(args.envelope, args.output)
    except (FileNotFoundError, ValueError) as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()

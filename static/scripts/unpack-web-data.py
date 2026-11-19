#!/usr/bin/env python3
"""Export an Emscripten LZ4 package using its matching JS loader's metadata.

Requires python3-lz4. Does not execute the loader. The source manifest describes
an older build; chunk offsets and file ranges must come from the matching JS.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re


def unpack(loader, package, destination):
    import lz4.block

    script = loader.read_text()
    compressed = re.search(r'var compressedData=(\{.*?\});compressedData\["data"\]', script)
    metadata = re.search(r'loadPackage\((\{files:\[.*?\],remote_package_size:\d+,package_uuid:"[^"]+"\})\)', script)
    chunk = re.search(r'exports\.CHUNK_SIZE=(\d+)', script)
    if not all((compressed, metadata, chunk)):
        raise ValueError('Unsupported loader: Emscripten LZ4 metadata not found')

    def parse_object(text):
        # These generated objects contain only arrays, strings, numbers and null.
        return json.loads(re.sub(r'([{,])([A-Za-z_][A-Za-z_0-9]*):', r'\1"\2":', text))

    layout = parse_object(compressed[1])
    manifest = parse_object(metadata[1])
    chunk_size = int(chunk[1])
    data = package.read_bytes()
    if len(data) != manifest['remote_package_size']:
        raise ValueError('Package size does not match loader')
    expected_hash = manifest['package_uuid'].removeprefix('sha256-')
    if hashlib.sha256(data).hexdigest() != expected_hash:
        raise ValueError('Package SHA-256 does not match loader')
    offsets, sizes, successes = (layout[key] for key in ('offsets', 'sizes', 'successes'))
    if not len(offsets) == len(sizes) == len(successes):
        raise ValueError('Inconsistent chunk tables')
    expanded = bytearray()
    for offset, size, compressed_flag in zip(offsets, sizes, successes):
        # JS minifiers may spell integral offsets in exponent notation (e.g. 1e6).
        if int(offset) != offset or int(size) != size:
            raise ValueError('Non-integral chunk range')
        offset, size = int(offset), int(size)
        if offset < 0 or size < 0 or offset + size > layout['cachedOffset']:
            raise ValueError('Invalid chunk range')
        block = data[offset:offset + size]
        expanded.extend(lz4.block.decompress(block, uncompressed_size=chunk_size) if compressed_flag else block)
    files = []
    for item in manifest['files']:
        path = PurePosixPath(item['filename'])
        if not path.is_relative_to('/game') or '..' in path.parts:
            raise ValueError('Invalid game asset path')
        start, end = item['start'], item['end']
        if int(start) != start or int(end) != end:
            raise ValueError('Non-integral game asset range')
        start, end = int(start), int(end)
        if not 0 <= start <= end <= len(expanded):
            raise ValueError('Invalid game asset range')
        files.append((path.relative_to('/game'), start, end))
    if destination.exists() and any(destination.iterdir()):
        raise ValueError('Destination must be empty; refusing to overwrite game data')
    destination.mkdir(parents=True, exist_ok=True)
    for relative, start, end in files:
        target = destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(expanded[start:end])
    print(f'Verified and exported {len(files)} assets ({len(expanded):,} bytes) to {destination}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--loader', type=Path, required=True)
    parser.add_argument('--data', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    unpack(args.loader, args.data, args.output)

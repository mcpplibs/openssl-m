#!/usr/bin/env python3
"""Audit the module surface against OpenSSL's headers.

The generated sources (src/crypto.cppm, src/ssl.cppm, src/macros.cppm,
src/detail/capture.hpp, tests/constants_test.cpp) must be exactly what
tools/surface.py writes from tools/surface.json; every macro of the record is
in exactly one class, with a reason when it stays textual. With a clang++
(--clang, or MWRAP_CLANG) the record itself is derived again from the
headers compat.openssl installs and must be the same: no public name left
out, none exported that is not there. MWRAP_REQUIRE_CLANG=1 makes the second
half mandatory (CI's Linux LLVM row).
"""
import argparse
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import surface  # noqa: E402


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--clang', default=os.environ.get('MWRAP_CLANG'))
    args = parser.parse_args()
    record = surface.record()
    for path, render in surface.OUTPUTS.items():
        assert (surface.ROOT / path).read_text(encoding='utf-8') == render(record), f'{path} is not what tools/surface.py --write gives'
    macros = record['macros']
    classes = [set(macros[kind]) for kind in ('constants', 'calls', 'textual')]
    assert not (classes[0] & classes[1] or classes[0] & classes[2] or classes[1] & classes[2]), 'a macro in two classes'
    assert all(entry['reason'] for entry in macros['textual'].values())
    if args.clang:
        derived = surface.derive(args.clang)
        if derived != record:
            for key in sorted(set(derived) | set(record)):
                if derived.get(key) != record.get(key):
                    print('differs:', key, file=sys.stderr)
            sys.exit('the record is not what clang derives from the headers: run tools/surface.py --clang <clang++> --write')
    else:
        assert os.environ.get('MWRAP_REQUIRE_CLANG') != '1', 'MWRAP_REQUIRE_CLANG=1 and no clang++ given'
    declared = sum(len(names) for module in ('crypto', 'ssl') for kind, names in record[module].items() if kind != 'internal')
    print(f'{declared} declarations exported; {len(record["internal"]["adapted"])} inline functions declared again, '
          f'{len(record["internal"]["not exported"])} stack/lhash helpers not; macros: {len(macros["constants"])} constants, '
          f'{len(macros["calls"])} calls, {len(macros["textual"])} textual; '
          + ('the record is what clang derives from the headers' if args.clang else 'the record not derived again (no clang++ given)'))


if __name__ == '__main__':
    main()

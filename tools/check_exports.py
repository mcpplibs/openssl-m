#!/usr/bin/env python3
"""Audit the module surface record against OpenSSL's headers.

src/surface.tsv is what build.mcpp writes the module sources from. It must be
well formed (every section, every entry with its fields), name every macro in
exactly one class, and give the reason for each that stays textual. With a
clang++ (--clang, or MWRAP_CLANG) the record is derived again from the headers
compat.openssl installs and must be the same text: no public name left out,
none exported that is not there. MWRAP_REQUIRE_CLANG=1 makes the second half
mandatory (CI's Linux LLVM row).
"""
import argparse
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import surface  # noqa: E402

SECTIONS = ['openssl', 'headers', *(f'{m} {k}' for m in ('crypto', 'ssl') for k in surface.DECLARATION_KINDS),
            'adapted', 'not exported', 'constants', 'calls', 'textual']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--clang', default=os.environ.get('MWRAP_CLANG'))
    args = parser.parse_args()
    text = surface.RECORD.read_text(encoding='utf-8')
    record = surface.sections(text)
    assert list(record) == SECTIONS, list(record)
    assert record['openssl'] == [[surface.VERSION]], record['openssl']
    assert all(len(e) == 2 and e[1] in ('crypto', 'ssl') for e in record['headers'])
    pairs = {'adapted': 3, 'calls': 3}  # fields before the (name, type) pairs
    for section, entries in record.items():
        for entry in entries:
            assert all(entry), (section, entry)
            if section in pairs:
                assert (len(entry) - pairs[section]) % 2 == 0, (section, entry)
            elif section in ('not exported', 'textual'):
                assert len(entry) == 2, (section, entry)
            elif section not in ('openssl', 'headers'):
                assert len(entry) == 1 or entry[1] == surface.CONDITIONAL.get(entry[0]), (section, entry)
    classes = [{e[0] for e in record[kind]} for kind in ('constants', 'calls', 'textual')]
    assert not (classes[0] & classes[1] or classes[0] & classes[2] or classes[1] & classes[2]), 'a macro in two classes'
    if args.clang:
        derived = surface.record_text(surface.derive(args.clang))
        if derived != text:
            ours, theirs = surface.sections(text), surface.sections(derived)
            for section in SECTIONS:
                if ours.get(section) != theirs.get(section):
                    print('differs:', section, file=sys.stderr)
            sys.exit('the record is not what clang derives from the headers: run tools/surface.py --clang <clang++>')
    else:
        assert os.environ.get('MWRAP_REQUIRE_CLANG') != '1', 'MWRAP_REQUIRE_CLANG=1 and no clang++ given'
    declared = sum(len(record[f'{m} {k}']) for m in ('crypto', 'ssl') for k in surface.DECLARATION_KINDS)
    print(f'{declared} declarations exported; {len(record["adapted"])} inline functions declared again, '
          f'{len(record["not exported"])} stack/lhash helpers not; macros: {len(record["constants"])} constants, '
          f'{len(record["calls"])} calls, {len(record["textual"])} textual; '
          + ('the record is what clang derives from the headers' if args.clang else 'the record not derived again (no clang++ given)'))


if __name__ == '__main__':
    main()

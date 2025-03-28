#!/usr/bin/env python3

import argparse
import os
import subprocess
import sys

from python.runfiles import Runfiles


RUNFILES = Runfiles.Create()
DUBC_PATH = RUNFILES.Rlocation('__main__/src/dubc')
CLANG_PATH = '/usr/local/bin/clang'

if not DUBC_PATH:
    print(f'Could not find dubc when RUNFILES_DIR={os.getenv("RUNFILES_DIR")}')
    sys.exit(1)


def link(obj_file, out):
    cmd = [CLANG_PATH, obj_file, '-o', out]
    print('$', ' '.join(cmd))
    link_ps = subprocess.run(cmd, text=True)
    if link_ps.returncode != 0:
        return
    print(f"generated {out}")


def compile(args, forward_args):
    dubc_ps = subprocess.run([DUBC_PATH, args.input] + forward_args, text=True)
    if dubc_ps.returncode != 0:
        return
    # TODO: This logic is in DUBC now
    file_prefix,_ = os.path.splitext(os.path.basename(args.input))
    link(obj_file=os.path.join('out', f'{file_prefix}.o'),
         out=os.path.join('out', f'{file_prefix}.out'))


def main():
    parser = argparse.ArgumentParser(
        prog='dub',
        description='dub language CLI tool')

    subparsers = parser.add_subparsers(required=True)

    compile_parser = subparsers.add_parser('compile', help='compile dub source files to object or executable files')
    compile_parser.add_argument('input', type=str, help='path to input file to compile')
    compile_parser.set_defaults(func=compile)

    args, forward_args = parser.parse_known_args()
    args.func(args, forward_args)


if __name__ == '__main__':
    main()

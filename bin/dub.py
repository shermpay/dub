#!/usr/bin/env python3

import argparse
import os
import subprocess
import sys

from python.runfiles import Runfiles


RUNFILES = Runfiles.Create()
DUBC_PATH = RUNFILES.Rlocation('__main__/src/dubc')
REPL_PATH = RUNFILES.Rlocation('__main__/src/repl')
CLANG_PATH = '/usr/local/bin/clang'

if not DUBC_PATH:
    print(f'Could not find dubc when RUNFILES_DIR={os.getenv("RUNFILES_DIR")}')
    sys.exit(1)


def link(obj_file, out):
    cmd = [CLANG_PATH, obj_file, '-o', out]
    print('$', ' '.join(cmd))
    link_ps = subprocess.run(cmd, text=True)
    if link_ps.returncode != 0:
        return link_ps.returncode
    print(f"generated {out}")
    return 0


def compile(args, forward_args):
    file_prefix,_ = os.path.splitext(os.path.basename(args.input))
    # forward_args += ['-emit_llvm', os.path.join('out', f'{file_prefix}.ll')]
    dubc_ps = subprocess.run([DUBC_PATH, args.input] + forward_args, text=True)
    if dubc_ps.returncode != 0:
        return dubc_ps.returncode
    # TODO: This logic is in DUBC now
    link(obj_file=os.path.join('out', f'{file_prefix}.o'),
         out=os.path.join('out', f'{file_prefix}.out'))
    return 0

def run(args, forward_args):
    compile_exitcode = compile(args, forward_args)
    if compile_exitcode:
        return compile_exitcode
    file_prefix,_ = os.path.splitext(os.path.basename(args.input))
    subprocess.run([os.path.join('out', f'{file_prefix}.out')])
    return 0

def repl(args, forward_args):
    subprocess.run([REPL_PATH])


def main():
    parser = argparse.ArgumentParser(
        prog='dub',
        description='dub language CLI tool')

    subparsers = parser.add_subparsers(required=True)

    compile_parser = subparsers.add_parser('compile', help='compile dub source files to object or executable files')
    compile_parser.add_argument('input', type=str, help='path to input file to compile')
    compile_parser.set_defaults(func=compile)


    run_parser = subparsers.add_parser('run', help='run dub source files to object or executable files')
    run_parser.add_argument('input', type=str, help='path to input file to run')
    run_parser.set_defaults(func=run)

    repl_parser = subparsers.add_parser('repl', help='Run interactive REPL')
    repl_parser.set_defaults(func=repl)

    args, forward_args = parser.parse_known_args()
    exitcode = args.func(args, forward_args)
    sys.exit(exitcode)


if __name__ == '__main__':
    main()

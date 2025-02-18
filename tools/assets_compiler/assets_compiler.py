import argparse
import subprocess

if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog='AssetsCompiler')
    parser.add_argument('-p', '--path')
    parser.add_argument('-v', '--verbose', action='store_true')
    parser.add_argument('-c', '--compiler')
    args = parser.parse_args()

    startline = [f'{args.compiler}', f'--verbose={int(args.verbose)}', '--path', f'{args.path}']
    proc = subprocess.Popen(startline, stdout=subprocess.PIPE)
    proc.wait()
    if proc.returncode == 0:
        print("Compiled successfuly")
    else:
        print("Failed to compile asset!")

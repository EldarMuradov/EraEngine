"""
Without arguments (pre-commit hook): formats changed lines of staged files.
  --all    format whole tracked files in place
  --check  with --all: only list files that would change, exit 1 if any
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

EXTENSIONS = {".h", ".hpp", ".c", ".cpp", ".inl"}
HUNK_RE = re.compile(rb"^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@")


def git(*args, input=None, check=True):
    result = subprocess.run(["git", *args], input=input, capture_output=True)
    if check and result.returncode != 0:
        raise RuntimeError(f"git {' '.join(args)} failed:\n{result.stderr.decode(errors='replace')}")
    return result


def warn(message):
    print(f"[format] {message}", file=sys.stderr)


def find_clang_format():
    for name in (os.environ.get("CLANG_FORMAT"), "clang-format"):
        path = shutil.which(name) if name else None
        if path:
            return path

    program_files = os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")
    vswhere = Path(program_files) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    if not vswhere.exists():
        return None
    result = subprocess.run([str(vswhere), "-latest", "-products", "*", "-property", "installationPath"],
        capture_output=True, text=True)
    install_dir = result.stdout.strip()
    candidate = Path(install_dir) / "VC" / "Tools" / "Llvm" / "x64" / "bin" / "clang-format.exe"
    return str(candidate) if install_dir and candidate.exists() else None


def load_format_paths(root):
    list_file = Path(os.environ.get("ERA_BUILD_DIR", root / "_build")) / "era_format_paths.txt"
    if not list_file.exists():
        return None

    includes, excludes = [], []
    for line in list_file.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if len(line) < 2 or line[0] not in "+-":
            continue
        (includes if line[0] == "+" else excludes).append(line[1:].rstrip("/") + "/")
    return includes, excludes


def is_format_target(path, includes, excludes):
    return (Path(path).suffix.lower() in EXTENSIONS
        and any(path.startswith(p) for p in includes)
        and not any(path.startswith(p) for p in excludes))


def changed_line_args(*diff_args):
    out = git("diff", *diff_args, "-U0", "--no-color", "--no-ext-diff").stdout
    args = []
    for line in out.splitlines():
        match = HUNK_RE.match(line)
        if not match:
            continue
        start = int(match.group(1))
        count = int(match.group(2)) if match.group(2) is not None else 1
        if count > 0:
            args.append(f"--lines={start}:{start + count - 1}")
    return args


def clang_format(exe, path, line_args, input=None, in_place=False):
    cmd = [exe, "--style=file", *line_args]
    cmd += ["-i", path] if in_place else [f"--assume-filename={path}"]
    result = subprocess.run(cmd, input=input, capture_output=True)
    if result.returncode != 0:
        raise RuntimeError(f"clang-format failed on {path}:\n{result.stderr.decode(errors='replace')}")
    return result.stdout


def format_file(exe, path):
    line_args = changed_line_args("--cached", "--", path)
    if not line_args:
        return False

    fully_staged = git("diff", "--quiet", "--", path, check=False).returncode == 0

    staged = git("show", f":{path}").stdout
    formatted = clang_format(exe, path, line_args, input=staged)
    if formatted == staged:
        return False

    mode = git("ls-files", "-s", "--", path).stdout.split()[0].decode()
    blob = git("hash-object", "-w", "--no-filters", "--stdin", input=formatted).stdout.strip().decode()
    git("update-index", "--cacheinfo", f"{mode},{blob},{path}")

    if fully_staged:
        git("checkout-index", "-f", "--", path)
    else:
        worktree_line_args = changed_line_args("HEAD", "--", path)
        if worktree_line_args:
            clang_format(exe, path, worktree_line_args, in_place=True)
    return True


def format_whole_file(exe, path, check):
    original = Path(path).read_bytes()
    formatted = clang_format(exe, path, [], input=original)
    if formatted == original:
        return False
    if not check:
        Path(path).write_bytes(formatted)
    return True


def run_all(exe, format_paths, check):
    tracked = git("ls-files", "-z").stdout.decode().split("\0")
    targets = [p for p in tracked if p and is_format_target(p, *format_paths)]

    with ThreadPoolExecutor() as pool:
        results = list(pool.map(lambda p: format_whole_file(exe, p, check), targets))

    changed = [p for p, was_changed in zip(targets, results) if was_changed]
    for path in changed:
        print(path)
    print(f"[format] {len(changed)} of {len(targets)} files {'need formatting' if check else 'formatted'}.")
    return 1 if check and changed else 0


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--all", action="store_true", help="format whole tracked files instead of staged changes")
    parser.add_argument("--check", action="store_true", help="with --all: do not write, exit 1 if changes are needed")
    args = parser.parse_args()
    if args.check and not args.all:
        parser.error("--check requires --all")
    skip_code = 1 if args.all else 0

    root = Path(git("rev-parse", "--show-toplevel").stdout.decode().strip())
    os.chdir(root)

    format_paths = load_format_paths(root)
    if format_paths is None:
        warn("era_format_paths.txt not found (generate the project or set ERA_BUILD_DIR), skipping.")
        return skip_code

    exe = find_clang_format()
    if exe is None:
        warn("clang-format not found (install VS C++ Clang tools or set CLANG_FORMAT), skipping.")
        return skip_code

    if args.all:
        return run_all(exe, format_paths, args.check)

    staged = git("diff", "--cached", "--name-only", "--diff-filter=ACMR", "-z").stdout.decode().split("\0")
    targets = [p for p in staged if p and is_format_target(p, *format_paths)]

    formatted = [p for p in targets if format_file(exe, p)]
    for path in formatted:
        warn(f"formatted {path}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except RuntimeError as error:
        warn(str(error))
        warn("aborted; for a commit, 'git commit --no-verify' skips formatting.")
        sys.exit(1)

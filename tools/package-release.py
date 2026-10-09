"""Package a Windows build plus tracked source without local race data."""
from pathlib import Path
import argparse
import hashlib
import shutil
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=ROOT / 'LANDELTA.exe')
    parser.add_argument('--output', type=Path, default=ROOT / 'dist')
    args = parser.parse_args()
    binary = args.binary.resolve()
    if not binary.is_file() or binary.read_bytes()[:2] != b'MZ':
        raise SystemExit('A Windows PE executable is required.')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    exe = output / 'LANDELTA.exe'
    if binary != exe:
        shutil.copyfile(binary, exe)
    names = subprocess.check_output(['git', 'ls-files', '-z'], cwd=ROOT).decode('utf-8').split('\0')
    files = [ROOT / name for name in names if name and not name.startswith(('.git', 'Vista-'))]
    files = [file for file in files if file.is_file() and file != binary]
    archive = output / 'LANDELTA-Windows-Beta.zip'
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as package:
        package.write(exe, 'LANDELTA/LANDELTA.exe')
        for file in sorted(files):
            package.write(file, 'LANDELTA/' + file.relative_to(ROOT).as_posix())
    with zipfile.ZipFile(archive) as package:
        if package.testzip() is not None or package.read('LANDELTA/LANDELTA.exe') != exe.read_bytes():
            raise SystemExit('Release archive integrity check failed.')
    checksums = output / 'SHA256SUMS.txt'
    checksums.write_text(''.join(f'{hashlib.sha256(file.read_bytes()).hexdigest()}  {file.name}\n'
                               for file in (exe, archive)), encoding='utf-8')
    print(f'Packaged {len(files)} source/doc files, Windows EXE and SHA-256 checksums.')


if __name__ == '__main__':
    main()

"""Package only runtime files; source remains in GitHub, not the user download."""
from pathlib import Path
import argparse
import hashlib
import shutil
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
    files = [ROOT / 'tools/ac-analyzer.html', ROOT / 'tools/session-analysis.js', *sorted((ROOT / 'licenses').glob('*.txt')),
             ROOT / 'vendor/JSON-LICENSE.txt']
    if not all(file.is_file() for file in files):
        raise SystemExit('Missing runtime file or required license.')
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
    print(f'Packaged Windows EXE, analyzer and licenses only ({len(files)} support files).')


if __name__ == '__main__':
    main()

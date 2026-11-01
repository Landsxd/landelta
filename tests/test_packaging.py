"""Verify the actual portable archive and installer runtime allowlist."""
from pathlib import Path
import hashlib
import subprocess
import tempfile
import zipfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='landelta-package-') as tmp:
    folder = Path(tmp)
    binary = folder / 'test.exe'
    binary.write_bytes(b'MZ-test-not-a-real-executable')
    output = folder / 'out'
    subprocess.run(['python3', str(root / 'tools/package-release.py'), '--binary', str(binary),
                    '--output', str(output)], check=True)
    with zipfile.ZipFile(output / 'LANDELTA-Windows-Beta.zip') as archive:
        assert set(archive.namelist()) == {
            'LANDELTA/LANDELTA.exe', 'LANDELTA/tools/session-analysis.js', 'LANDELTA/tools/ac-analyzer.html',
            'LANDELTA/licenses/GCC-MinGW-notices.txt',
            'LANDELTA/licenses/MinGW-runtime-notices.txt', 'LANDELTA/vendor/JSON-LICENSE.txt'}
        assert archive.read('LANDELTA/LANDELTA.exe') == binary.read_bytes()
        assert b'function readCSV' in archive.read('LANDELTA/tools/ac-analyzer.html')
    for line in (output / 'SHA256SUMS.txt').read_text().splitlines():
        digest, name = line.split('  ')
        assert hashlib.sha256((output / name).read_bytes()).hexdigest() == digest
src = (root / 'src/main.cpp').read_text()
assert 'Probar modo demo' not in src
assert 'std::wstring(args).find(L"--demo")' not in src
assert 'La versión 0.13.4 no se ha iniciado' in src
installer = (root / 'installer/landelta.iss').read_text()
assert 'AppMutex=Local\\VortexACCOverlay' in installer
assert 'PrivilegesRequired=lowest' in installer
assert '[UninstallDelete]' not in installer  # Never recursively remove user data.
assert 'CloseApplications=no' in installer  # No forced capture termination.
assert 'ABRIR-DEMO' not in installer
print('PASS: runtime-only packages, integrity, version conflict notice, no demo entry and safe installer policy')

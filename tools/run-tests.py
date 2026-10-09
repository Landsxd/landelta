"""Run portable tests in Linux. Does not launch ACC or native Windows APIs."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
environment = dict(os.environ)
environment["ASAN_OPTIONS"] = "detect_leaks=0"
environment["UBSAN_OPTIONS"] = "halt_on_error=1"
cpp_tests = (
    "test_telemetry", "test_analytics", "test_config_codec", "test_relatives",
    "test_driving", "test_lap_delta", "test_background",
)
python_tests = (
    "test_connection_loop", "test_settings", "test_background_loop", "test_startup", "test_history_export",
)

with tempfile.TemporaryDirectory(prefix="landelta-tests-") as directory:
    for name in cpp_tests:
        executable = Path(directory) / name
        subprocess.run([
            "g++", "-std=c++17", "-g", "-fsanitize=address,undefined",
            "-I", str(root / "src"), "-I", str(root / "vendor"),
            str(root / "tests" / (name + ".cpp")), "-o", str(executable),
        ], check=True, cwd=root)
        subprocess.run([str(executable)], check=True, cwd=root, env=environment)

for name in python_tests:
    subprocess.run(["python3", str(root / "tests" / (name + ".py"))], check=True, cwd=root, env=environment)

print("PASS: all portable tests; Windows/ACC live validation remains separate")

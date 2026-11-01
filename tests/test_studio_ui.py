from pathlib import Path
root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text()
studio=(root/'src/ac_studio.hpp').read_text()
ui=(root/'src/studio_ui.hpp').read_text()
assert 'bool acAuto=true' in main and 'int gameMode=0' in main
assert '{"gameMode",cfg.gameMode}' in main and '{"acAuto",cfg.acAuto}' in main
assert 'cfg.acAuto=j.value("acAuto",true)' in main
assert 'acstudio::studio.directory=dataDir/L"ac-telemetry";if(cfg.acAuto&&!dataDir.empty())acstudio::studio.start();' in main
assert 'acstudio::open' not in main
assert 'CreateWindowW' not in studio  # AC is no longer a separate unstyled window.
assert 'if(running)return;stop();running=true' in studio  # No duplicate workers.
assert 'if(g.status!=2)' in studio  # Pause/replay/menu cannot become recorded laps.
assert 'void stop(){running=false;if(worker.joinable())worker.join();}' in studio
assert 'case 84:cfg.acAuto=!cfg.acAuto' in main
assert 'AC · Studio' in main and 'ACC · Overlay' in main
assert 'Analizar vueltas / exportar' in ui and 'Mis sesiones' in ui
print('PASS: two-mode UI routing, auto-capture startup ordering, persisted toggle and capture lifecycle guards (source checks)')

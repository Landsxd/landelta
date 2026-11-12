# LANDELTA 0.14.0 — Race Studio (beta)

## New native Windows workspace

- Graphite and racing-lime interface, 1200 × 780 logical canvas, resizable/maximizable.
- Visible AC, ACC and iRacing selectors. Session, Telemetry, Your Screen, Settings.
- Track/car identity, lap times and sector table inside the application.
- Track trace drawn from captured coordinates, not a downloaded or invented circuit map.
- Speed, applied brake/throttle and native steering graphs versus lap distance; cursor readout and best-lap comparison.
- Official/derived three-sector display or ten estimated equal-distance microsectors. Missing values stay blank.
- Session browser with date, including existing AC CSV recordings and legacy ACC lap history. No Explorer/browser step needed.
- Export session ZIP: original samples CSV, lap summary CSV, session metadata JSON. Optional export is the only normal file dialog.
- Drag overlays on a scaled virtual desktop, including negative monitor coordinates, then save. Per-game layouts and scale persist.
- Lap/sectors and pace/catch overlays are retired, even if enabled in old settings. ACC retains classification, relatives, pedals, delta and recent sectors.
- Normal minimize stays on the taskbar; X stays in the tray. User data is not deleted by installation/uninstallation.

## Capture and limitations

- AC's existing recorder remains the full-rate CSV source. The dashboard observes a downsampled preview; opening a stored session loads its original samples.
- New ACC/iRacing session recordings observe up to 10 new samples/second through the app timer. This is not full SDK-rate telemetry and does not claim millisecond physical sampling precision. Timings retain the game's supplied milliseconds.
- Reconnecting UDP does not itself change the ACC recording identity. Game process/session/index changes and lap-counter reset start new recordings. Pauses keep recordings open.
- iRacing is experimental: read-only `Local\\IRSDKMemMapFileName`, runtime variable lookup, checked bounds, coherent buffer copies, reconnection on stalled ticks. No web API or credentials.
- iRacing overlays: classification, nearby cars on track, pedals and best-lap delta when available. Gaps are estimates, not asserted official intervals. No ACC car-ID mapping is applied to iRacing cars.
- AC overlays in this version: pedals and locally estimated delta; no unsupported AC competitor grid is fabricated.
- iRacing official sectors and lap validity remain unknown when not supplied. Microsectors are available from progress. No incident-based validity inference.
- AC/ACC steering retains native units; iRacing steering is radians. Pedals are normalized applied inputs, not foot force or hydraulic pressure.
- Sessions without position channels do not display a track shape. Legacy ACC history has lap/sector summaries but cannot recover telemetry never recorded.
- Microsector start/end estimates require samples on both sides of a finish crossing within 500 ms and an official lap time; otherwise remain unknown. Sector crossings are conservatively latched; an unchanged carried sector value is not reused.
- A loaded graph is capped at one million samples; underlying original CSV remains untouched and included in export. Missing metadata (car version/temperature) stays unknown.

## Validation

Portable tests cover iRacing binary bounds/types/missing variables, lap boundaries, gaps, CSV roundtrip/deduplication, desktop transforms, three-game settings, and native ZIP checked independently with Python. Existing ACC connection/session, delta, history, packaging and startup tests remain.

Windows CI compiles the EXE, renders the actual native UI and tests installation, reinstall, shortcuts, startup migration, running-app protection and data-preserving uninstall.

Real AC/ACC/iRacing driving remains a separate validation step. Test practice → qualifying → race, pause/resume, return to pits, app minimize/close, monitor scaling and session reopening before general distribution. This beta is not a claim of live-game validation.

// Uses the main window's drawing helpers and proportional layout.
void renderStudio(Graphics& g){
 auto state=acstudio::studio.read();const auto& s=state.first;
 fill(g,28,92,600,188,CARD);fill(g,28,92,4,188,GREEN);
 text(g,L"AC STUDIO",46,105,300,29,21,FG,FontStyleBold);
 text(g,s.active?L"● GRABANDO":(cfg.acAuto?L"● AUTOMÁTICO":L"○ DETENIDO"),380,108,228,26,14,s.active?GREEN:GOLD,FontStyleBold,StringAlignmentFar);
 text(g,s.track.empty()?L"Esperando Assetto Corsa":s.track,46,141,548,30,19,FG,FontStyleBold);
 text(g,s.car.empty()?L"Abre una práctica, clasificación o carrera":s.car,46,174,548,23,13,MUTED);
 text(g,s.active?L"VUELTA "+std::to_wstring(s.lap):L"ÚLTIMA LECTURA",46,212,184,21,12,MUTED,FontStyleBold);
 text(g,s.lap?lapTime(s.currentMs):L"—:—.———",44,232,314,43,32,s.active?GREEN:MUTED,FontStyleBold);
 text(g,L"ÚLTIMA VUELTA",388,211,210,22,12,MUTED,FontStyleBold,StringAlignmentFar);
 text(g,s.lastMs>0&&s.lastMs<INT32_MAX?lapTime(s.lastMs):L"—",370,239,228,31,23,FG,FontStyleBold,StringAlignmentFar);
 for(int i=0;i<3;i++)fill(g,28+i*204.f,298,192,108,CARD);
 text(g,L"ACELERADOR",42,310,162,21,12,MUTED,FontStyleBold);
 text(g,L"FRENO",246,310,162,21,12,MUTED,FontStyleBold);
 text(g,L"VELOCIDAD",450,310,162,21,12,MUTED,FontStyleBold);
 for(int i=0;i<2;i++){float x=42+i*204.f;fill(g,x,354,164,21,BG);float v=s.active?(i?s.brake:s.gas):0;fill(g,x,354,164*std::clamp(v,0.f,1.f),21,i?RED:GREEN);}
 text(g,s.active?num(s.speed,0)+L" km/h":L"—",450,345,162,39,24,FG,FontStyleBold);
 button(g,L"Analizar vueltas / exportar",28,426,360,47,82,true);
 button(g,L"Mis sesiones",400,426,228,47,83);
 button(g,cfg.acAuto?L"Captura automática: ON · detener y guardar":L"Captura automática: OFF · activar",28,489,600,42,84,false,cfg.acAuto);
 text(g,state.second,28,543,600,44,14,s.active?GREEN:MUTED);
 text(g,L"Un CSV por sesión · todas las vueltas · análisis local",28,592,600,24,13,MUTED);
 line(g,28,624,600);text(g,L"La X mantiene la captura · Salir guarda y cierra · No se graban replays",28,636,600,25,11,MUTED);
}

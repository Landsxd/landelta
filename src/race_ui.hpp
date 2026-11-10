#include "session_zip.hpp"
// Native, offline Race Studio. Uses exactly the same renderer in previews and EXE.
int layoutDrag=-1;float dragDX=0,dragDY=0;bool layoutDirty=false,studioPreview=false;
float draftScale=1,draftDelta=1;std::array<bool,PanelCount> draftVisible{};
std::array<int,PanelCount> draftX{},draftY{};race::ScreenTransform screenMap;
std::vector<std::pair<RectF,int>> layoutHits;
const wchar_t* panelName(int i){const wchar_t* n[]={L"Clasificación",L"Relativos",L"",L"Pedales",L"Delta",L"Últimos sectores",L""};return n[i];}
void beginLayout(){draftScale=cfg.scale;draftDelta=cfg.deltaScale;for(int i=0;i<PanelCount;++i)draftVisible[i]=cfg.visible[i];int left=studioPreview?0:GetSystemMetrics(SM_XVIRTUALSCREEN),top=studioPreview?0:GetSystemMetrics(SM_YVIRTUALSCREEN),width=studioPreview?1920:GetSystemMetrics(SM_CXVIRTUALSCREEN),height=studioPreview?1080:GetSystemMetrics(SM_CYVIRTUALSCREEN);width=std::max(1,width);height=std::max(1,height);for(int i=0;i<PanelCount;++i){int w=int(panelW(i)*cfg.scale*(i==4?cfg.deltaScale:1)),h=int(panelH(i)*cfg.scale*(i==4?cfg.deltaScale:1));draftX[i]=std::clamp(cfg.x[i],left,left+std::max(0,width-w));draftY[i]=std::clamp(cfg.y[i],top,top+std::max(0,height-h));}layoutDirty=false;layoutDrag=-1;}
void studioSwitch(int mode){reviewRequest.cancel();studioNotice.clear();cfg.saveProfile();cfg.gameMode=mode;cfg.restoreProfile();reviewing=false;selectedLap=compareLap=-1;lapOffset=0;beginLayout();refreshCatalog();}
void label(Graphics& g,const std::wstring& s,float x,float y,float w=250,Color c=MUTED){text(g,s,x,y,w,22,12,c,FontStyleBold);}
void metric(Graphics& g,const std::wstring& title,const std::wstring& value,float x,float y,float width,Color color=FG){fill(g,x,y,width,91,CARD);label(g,title,x+16,y+12,width-32);text(g,value,x+14,y+35,width-28,44,30,color,FontStyleBold);}
std::wstring stateLabel(const race::Lap& l){if(!l.complete)return L"En curso / parcial";if(l.pit)return L"Boxes";if(l.partial)return L"Captura parcial";return l.valid==0?L"Inválida":(l.valid==1?L"Válida":L"Validez desconocida");}
void trackMap(Graphics& g,const race::Session& s,RectF r){
 fill(g,r.X,r.Y,r.Width,r.Height,CARD);label(g,L"TRAZADO REGISTRADO",r.X+16,r.Y+12,r.Width-32);
 double minx=INFINITY,minz=INFINITY,maxx=-INFINITY,maxz=-INFINITY;
 for(const auto& p:s.samples)if(std::isfinite(p.x)&&std::isfinite(p.z)){minx=std::min(minx,p.x);maxx=std::max(maxx,p.x);minz=std::min(minz,p.z);maxz=std::max(maxz,p.z);}
 if(!std::isfinite(minx)||maxx-minx<1e-7||maxz-minz<1e-7){text(g,L"El trazado se dibuja al conducir",r.X+16,r.Y+55,r.Width-32,30,15,MUTED);text(g,L"Solo con coordenadas disponibles",r.X+16,r.Y+88,r.Width-32,25,12,MUTED);return;}
 double k=std::min((r.Width-48)/(maxx-minx),(r.Height-80)/(maxz-minz));auto point=[&](const race::Sample& p){return PointF(float(r.X+r.Width/2+(p.x-(minx+maxx)/2)*k),float(r.Y+45+(r.Height-60)/2-(p.z-(minz+maxz)/2)*k));};
 Pen pen(Color(255,119,145,127),3);size_t stride=std::max(size_t(1),s.samples.size()/1600);for(size_t i=stride;i<s.samples.size();i+=stride){auto& a=s.samples[i-stride];auto& b=s.samples[i];if(!std::isfinite(a.x)||!std::isfinite(a.z)||!std::isfinite(b.x)||!std::isfinite(b.z)||b.at<a.at||b.at-a.at>2000)continue;auto p=point(a),q=point(b);if(std::hypot(q.X-p.X,q.Y-p.Y)<r.Width*.45)g.DrawLine(&pen,p,q);}
 if(!s.samples.empty()&&std::isfinite(s.samples.back().x)&&std::isfinite(s.samples.back().z)){auto p=point(s.samples.back());SolidBrush b(GREEN);g.FillEllipse(&b,p.X-5,p.Y-5,10.f,10.f);}
}
void lapTable(Graphics& g,race::Session& s,float y,int rows){
 label(g,L"VUELTAS DE LA SESIÓN",228,y,280);button(g,L"Exportar PNG",900,y-2,158,28,228);button(g,L"‹",1072,y-2,38,28,221);button(g,L"›",1118,y-2,38,28,222);y+=36;
 const float x[]={240,304,468,634,800,958};const wchar_t* titles[]={L"VUELTA",L"TIEMPO",L"S1",L"S2",L"S3",L"ESTADO"};for(int i=0;i<6;++i)label(g,titles[i],x[i],y,155);
 lapOffset=std::clamp(lapOffset,0,std::max(0,int(s.laps.size())-rows));int first=lapOffset;
 if(!reviewing&&selectedLap<0)first=std::max(0,int(s.laps.size())-rows);
 for(int row=0;row<rows&&first+row<int(s.laps.size());++row){auto& l=s.laps[first+row];float yy=y+28+row*37;fill(g,228,yy,928,35,l.number==selectedLap?Color(255,39,57,28):(row%2?BG:CARD));hits.emplace_back(RectF(228,yy,928,35),300+first+row);text(g,std::to_wstring(l.number),x[0],yy,56,35,18,FG,FontStyleBold);text(g,lapTime(l.ms),x[1],yy,156,35,21,l.ms&&l.ms==s.best()?GREEN:FG,FontStyleBold);for(int k=0;k<3;++k)text(g,lapTime(l.sectors[k]),x[k+2],yy,155,35,18,FG);text(g,stateLabel(l),x[5],yy,190,35,12,l.valid==0?GOLD:MUTED);}
 if(s.laps.empty())text(g,L"Las vueltas aparecerán aquí automáticamente al entrar en pista.",240,y+38,890,40,17,MUTED);
}
void renderSession(Graphics& g){auto& s=currentSession();race::Sample p;if(!s.samples.empty())p=s.samples.back();
 metric(g,L"VUELTA ACTUAL",p.lap?lapTime(p.ms):L"—",228,155,300,GREEN);metric(g,L"ÚLTIMA VUELTA",lapTime(p.last),542,155,300);metric(g,L"MEJOR REGISTRADA",lapTime(s.best()),856,155,300);
 trackMap(g,s,RectF(228,264,436,200));fill(g,680,264,476,200,CARD);label(g,L"EN PISTA",698,276,200);text(g,s.car.empty()?L"Coche pendiente de detectar":wide(s.car),698,306,440,32,20,FG,FontStyleBold);
 text(g,std::isfinite(p.speed)?num(p.speed,0)+L" km/h":L"— km/h",698,349,200,38,28,GREEN,FontStyleBold);text(g,L"V"+std::to_wstring(p.lap)+L"  /  "+(p.pit?L"BOXES":(studioRecords[cfg.gameMode].online&&!reviewing?L"EN DIRECTO":L"GUARDADO")),922,350,210,38,16,MUTED,FontStyleBold,StringAlignmentFar);
 text(g,L"Historial y gráficas dentro de LANDELTA",698,400,440,27,14,MUTED);text(g,L"Selecciona una vuelta para inspeccionar su telemetría.",698,429,440,22,12,MUTED);
 lapTable(g,s,486,5);
}
double channel(const race::Sample& p,int c){return c==0?p.speed:(c==1?p.brake*100:(c==2?p.gas*100:p.steer));}
void renderTelemetry(Graphics& g){auto& s=currentSession();if(selectedLap<0&&!s.laps.empty())selectedLap=s.laps.back().number;
 std::wstring selection=L"Vuelta "+(selectedLap<0?std::wstring(L"—"):std::to_wstring(selectedLap));button(g,L"‹",228,153,36,34,223);text(g,selection,276,153,145,34,20,FG,FontStyleBold);button(g,L"›",424,153,36,34,224);button(g,compareLap<0?L"Comparar mejor":L"Comparando V"+std::to_wstring(compareLap),476,153,192,34,225,false,compareLap>=0);
 button(g,sectorMode?L"10 microsectores":L"Sectores oficiales",686,153,214,34,226,false,sectorMode);button(g,L"Exportar sesión ZIP",916,153,240,34,227);
 std::vector<const race::Sample*> traces[2];for(const auto& p:s.samples){if(p.lap==compareLap)traces[0].push_back(&p);if(p.lap==selectedLap)traces[1].push_back(&p);}
 const wchar_t* names[]={L"VELOCIDAD · km/h",L"FRENO · % aplicado",L"GAS · % aplicado",L"VOLANTE"};Color colors[]={BLUE,RED,GREEN,GOLD};
 const float gx=410,gw=728;const float tops[]={209,316,423,530};
 for(int c=0;c<4;++c){float y=tops[c];fill(g,228,y,928,98,CARD);label(g,c==3?std::wstring(names[c])+L" · "+wide(s.steerUnit):names[c],242,y+10,165,colors[c]);
  double low=0,high=c==0?350:100;if(c==3){low=-1;high=1;for(const auto& p:s.samples)if((p.lap==selectedLap||p.lap==compareLap)&&std::isfinite(p.steer)){low=std::min(low,p.steer);high=std::max(high,p.steer);}}
  for(int j=0;j<3;++j)line(g,gx,y+12+j*34,gw,EDGE);
  for(int pass=0;pass<2;++pass){int lap=pass?selectedLap:compareLap;if(lap<0)continue;Pen pen(pass?colors[c]:Color(255,105,116,120),pass?2.f:1.5f);bool have=false;race::Sample previous;
   for(const auto* sample:traces[pass]){const auto& p=*sample;double value=channel(p,c);if(p.lap!=lap||!std::isfinite(value)||!std::isfinite(p.progress)){have=false;continue;}if(have&&p.progress>=previous.progress&&p.progress-previous.progress<.03&&p.at>=previous.at&&p.at-previous.at<500){auto py=[&](double v){return y+84-float(std::clamp((v-low)/(high-low),0.,1.)*70);};g.DrawLine(&pen,gx+float(previous.progress)*gw,py(channel(previous,c)),gx+float(p.progress)*gw,py(value));}previous=p;have=true;}
  }
  const race::Sample* nearest=nullptr;if(graphCursor>=0){for(const auto* p:traces[1])if(!nearest||std::abs(p->progress-graphCursor)<std::abs(nearest->progress-graphCursor))nearest=p;Pen cursor(FG,1);float cx=gx+float(graphCursor)*gw;g.DrawLine(&cursor,cx,y+8,cx,y+89);}
  if(nearest)text(g,num(channel(*nearest,c),c==3?3:1),242,y+40,160,37,26,colors[c],FontStyleBold);else text(g,L"—",242,y+40,160,37,26,MUTED);
 }
 label(g,L"DISTANCIA DE VUELTA  ·  0 %",410,630,380);text(g,L"100 %",1078,630,60,22,12,MUTED,FontStyleBold,StringAlignmentFar);
 auto l=std::find_if(s.laps.begin(),s.laps.end(),[](const auto& v){return v.number==selectedLap;});int bins=sectorMode?10:3;float cell=928.f/bins;
 for(int i=0;i<bins;++i){float x=228+i*cell;fill(g,x,663,cell-5,53,CARD);label(g,(sectorMode?L"M":L"S")+std::to_wstring(i+1),x+8,665,cell-16);int ms=0;if(l!=s.laps.end())ms=sectorMode?(std::isfinite(l->micro[i])?int(l->micro[i]):0):l->sectors[i];text(g,lapTime(ms),x+8,687,cell-16,24,bins==10?13:18,FG,FontStyleBold);}
 text(g,sectorMode?L"Microsectores estimados por distancia · los huecos no se rellenan":L"Sectores recibidos / derivados del tiempo oficial · — indica falta de datos",228,722,928,22,12,MUTED);
 if(s.samples.empty())text(g,L"Esta sesión no contiene muestras de telemetría.",410,247,708,35,18,MUTED);
}
void renderLayout(Graphics& g){
 label(g,L"TU PANTALLA",228,153,300,GREEN);text(g,L"Arrastra los paneles. Los cambios se aplican al guardar.",228,181,928,28,18,FG);
 screenMap={double(GetSystemMetrics(SM_XVIRTUALSCREEN)),double(GetSystemMetrics(SM_YVIRTUALSCREEN)),double(GetSystemMetrics(SM_CXVIRTUALSCREEN)),double(GetSystemMetrics(SM_CYVIRTUALSCREEN)),228,224,928,365};if(studioPreview)screenMap={0,0,1920,1080,228,224,928,365};if(screenMap.width<1||screenMap.height<1){screenMap.width=1920;screenMap.height=1080;}
 auto origin=screenMap.toMini(screenMap.left,screenMap.top);double k=screenMap.scale();fill(g,228,224,928,365,Color(255,8,12,10));fill(g,float(origin.first),float(origin.second),float(screenMap.width*k),float(screenMap.height*k),Color(255,19,27,23));
 Pen grid(Color(255,32,44,35),1);for(int j=1;j<12;++j){float x=float(origin.first+screenMap.width*k*j/12);g.DrawLine(&grid,x,float(origin.second),x,float(origin.second+screenMap.height*k));}for(int j=1;j<7;++j){float y=float(origin.second+screenMap.height*k*j/7);g.DrawLine(&grid,float(origin.first),y,float(origin.first+screenMap.width*k),y);}
 layoutHits.clear();for(int i=0;i<PanelCount;++i){if(!panelAllowed(i)||!draftVisible[i])continue;auto p=screenMap.toMini(draftX[i],draftY[i]);float w=float(panelW(i)*draftScale*(i==4?draftDelta:1)*k),h=float(panelH(i)*draftScale*(i==4?draftDelta:1)*k);RectF r(float(p.first),float(p.second),w,h);fill(g,r.X,r.Y,r.Width,r.Height,Color(255,42,65,30));Pen border(GREEN,1.5);g.DrawRectangle(&border,r);text(g,panelName(i),r.X+4,r.Y+2,r.Width-8,std::min(28.f,r.Height),std::min(14.f,std::max(9.f,r.Height*.4f)),FG,FontStyleBold);layoutHits.emplace_back(r,i);}
 float bx=228;for(int i=0;i<PanelCount;++i)if(panelAllowed(i)){button(g,panelName(i),bx,607,174,32,10+i,false,draftVisible[i]);bx+=184;}
 button(g,L"− Tamaño",228,657,118,34,20);text(g,num(draftScale*100,0)+L" %",352,657,74,34,16,FG,FontStyleBold);button(g,L"+ Tamaño",432,657,118,34,21);button(g,L"Cancelar",680,657,130,38,230);button(g,L"Restablecer",822,657,148,38,231);button(g,L"Guardar distribución",982,657,174,38,232,true);
 label(g,L"TAMAÑO DELTA",228,710,155);button(g,L"−",388,706,38,32,27);text(g,num(draftDelta*100,0)+L" %",436,706,80,32,16,FG);button(g,L"+",526,706,38,32,28);text(g,layoutDirty?L"Cambios sin guardar":L"Distribución guardada · F10 + Ctrl + Alt: mover",590,710,566,28,13,layoutDirty?GOLD:MUTED);
}
void renderSettings(Graphics& g){
 label(g,L"CONEXIÓN Y CAPTURA",228,155,700,GREEN);fill(g,228,193,928,126,CARD);
 text(g,cfg.gameMode==1?status:(cfg.gameMode==0?acstudio::studio.read().second:(irClient.connected?L"iRacing conectado · SDK local":L"Esperando iRacing · entra en pista")),246,207,890,36,21,FG,FontStyleBold);
 text(g,cfg.gameMode==1?configStatus:(cfg.gameMode==0?L"Captura de AC en segundo plano. No se graban replays.":L"Integración experimental · lectura local sin cuenta ni contraseña"),246,252,890,28,15,MUTED);
 text(g,cfg.gameMode==2?L"Sectores oficiales y validez: desconocidos cuando el SDK no los expone.":L"Tus sesiones se guardan automáticamente y se abren desde la barra lateral.",246,285,890,24,13,MUTED);
 if(cfg.gameMode==1){button(g,L"Reparar ACC",228,336,200,40,40);button(g,L"Reconectar",444,336,200,40,42);button(g,L"Diagnóstico",660,336,200,40,44);button(g,L"Carpeta de ACC",876,336,280,40,41);}
 if(cfg.gameMode==0)button(g,cfg.acAuto?L"Captura automática: ON":L"Captura automática: OFF",228,336,350,40,84,false,cfg.acAuto);
 button(g,startupInfo.current?L"Inicio con Windows: ON":L"Inicio con Windows: OFF",228,406,448,44,60,false,startupInfo.current);button(g,cfg.autoShow?L"Mostrar al entrar: ON":L"Mostrar al entrar: OFF",694,406,462,44,64,false,cfg.autoShow);
 button(g,cfg.keepAfterRace?L"Fuera de sesión: ON":L"Fuera de sesión: OFF",228,469,448,44,29,false,cfg.keepAfterRace);button(g,cfg.hidden?L"Mostrar overlays":L"Ocultar overlays",694,469,462,44,71);
 label(g,L"FONDO OVERLAY",228,543,190);button(g,L"−",430,540,40,32,22);text(g,num(cfg.opacity/255.*100,0)+L" %",480,540,90,32,16,FG);button(g,L"+",580,540,40,32,23);
 label(g,L"FILAS CLASIFICACIÓN",694,543,230);button(g,L"−",930,540,40,32,24);text(g,std::to_wstring(cfg.rows),980,540,90,32,16,FG);button(g,L"+",1090,540,40,32,25);
 button(g,L"Segundo plano",228,608,260,44,62);button(g,L"Salir de LANDELTA",506,608,260,44,63);text(g,L"Minimizar: barra de tareas · X: segundo plano",228,678,928,26,15,MUTED);
 text(g,startupNotice,228,711,928,27,13,GOLD);
}
void renderControl(Graphics& g){
 g.Clear(BG);hits.clear();fill(g,0,0,200,780,Color(255,16,22,18));fill(g,18,25,4,35,GREEN);text(g,L"LANDELTA",32,20,161,35,24,FG,FontStyleBold);label(g,L"RACE STUDIO / 0.14.1",32,58,160,GREEN);
 const wchar_t* nav[]={L"01   Sesión",L"02   Telemetría",L"03   Tu pantalla",L"04   Ajustes"};for(int i=0;i<4;++i){button(g,nav[i],16,122+i*51,168,41,200+i,studioPage==i,studioPage==i);}
 label(g,L"MIS SESIONES",20,354,160);button(g,L"Actualizar",20,383,160,30,210);int shown=0;
 for(int i=sessionOffset;i<int(catalog.size())&&shown<5;++i,++shown){float y=429+shown*49;button(g,L"",20,y,160,42,4000+i,false,reviewing&&reviewed.id==catalog[i].id);text(g,wide(catalog[i].track),27,y+2,146,20,12,FG,FontStyleBold);text(g,catalog[i].started?historyDate(catalog[i].started):L"Fecha desconocida",27,y+22,146,18,10,MUTED);}
 if(catalog.empty())text(g,L"Aún sin sesiones",20,434,160,30,13,MUTED);
 button(g,L"‹",20,687,70,29,211);button(g,L"›",110,687,70,29,212);label(g,L"LOCAL · SIN NUBE",20,740,160);
 text(g,studioPage==2?L"Diseña tu cockpit":(studioPage==3?L"Ajustes":L"Tu centro de carrera"),228,23,390,38,29,FG,FontStyleBold);
 for(int mode=0;mode<3;++mode)button(g,gameLabel(mode),680+mode*162.f,26,152,41,240+mode,cfg.gameMode==mode,cfg.gameMode==mode);
 auto& s=currentSession();auto& r=studioRecords[cfg.gameMode];text(g,s.track.empty()?L"Entra en pista para comenzar":wide(s.track)+(s.variant.empty()?L"":L" / "+wide(s.variant)),228,88,690,35,24,FG,FontStyleBold);
 button(g,reviewing?L"Volver al directo":(r.online?L"● EN DIRECTO":L"ESPERANDO JUEGO"),940,88,216,34,213,false,!reviewing&&r.online);
 if(studioPage==0)renderSession(g);else if(studioPage==1)renderTelemetry(g);else if(studioPage==2)renderLayout(g);else renderSettings(g);
 auto notice=!r.error.empty()?wide(r.error):studioNotice; if(loadingSession)notice=L"Cargando sesión en segundo plano…";text(g,notice.empty()?L"LANDELTA  ·  Telemetría local  ·  AC / ACC / iRacing":notice,228,750,928,22,12,notice.empty()?MUTED:GOLD);
}
void exportStudioCSV(){auto& s=currentSession();if(s.samples.empty()&&s.laps.empty()){studioNotice=L"Todavía no hay datos para exportar";return;}wchar_t name[32768]=L"LANDELTA-sesion.zip";OPENFILENAMEW d{};d.lStructSize=sizeof d;d.hwndOwner=control;d.lpstrFilter=L"Sesión ZIP\0*.zip\0\0";d.lpstrFile=name;d.nMaxFile=32768;d.lpstrDefExt=L"zip";d.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;if(!GetSaveFileNameW(&d))return;try{for(auto& r:studioRecords)if(r.file.is_open())r.file.flush();race::Session exportSession=s;if(!s.path.empty())exportSession=race::loadCSV(s.path);race::exportZIP(exportSession,fs::path(name));studioNotice=L"ZIP exportado: muestras originales, vueltas y datos de sesión";}catch(const std::exception& e){studioNotice=wide(e.what());}}
bool studioAction(int id){
 if(id==202&&studioPage==2)return true; // Re-clicking this tab must not discard the draft.
 if(studioPage==2&&id>=10&&id<10+PanelCount){draftVisible[id-10]=!draftVisible[id-10];layoutDirty=true;return true;}
 if(studioPage==2&&(id==20||id==21||id==27||id==28)){if(id==20||id==21)draftScale=std::clamp(draftScale+(id==20?-.05f:.05f),.65f,1.6f);else draftDelta=std::clamp(draftDelta+(id==27?-.1f:.1f),.5f,2.5f);layoutDirty=true;return true;}
 if(studioPage==2&&layoutDirty&&((id>=200&&id<=203&&id!=202)||(id>=240&&id<=242))&&MessageBoxW(control,L"¿Descartar los cambios de distribución sin guardar?",L"LANDELTA",MB_YESNO|MB_ICONQUESTION)!=IDYES)return true;
 if(id>=4000){selectSession(id-4000);return true;}
 if(id>=300&&id<4000){auto& s=currentSession();if(id-300<int(s.laps.size())){selectedLap=s.laps[id-300].number;studioPage=1;}return true;}
 if(id>=200&&id<=203){studioPage=id-200;if(studioPage==2)beginLayout();return true;}
 if(id>=240&&id<=242){studioSwitch(id-240);return true;}
 switch(id){case 210:refreshCatalog();break;case 211:sessionOffset=std::max(0,sessionOffset-5);break;case 212:if(sessionOffset+5<int(catalog.size()))sessionOffset+=5;break;case 213:reviewRequest.cancel();studioNotice.clear();reviewing=false;selectedLap=compareLap=-1;break;case 221:lapOffset=std::max(0,lapOffset-5);selectedLap=0;break;case 222:lapOffset+=5;selectedLap=0;break;
 case 223:case 224:{auto& l=currentSession().laps;for(size_t i=0;i<l.size();++i)if(l[i].number==selectedLap){int next=std::clamp(int(i)+(id==223?-1:1),0,int(l.size())-1);selectedLap=l[next].number;break;}break;}
 case 225:{if(compareLap>=0){compareLap=-1;break;}int best=0;for(auto& l:currentSession().laps)if(l.complete&&!l.partial&&!l.pit&&l.valid!=0&&l.number!=selectedLap&&l.ms&&(!best||l.ms<best)){best=l.ms;compareLap=l.number;}if(compareLap<0)studioNotice=L"Todavía no hay otra vuelta completa para comparar";break;}
 case 228:{auto& s=currentSession();acc::SessionRecord r;r.track=s.track;r.driver=s.car;r.started=s.started;for(auto& l:s.laps){acc::RecordedLap a;a.number=l.number;a.complete=l.complete&&!l.partial;a.pit=l.pit;a.timing.ms=l.ms;a.timing.splits=l.sectors;a.timing.valid=l.valid==1;a.timing.invalid=l.valid==0;r.laps.push_back(a);}exportHistoryImage(&r);break;}
 case 226:sectorMode=!sectorMode;break;case 227:exportStudioCSV();break;case 230:beginLayout();break;case 231:for(int i=0;i<PanelCount;++i){draftX[i]=int(screenMap.left)+Config{}.x[i];draftY[i]=int(screenMap.top)+Config{}.y[i];}layoutDirty=true;break;
 case 232:cfg.scale=draftScale;cfg.deltaScale=draftDelta;for(int i=0;i<PanelCount;++i){cfg.visible[i]=draftVisible[i];auto mini=screenMap.toMini(draftX[i],draftY[i]);auto pos=screenMap.toDesktop(mini.first,mini.second,int(panelW(i)*cfg.scale*(i==4?cfg.deltaScale:1)),int(panelH(i)*cfg.scale*(i==4?cfg.deltaScale:1)));cfg.x[i]=draftX[i]=pos.first;cfg.y[i]=draftY[i]=pos.second;}locked=true;layoutDirty=false;studioNotice=L"Distribución aplicada y paneles bloqueados";break;default:return false;}return true;
}
bool studioPointer(HWND h,UINT msg,float x,float y){
 if(msg==WM_MOUSEMOVE&&studioPage==1){graphCursor=x>=410&&x<=1138&&y>=209&&y<=628?(x-410)/728.:-1;InvalidateRect(h,nullptr,FALSE);}
 if(studioPage!=2)return false;
 if(msg==WM_LBUTTONDOWN){for(auto it=layoutHits.rbegin();it!=layoutHits.rend();++it)if(it->first.Contains(x,y)){layoutDrag=it->second;dragDX=x-it->first.X;dragDY=y-it->first.Y;SetCapture(h);return true;}}
 if(msg==WM_MOUSEMOVE&&layoutDrag>=0){int i=layoutDrag;auto p=screenMap.toDesktop(x-dragDX,y-dragDY,int(panelW(i)*draftScale*(i==4?draftDelta:1)),int(panelH(i)*draftScale*(i==4?draftDelta:1)));draftX[i]=p.first;draftY[i]=p.second;layoutDirty=true;InvalidateRect(h,nullptr,FALSE);return true;}
 if(msg==WM_LBUTTONUP&&layoutDrag>=0){layoutDrag=-1;ReleaseCapture();return true;}return false;
}

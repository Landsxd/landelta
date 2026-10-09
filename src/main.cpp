#ifndef UNICODE
#define UNICODE
#endif
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <shlobj.h>
#include <shellapi.h>
#include <commdlg.h>
#include <tlhelp32.h>
#include <gdiplus.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include "runtime.hpp"
#include "relatives.hpp"
#include "driving.hpp"
#include "background.hpp"
#include "startup.hpp"
#include "config_codec.hpp"
#include "history.hpp"
#include <ctime>
#include <nlohmann/json.hpp>
using namespace Gdiplus;
using json=nlohmann::json;
namespace fs=std::filesystem;

const Color BG(255,13,18,24), CARD(255,22,30,39), EDGE(255,43,55,67), FG(255,255,255,255), MUTED(255,206,220,232), GREEN(255,0,255,156), RED(255,255,69,99), PURPLE(255,214,140,255), GOLD(255,255,207,0), BLUE(255,0,195,255);
std::wstring wide(const std::string& s){if(s.empty())return L"";int n=MultiByteToWideChar(CP_UTF8,0,s.data(),(int)s.size(),nullptr,0);std::wstring w(n,0);MultiByteToWideChar(CP_UTF8,0,s.data(),(int)s.size(),w.data(),n);return w;}
std::string utf8(const std::wstring& s){if(s.empty())return "";int n=WideCharToMultiByte(CP_UTF8,0,s.data(),(int)s.size(),nullptr,0,nullptr,nullptr);std::string a(n,0);WideCharToMultiByte(CP_UTF8,0,s.data(),(int)s.size(),a.data(),n,nullptr,nullptr);return a;}
std::wstring num(double n,int precision=1,bool sign=false){if(!std::isfinite(n))return L"--";std::wostringstream s;s.setf(std::ios::fixed);s.precision(precision);if(sign && n>=0)s<<L"+";s<<n;return s.str();}
std::wstring lapTime(int ms){return wide(acc::formatTimeMs(ms));}
std::wstring sectorTime(int ms){return lapTime(ms);}
std::wstring clockTime(float ms){if(!std::isfinite(ms)||ms<0)return L"--:--";int sec=(int)(ms/1000);wchar_t b[32];swprintf(b,32,L"%02d:%02d",sec/60,sec%60);return b;}
std::wstring sessionName(int t){switch(t){case 4:return L"CLASIFICACIÓN";case 9:return L"SUPERPOLE";case 10:return L"CARRERA";case 11:return L"HOTLAP";case 12:return L"HOTSTINT";case 14:return L"REPLAY";default:return L"PRÁCTICA";}}
bool fileWrite(const fs::path& p,const std::string& data){std::ofstream f(p,std::ios::binary|std::ios::trunc);f.write(data.data(),data.size());f.close();return !f.fail();}
std::string fileRead(const fs::path& p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("No se pudo leer el archivo");std::string s((std::istreambuf_iterator<char>(f)),{});if(s.size()>1048576)throw std::runtime_error("Archivo de configuración demasiado grande");return s;}
constexpr int PanelCount=7;
struct Config {float scale=1,deltaScale=1;int opacity=64,rows=8;bool autoShow=true,keepAfterRace=false;bool visible[PanelCount]={true,true,true,true,true,true,false},hidden=false;int x[PanelCount]={25,650,25,285,650,285,650},y[PanelCount]={90,90,470,470,335,580,425};};
bool renderingOverlay=false;
Config cfg;acc::State live,demo;acc::Graphics shared{};bool sharedOK=false,demoMode=false,locked=false,gameRunning=false,configReady=false,autoConfigured=false;
HWND control=nullptr,panels[PanelCount]{};SOCKET sock=INVALID_SOCKET;HANDLE mapping=nullptr;const void* view=nullptr;int port=9000,page=0;std::string password;std::wstring status=L"Buscando ACC…",configStatus=L"Configuración pendiente",networkError,lastReset=L"Inicio";fs::path dataDir,configPath,settingsPath;uint64_t lastCheck=0,lastEntry=0,lastConfigCheck=0,connectedAt=0;DWORD gamePID=0;int networkReconnects=0,sessionResets=0,rejectedPackets=0;acc::ReconnectPolicy retry;acc::PlayerTiming playerTiming;bool sharedOffline=false;ULONG_PTR gdip=0;bool configAttempted=false;int scroll=-1;
std::vector<std::pair<RectF,int>> hits;
acc::SessionVisibility sessionVisibility;acc::SessionLaunch sessionLaunch;acc::LapDelta lapDelta;acc::DeltaSnapshot retainedDelta;acc::StartupInfo startupInfo;fs::path executablePath,startupPath;
std::wstring startupNotice,hotkeyIssue;bool trayPresent=false,trayV4=false,backgroundLaunch=false;UINT taskbarCreated=0;
constexpr UINT TrayMessage=WM_APP+20;unsigned mainInterval=0;bool pedalTimer=false;uint64_t lastTrayCheck=0;
void adjustPolling();bool hideControl();

HANDLE inputMapping=nullptr;const void* inputView=nullptr;acc::PedalState pedals;uint64_t inputRetryAt=0;
acc::Analytics analytics,demoAnalytics;acc::HistoryStore historyStore;bool analyticsTrackReady=false;uint64_t gridWaitAt=0,lastCarPacket=0;
std::array<uint64_t,9> udpReceived{},udpAccepted{},udpRejected{};uint64_t gatedCars=0;std::wstring protocolError;int gridRecoveries=0;
uint64_t udpGeneration=0;uint64_t lastHistoryFlush=0;int historyView=0,historyOffset=0,historyTotal=0,historyPageSize=4;
std::string historyTrack,historySelected;std::vector<std::string> historyTargets;
int64_t wallMs(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();}
void captureAnalytics(int kind,unsigned packetType,const char* packet,int size,uint64_t now){
 if(kind==12)analyticsTrackReady=false;
 if(kind>0&&packetType==5)analyticsTrackReady=true;
 if(demoMode||!analyticsTrackReady||live.replay)return;
 if(kind==3&&size>=3){int id=(uint8_t)packet[1]|((uint8_t)packet[2]<<8);int player=sharedOK&&(shared.status==2||shared.status==3)?shared.player:-1;analytics.observe(live,id,player,now,wallMs());}
}

uint64_t nowMs(){return GetTickCount64();}

DWORD accProcess(){HANDLE h=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(h==INVALID_HANDLE_VALUE)return gamePID;PROCESSENTRY32W e{};e.dwSize=sizeof e;DWORD pid=0;if(Process32FirstW(h,&e))do{if(!_wcsicmp(e.szExeFile,L"AC2-Win64-Shipping.exe")){pid=e.th32ProcessID;break;}if(!_wcsicmp(e.szExeFile,L"ACC.exe"))pid=e.th32ProcessID;}while(Process32NextW(h,&e));CloseHandle(h);return pid;}
fs::path knownFolder(REFKNOWNFOLDERID id){PWSTR p=nullptr;if(FAILED(SHGetKnownFolderPath(id,0,nullptr,&p)))return {};fs::path f=p;CoTaskMemFree(p);return f;}
void locateStartup(){wchar_t exe[32768]{};DWORD n=GetModuleFileNameW(nullptr,exe,32768);if(n&&n<32768)executablePath=fs::path(exe);auto folder=knownFolder(FOLDERID_Startup);if(!folder.empty()){startupPath=folder/L"LANDELTA-Auto.lnk";auto old=folder/L"VortexACC-Auto.lnk";if(!fs::exists(startupPath)&&fs::is_regular_file(old))startupPath=old;}}
void refreshStartup(){startupInfo=acc::inspectStartup(startupPath,executablePath);}
void setStartup(bool enable){std::wstring error;auto target=enable&&!startupPath.empty()?startupPath.parent_path()/L"LANDELTA-Auto.lnk":startupPath;if(acc::writeStartup(target,executablePath,enable,error)){if(enable&&target!=startupPath){std::wstring cleanup;acc::writeStartup(startupPath,executablePath,false,cleanup);}startupPath=target;startupNotice=enable?L"Listo: iniciará oculto al entrar a Windows.":L"Inicio con Windows desactivado.";}else startupNotice=error;refreshStartup();}
std::vector<fs::path> documentCandidates(){std::vector<fs::path> dirs;auto k=knownFolder(FOLDERID_Documents);if(!k.empty())dirs.push_back(k);wchar_t b[32768];for(auto env:{L"USERPROFILE",L"OneDrive",L"OneDriveConsumer",L"OneDriveCommercial"}){DWORD n=GetEnvironmentVariableW(env,b,32768);if(n&&n<32768)dirs.push_back(fs::path(b)/L"Documents");}return dirs;}
void locateACC(){if(!configPath.empty())return;for(auto& d:documentCandidates()){auto p=d/L"Assetto Corsa Competizione"/L"Config";if(fs::is_directory(p)){configPath=p/L"broadcasting.json";return;}}}
std::string decodeConfig(const std::string& s){return acc::decodeConfigBytes(s);}
fs::path lastValidConfigPath;std::string configReadIssue;
std::string encodeConfig(const std::string& s){auto w=wide(s);std::string b="\xff\xfe";for(auto c:w){b.push_back(c&255);b.push_back((c>>8)&255);}return b;}
void configureACC(bool explicitClick=false){try{locateACC();if(configPath.empty()){configStatus=L"No se encontró la carpeta de ACC. Usa «Elegir carpeta».";configReady=false;return;}
 json j=json::object();bool exists=fs::exists(configPath);if(!exists&&gameRunning&&configReady&&lastValidConfigPath==configPath)throw std::runtime_error("Configuración en escritura");if(exists)j=json::parse(decodeConfig(fileRead(configPath)));if(!j.is_object())throw std::runtime_error("broadcasting.json debe ser un objeto JSON");
 int p=0;if(j.contains("updListenerPort")){if(!j["updListenerPort"].is_number_integer())throw std::runtime_error("Puerto inválido en broadcasting.json");p=j["updListenerPort"].get<int>();}
 if(j.contains("connectionPassword")&&!j["connectionPassword"].is_string())throw std::runtime_error("Contraseña inválida en broadcasting.json");
 if(p>0&&p<=65535){port=p;password=j.value("connectionPassword",std::string());configReady=true;lastValidConfigPath=configPath;configReadIssue.clear();configStatus=L"ACC preparado · puerto "+std::to_wstring(port);return;}
 if(gameRunning){configReady=false;configStatus=L"Cierra ACC una vez para activar la conexión automática.";return;}
 if(!exists && !explicitClick && !fs::is_directory(configPath.parent_path()))return;
 j["updListenerPort"]=9000;if(!j.contains("connectionPassword"))j["connectionPassword"]="vortex-local";if(!j.contains("commandPassword"))j["commandPassword"]="";
 if(exists){auto backup=configPath;backup+=L".vortex-"+std::to_wstring(nowMs())+L".bak";fs::copy_file(configPath,backup);}
 auto tmp=configPath;tmp+=L".vortex.tmp";if(!fileWrite(tmp,encodeConfig(j.dump(2))))throw std::runtime_error("No se pudo escribir la configuración");if(!MoveFileExW(tmp.c_str(),configPath.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("No se pudo activar la configuración");
 port=9000;password=j.value("connectionPassword",std::string());configReady=true;lastValidConfigPath=configPath;configReadIssue.clear();autoConfigured=true;configStatus=L"Conexión activada. Ya puedes abrir ACC.";
 }catch(const std::exception& e){auto parse=dynamic_cast<const json::parse_error*>(&e);configReadIssue=parse?"JSON incompleto o inválido en byte "+std::to_string(parse->byte):"Archivo inaccesible, incompleto o codificación no compatible";configReady=acc::retainConfigOnReadError(gameRunning,configReady,lastValidConfigPath==configPath);configStatus=configReady?L"Releyendo configuración · se mantiene la conexión anterior":L"No se pudo leer broadcasting.json. Reintento automático en 5 s.";}}
void saveSettings(){try{json j={{"schema",6},{"deltaScale",cfg.deltaScale},{"keepAfterRace",cfg.keepAfterRace},{"autoShowOnSession",cfg.autoShow},{"scale",cfg.scale},{"backgroundOpacity",cfg.opacity},{"rows",cfg.rows},{"accConfig",utf8(configPath.wstring())}};for(int i=0;i<PanelCount;i++)j["panels"][i]={{"x",cfg.x[i]},{"y",cfg.y[i]},{"visible",cfg.visible[i]}};fileWrite(settingsPath,j.dump(2));}catch(...) {}}
void loadSettings(){try{if(!fs::exists(settingsPath))return;auto j=json::parse(fileRead(settingsPath));cfg.deltaScale=std::clamp(j.value("deltaScale",1.f),.5f,2.5f);cfg.keepAfterRace=j.value("keepAfterRace",false);cfg.autoShow=j.value("autoShowOnSession",true);cfg.scale=std::clamp(j.value("scale",1.f),.65f,1.6f);cfg.opacity=std::clamp(j.value("backgroundOpacity",64),0,255);cfg.rows=std::clamp(j.value("rows",8),4,30);auto path=j.value("accConfig",std::string());if(!path.empty()&&fs::exists(fs::path(wide(path)).parent_path()))configPath=fs::path(wide(path));if(j.contains("panels")&&j["panels"].is_array())for(int i=0;i<PanelCount&&i<int(j["panels"].size());i++){auto p=j["panels"][i];cfg.x[i]=p.value("x",cfg.x[i]);cfg.y[i]=p.value("y",cfg.y[i]);cfg.visible[i]=p.value("visible",cfg.visible[i]);}if(j.value("schema",1)<4){if(cfg.rows==12)cfg.rows=8;if((cfg.x[1]==720||cfg.x[1]==850)&&cfg.y[1]==90){cfg.x[1]=650;}if((cfg.x[2]==720||cfg.x[2]==850)&&(cfg.y[2]==450||cfg.y[2]==580||cfg.y[2]==700)){cfg.x[2]=25;cfg.y[2]=470;}}}catch(...) {}}
void closeInputs(){if(inputView)UnmapViewOfFile(inputView);if(inputMapping)CloseHandle(inputMapping);inputView=nullptr;inputMapping=nullptr;pedals.reset();inputRetryAt=0;}
void pollInputs(uint64_t now){
 if(!gameRunning||!sharedOK||shared.status!=2||live.replay||demoMode){if(inputView||pedals.have)closeInputs();return;}
 if(!inputView){if(now<inputRetryAt)return;inputRetryAt=now+1000;inputMapping=OpenFileMappingW(FILE_MAP_READ,FALSE,L"Local\\acpmf_physics");if(!inputMapping)inputMapping=OpenFileMappingW(FILE_MAP_READ,FALSE,L"acpmf_physics");if(inputMapping){inputView=MapViewOfFile(inputMapping,FILE_MAP_READ,0,0,sizeof(acc::PhysicsInput));if(!inputView){CloseHandle(inputMapping);inputMapping=nullptr;}}}
 if(!inputView)return;acc::PhysicsInput p{};bool coherent=false;for(int i=0;i<3;i++){int before;memcpy(&before,inputView,4);MemoryBarrier();memcpy(&p,inputView,sizeof p);MemoryBarrier();int after;memcpy(&after,inputView,4);if(before==after&&p.packet==before){coherent=true;break;}}if(!coherent){pedals.valid=false;return;}pedals.update(p,now);
 if(pedals.expired(now)){int packet=pedals.value.packet;closeInputs();pedals.waitForPacket(packet);inputRetryAt=now+250;}
}
void resetPlayerTiming(){closeInputs();playerTiming.waitForPacket(shared.packet);sharedOK=false;shared={};}
void closeMapping(){lapDelta.interrupt();closeInputs();if(view)UnmapViewOfFile(view);if(mapping)CloseHandle(mapping);view=nullptr;mapping=nullptr;sharedOK=false;shared={};playerTiming.reset();sharedOffline=false;}
bool sendPacket(const acc::Writer& w){return sock!=INVALID_SOCKET&&::send(sock,(const char*)w.b.data(),(int)w.b.size(),0)==(int)w.b.size();}
void closeSocket(){if(sock!=INVALID_SOCKET){if(live.registered){acc::Writer w;w.put<uint8_t>(9);sendPacket(w);}closesocket(sock);}sock=INVALID_SOCKET;live.registered=false;}
void reconnect(){analytics.disconnect();analyticsTrackReady=false;closeSocket();live={};retry={};lastEntry=0;connectedAt=0;gridWaitAt=lastCarPacket=0;scroll=-1;networkError.clear();resetPlayerTiming();++networkReconnects;}
void failUDP(uint64_t now,const std::wstring& reason){analytics.disconnect();analyticsTrackReady=false;closeSocket();live={};lastEntry=0;connectedAt=0;gridWaitAt=lastCarPacket=0;networkError=reason;retry.failed(now);++networkReconnects;}
bool openSocket(){sock=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);if(sock==INVALID_SOCKET)return false;u_long nonblock=1;if(ioctlsocket(sock,FIONBIO,&nonblock)==SOCKET_ERROR){closeSocket();return false;}int size=1048576;setsockopt(sock,SOL_SOCKET,SO_RCVBUF,(char*)&size,sizeof size);sockaddr_in a{};a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);a.sin_port=htons((u_short)port);if(connect(sock,(sockaddr*)&a,sizeof a)==SOCKET_ERROR){closeSocket();return false;}return true;}
void pollShared(uint64_t now){
 if(!gameRunning){if(view)closeMapping();return;}
 if(!view){mapping=OpenFileMappingW(FILE_MAP_READ,FALSE,L"Local\\acpmf_graphics");if(!mapping)mapping=OpenFileMappingW(FILE_MAP_READ,FALSE,L"acpmf_graphics");if(mapping){view=MapViewOfFile(mapping,FILE_MAP_READ,0,0,sizeof(acc::Graphics));if(!view)closeMapping();}}
 if(!view)return;acc::Graphics g{};bool coherent=false;for(int i=0;i<3;i++){int before;memcpy(&before,view,4);MemoryBarrier();memcpy(&g,view,sizeof g);MemoryBarrier();int after;memcpy(&after,view,4);if(before==after&&g.packet==before){coherent=true;break;}}if(!coherent){sharedOK=false;return;}
 bool wasOffline=sharedOffline;sharedOffline=g.status==0;
 if(sharedOffline){lapDelta.reset();if(!wasOffline){analytics.endSession();reconnect();lastReset=L"Salida de la sesión / menú";++sessionResets;}playerTiming.reset();shared={};sharedOK=false;return;}
 int st=acc::broadcastSession(g.session);if(g.status!=1&&live.hasSession&&acc::fresh(live.sessionReceived,now)&&st>=0&&st!=live.type&&!(st==4&&live.type==9)){sharedOK=false;return;}
 auto change=playerTiming.update(g,now);sharedOK=playerTiming.valid;shared=sharedOK?g:acc::Graphics{};
 if(change==acc::SharedChange::Session){analytics.endSession();auto current=playerTiming;reconnect();playerTiming=current;sharedOK=current.valid;shared=sharedOK?current.g:acc::Graphics{};lastReset=L"Cambio de sesión detectado en ACC";++sessionResets;}
 lapDelta.observe(playerTiming,now);
 auto delta=lapDelta.view(playerTiming,now);if(!delta.available)delta=acc::drivingDelta(playerTiming,now);retainedDelta.observe(shared,live.epoch,delta,sharedOK&&shared.status==2&&!live.replay);
 if(playerTiming.expired(now)){int packet=playerTiming.g.packet;closeMapping();playerTiming.waitForPacket(packet);} // Release stale handles; reject the same frozen snapshot after reopening.
}
void requestMetadata(uint64_t now){if(live.registered){sendPacket(acc::command(10,live.connection));sendPacket(acc::command(11,live.connection));lastEntry=now;}}
void pollUDP(uint64_t now){
 if(!gameRunning||!configReady||sharedOffline)return;
 if(sock==INVALID_SOCKET){if(!retry.due(now))return;retry.begin(now);if(!openSocket()||!sendPacket(acc::registration(password))){failUDP(now,L"No se pudo abrir UDP local; reintentando");return;}}
 // Drain queued data before checking deadlines, avoiding false reconnects under load.
 char b[65536];for(int i=0;i<512;i++){int n=recv(sock,b,sizeof b,0);if(n==SOCKET_ERROR){int e=WSAGetLastError();if(e!=WSAEWOULDBLOCK)failUDP(now,L"UDP "+std::to_wstring(e)+L"; reintentando");break;}if(!n)continue;
  unsigned type=(uint8_t)b[0];if(type<udpReceived.size())++udpReceived[type];if(type!=1&&!live.registered)continue;if(type==3&&(!live.hasSession||!acc::fresh(live.sessionReceived,now,8000))){++gatedCars;continue;}
  bool hadSession=live.hasSession;int kind=live.parse((uint8_t*)b,n,now);if(kind<0){++rejectedPackets;if(type<udpRejected.size())++udpRejected[type];protocolError=L"Paquete "+std::to_wstring(type)+L": "+wide(live.parseError);continue;}if(kind>0&&type<udpAccepted.size())++udpAccepted[type];if(kind==3){lastCarPacket=now;networkError.clear();}
  if(kind==1){if(!live.registered){auto reason=wide(live.error);failUDP(now,L"ACC rechazó la conexión: "+reason);break;}retry.connected();connectedAt=gridWaitAt=now;lastCarPacket=0;live.received=now;networkError.clear();requestMetadata(now);}
  if(kind==12){if(hadSession){lapDelta.reset();++udpGeneration;}gridWaitAt=now;lastCarPacket=0;resetPlayerTiming();scroll=-1;lastReset=wide(live.resetReason);++sessionResets;requestMetadata(now);}
  captureAnalytics(kind,type,b,n,now);
 }
 if(sock==INVALID_SOCKET)return;
 if(retry.timedOut(now)){failUDP(now,L"ACC no respondió; reintentando");return;}
 if(live.registered&&!(sharedOK&&shared.status==3)&&!acc::fresh(live.sessionReceived?live.sessionReceived:connectedAt,now,8000)){failUDP(now,L"Se perdió la sesión UDP; reintentando");return;}
 if(acc::gridTimedOut(now,gridWaitAt,lastCarPacket,live.registered,live.hasSession,sharedOK&&shared.status==3)){++gridRecoveries;failUDP(now,L"ACC envía la sesión pero no los pilotos; recuperando parrilla");return;}
 bool missing=live.active(now,sharedOK&&shared.status==3).empty()||live.track.empty()||live.cars.empty();for(const auto& kv:live.cars)if(kv.second.drivers.empty())missing=true;
 if(live.registered&&now-lastEntry>uint64_t(missing?1500:15000))requestMetadata(now);
}
void generateDemo(uint64_t now){demo.registered=true;demo.track="MONZA";demo.trackId=1;demo.hasSession=true;demo.type=10;demo.phase=5;demo.focus=4;demo.meters=5793;demo.elapsed=now%1500000;demo.end=1800000;double progress=14.58+(now%108900)/108900.;const char* names[]={"M. Verstappen","A. Rossi","D. Moreno","C. Fuentes","L. Landeros","S. Martínez","P. García","R. Silva","J. Navarro","A. Torres","F. López","N. Bianchi","K. Sato","E. Wilson","T. Oliveira","M. Costa","R. Álvarez","A. Dubois"};for(int i=0;i<18;i++){auto& c=demo.cars[i];c.id=i;c.number=7+i*6;c.position=i+1;c.driver=0;c.drivers={names[i]};double p=progress+(4-i)*.029+.001*std::sin(now/5000.+i);if(i==9)p=progress-1+.017;if(i==2)p=progress+1-.018;c.laps=(int)p;c.spline=(float)(p-std::floor(p));c.location=i==13?2:1;c.speed=210+(int)(35*std::sin(now/1900.+i));c.best.ms=108520+i*210;c.best.valid=true;c.best.splits={35470+i*30,36320+i*80,36730+i*100};c.last.ms=c.best.ms+360;c.last.valid=true;c.last.splits={c.best.splits[0]+80,c.best.splits[1]+140,c.best.splits[2]+140};c.current.ms=(int)(c.spline*108900);c.current.splits={c.current.ms>35400?35400:0,c.current.ms>71800?36400:0,0};c.delta=-170;c.seen=now;c.penaltyNoticeAt=i==3?now:0;c.penaltyMessage=i==3?"Aviso simulado":"";}demo.sessionBest=demo.cars[0].best;
 for(auto& kv:demo.cars){auto& t=demoAnalytics.trackers[kv.first];t.lap=kv.second.laps;t.recent.clear();for(int k=0;k<5;k++){acc::RecordedLap l;l.number=t.lap-k;l.timing.valid=true;l.timing.ms=kv.first==4?109200+(k-2)*80:(kv.first==3?109850+(k-2)*90:109570+(k-2)*70);l.complete=true;t.recent.push_back(l);}}
 if(demoAnalytics.history.empty()){for(int circuit=0;circuit<2;circuit++){acc::SessionRecord r;r.id=std::to_string(1791309600000LL+circuit);r.started=1791309600000LL+circuit*86400000LL;r.track=circuit?"SPA FRANCORCHAMPS":"MONZA";r.trackId=circuit?7:1;r.type=10;r.number=31;r.driver="L. Landeros";for(int k=1;k<=9;k++){acc::RecordedLap l;l.number=k;l.driver=r.driver;l.complete=true;l.timing.valid=true;l.timing.ms=(circuit?139500:109200)+(9-k)*80;l.timing.splits={35500+k*8,36300-k*34,l.timing.ms-(35500+k*8)-(36300-k*34)};if(k==7)l.timing.invalid=true;r.laps.push_back(l);}demoAnalytics.history.push_back(r);}}
 demoAnalytics.active=true;demoAnalytics.record=0;demoAnalytics.recordPlayer=4;
}
acc::State& display(){return demoMode?demo:live;}
bool paused(){return !demoMode&&sharedOK&&shared.status==3;}
int selectedID(){return demoMode?4:(!live.replay&&sharedOK?shared.player:live.focus);}
acc::Car* selectedCar(){auto& s=display();auto it=s.cars.find(selectedID());if(it==s.cars.end())return nullptr;if(!demoMode&&!paused()&&!acc::fresh(it->second.seen,nowMs()))return nullptr;return &it->second;}
void updateStatus(uint64_t now){
 if(demoMode){status=L"DEMO · datos simulados";return;}
 if(!gameRunning){status=L"Esperando ACC · conexión automática";return;}
 if(sharedOffline){status=L"ACC en el menú · esperando sesión";return;}
 if(!configReady){status=sharedOK?L"Tiempos activos · falta configurar pilotos":L"Preparar conexión de ACC";return;}
 if(paused()){status=L"ACC en pausa · tiempos detenidos";return;}
 auto cars=live.active(now);
 if(live.registered&&acc::fresh(live.sessionReceived,now)&&!cars.empty()){status=(live.replay?L"ACC replay · ":L"ACC conectado · ")+std::to_wstring(cars.size())+L" pilotos";return;}
 if(!networkError.empty()){status=networkError;return;}
 if(live.registered){status=L"UDP registrado · esperando pilotos de ACC";return;}
 if(sharedOK){status=L"Tiempos activos · conectando pilotos por UDP";return;}
 status=retry.awaiting?L"Conectando con ACC…":L"ACC detectado · entra en una sesión";
}
std::wstring pilotsStatus(){
 if(!configReady)return L"Pilotos UDP: falta configurar ACC";
 if(!live.registered)return L"Pilotos UDP: sin registro / reconectando";
 auto n=live.active(nowMs(),paused()).size();return n?L"Pilotos UDP: "+std::to_wstring(n)+L" recibidos":L"Pilotos UDP: registrado, sin parrilla";
}

acc::Analytics& displayAnalytics(){return demoMode?demoAnalytics:analytics;}
std::string trackKey(const acc::SessionRecord& r){return std::to_string(r.trackId)+"|"+r.track;}
struct HistoryTrack {std::string key,name;int sessions=0,best=0;};
std::vector<HistoryTrack> historyTracks(){std::vector<HistoryTrack> result;for(auto it=displayAnalytics().history.rbegin();it!=displayAnalytics().history.rend();++it){auto key=trackKey(*it);auto item=std::find_if(result.begin(),result.end(),[&](const HistoryTrack& t){return t.key==key;});if(item==result.end()){result.push_back({key,it->track,0,0});item=std::prev(result.end());}++item->sessions;int b=it->best();if(b)item->best=item->best?std::min(item->best,b):b;}return result;}
std::wstring historyDate(int64_t ms){std::time_t seconds=ms/1000;std::tm* tm=std::localtime(&seconds);if(!tm)return L"Fecha desconocida";wchar_t b[64];std::wcsftime(b,64,L"%d/%m/%Y  %H:%M",tm);return b;}
void fill(Graphics& g,float x,float y,float w,float h,Color c){if(renderingOverlay&&w>5&&h>5)c=Color((BYTE)cfg.opacity,c.GetR(),c.GetG(),c.GetB());SolidBrush b(c);g.FillRectangle(&b,x,y,w,h);}
void text(Graphics& g,const std::wstring& s,float x,float y,float w,float h,float size=14,Color color=FG,int weight=FontStyleRegular,StringAlignment align=StringAlignmentNear){FontFamily family(L"Segoe UI");Font f(&family,size,weight,UnitPixel);SolidBrush b(color);StringFormat sf;sf.SetAlignment(align);sf.SetLineAlignment(StringAlignmentCenter);sf.SetTrimming(StringTrimmingEllipsisCharacter);sf.SetFormatFlags(StringFormatFlagsNoWrap);if(renderingOverlay){SolidBrush shadow(Color(230,0,0,0));g.DrawString(s.c_str(),(int)s.size(),&f,RectF(x+1,y+1,w,h),&sf,&shadow);}g.DrawString(s.c_str(),(int)s.size(),&f,RectF(x,y,w,h),&sf,&b);}
void line(Graphics& g,float x,float y,float w,Color c=EDGE){Pen p(c,1);g.DrawLine(&p,x,y,x+w,y);}
void tag(Graphics& g,const std::wstring& s,float x,float y,float w,Color c=GREEN){fill(g,x,y,w,23,Color(255,31,52,47));text(g,s,x,y,w,23,11,c,FontStyleBold,StringAlignmentCenter);}
void button(Graphics& g,const std::wstring& label,float x,float y,float w,float h,int id,bool accent=false,bool active=false){fill(g,x,y,w,h,accent?GREEN:(active?Color(255,33,67,57):CARD));Pen p(active?GREEN:EDGE);g.DrawRectangle(&p,x,y,w,h);text(g,label,x+4,y,w-8,h,13,accent?BG:(active?GREEN:FG),FontStyleBold,StringAlignmentCenter);hits.emplace_back(RectF(x,y,w,h),id);}
void section(Graphics& g,const std::wstring& label,float y){text(g,label,28,y,600,25,11,MUTED,FontStyleBold);}

void renderHistoryImage(Graphics& g,const acc::SessionRecord& record,size_t page){
 const size_t first=page*40,pages=(record.laps.size()+39)/40,n=first<record.laps.size()?std::min(size_t(40),record.laps.size()-first):0;
 int best=0;for(const auto& lap:record.laps)if(lap.clean()&&(!best||lap.timing.ms<best))best=lap.timing.ms;
    fill(g,36,30,7,48,GREEN);text(g,L"LANDELTA / TIEMPOS",60,24,1100,52,30,FG,FontStyleBold);
    text(g,wide(record.track),36,90,1128,44,28,GREEN,FontStyleBold);
    text(g,wide(record.driver)+L" · #"+std::to_wstring(record.number)+L" · "+sessionName(record.type),36,138,1128,30,18,FG);
    text(g,historyDate(record.started)+L" · Mejor limpia "+lapTime(best)+L" · "+std::to_wstring(page+1)+L" / "+std::to_wstring(pages)+(demoMode?L" · DATOS SIMULADOS":L""),36,176,1128,28,16,MUTED);
    const float x[]={36,140,355,555,755,955};const float w[]={90,200,185,185,185,209};const wchar_t* titles[]={L"V",L"VUELTA",L"S1",L"S2",L"S3",L"ESTADO"};
    for(int col=0;col<6;col++)text(g,titles[col],x[col],218,w[col],28,16,MUTED,FontStyleBold);
    for(size_t row=0;row<n;row++){const auto& lap=record.laps[first+row];float y=250+row*42;fill(g,30,y,1140,40,row%2?BG:CARD);Color c=lap.clean()?(lap.timing.ms==best?GREEN:FG):GOLD;
     text(g,std::to_wstring(lap.number),x[0],y,w[0],40,20,FG,FontStyleBold);text(g,lapTime(lap.timing.ms),x[1],y,w[1],40,23,c,FontStyleBold);
     for(int sector=0;sector<3;sector++)text(g,sectorTime(lap.timing.splits[sector]),x[sector+2],y,w[sector+2],40,21,FG);
     auto state=lap.timing.invalid||!lap.timing.valid?L"INVÁLIDA":(lap.pit||lap.timing.in||lap.timing.out?L"BOXES":(lap.complete?L"LIMPIA":L"PARCIAL"));text(g,state,x[5],y,w[5],40,18,c,FontStyleBold);
    }}

void renderHistory(Graphics& g){
 historyTargets.clear();auto& records=displayAnalytics().history;
 if(historyView)button(g,L"← Volver",28,238,102,32,50);
 if(historyView==2)button(g,L"Exportar PNG",493,238,135,32,53);
 text(g,demoMode?L"HISTORIAL DEMO":L"TU HISTORIAL",historyView?144:28,238,historyView==2?340:360,31,17,FG,FontStyleBold);
 text(g,demoMode?L"Ejemplos simulados · no se guardan":(historyStore.error.empty()?L"Guardado automático en este equipo":wide(historyStore.error)),28,275,600,26,12,historyStore.error.empty()||demoMode?MUTED:GOLD);
 historyTotal=0;historyPageSize=4;
 if(records.empty()){text(g,L"Tu próxima vuelta empieza el historial",28,346,600,38,22,FG,FontStyleBold);text(g,L"ACC identifica el circuito; guardamos tus vueltas y sectores.",28,398,600,28,14,MUTED);text(g,L"Después podrás revisar cada sesión desde esta pestaña.",28,432,600,28,14,MUTED);text(g,live.track.empty()?L"Esperando un circuito de ACC…":L"Circuito detectado: "+wide(live.track),28,488,600,32,16,GREEN);return;}
 if(historyView==0){auto tracks=historyTracks();historyTotal=(int)tracks.size();historyOffset=std::clamp(historyOffset,0,std::max(0,((historyTotal-1)/4)*4));
  for(int i=0;i<4&&historyOffset+i<historyTotal;i++){const auto& t=tracks[historyOffset+i];float y=314.f+i*62;fill(g,28,y,600,55,CARD);text(g,wide(t.name),42,y+3,320,27,18,FG,FontStyleBold);text(g,std::to_wstring(t.sessions)+L" sesiones  ·  mejor válida",42,y+30,285,19,12,MUTED);text(g,lapTime(t.best),342,y+3,145,47,19,GREEN,FontStyleBold,StringAlignmentFar);button(g,L"Ver →",507,y+10,108,34,100+i);historyTargets.push_back(t.key);}
 }else if(historyView==1){std::vector<const acc::SessionRecord*> sessions;for(auto it=records.rbegin();it!=records.rend();++it)if(trackKey(*it)==historyTrack)sessions.push_back(&*it);historyPageSize=5;historyTotal=(int)sessions.size();historyOffset=std::clamp(historyOffset,0,std::max(0,((historyTotal-1)/5)*5));
  if(!sessions.empty())text(g,wide(sessions[0]->track),28,300,600,27,16,GREEN,FontStyleBold);
  for(int i=0;i<5&&historyOffset+i<historyTotal;i++){const auto& r=*sessions[historyOffset+i];float y=335.f+i*44;fill(g,28,y,600,40,CARD);text(g,historyDate(r.started),40,y,187,40,13,FG,FontStyleBold);text(g,sessionName(r.type)+L" · "+std::to_wstring(r.laps.size())+L" V",232,y,156,40,11,MUTED);text(g,lapTime(r.best()),391,y,115,40,15,GREEN,FontStyleBold,StringAlignmentFar);button(g,L"Ver",524,y+5,90,30,100+i);historyTargets.push_back(r.id);}
 }else{auto it=std::find_if(records.begin(),records.end(),[](const acc::SessionRecord& r){return r.id==historySelected;});if(it==records.end()){historyView=historyOffset=0;return;}const auto& r=*it;historyPageSize=6;historyTotal=(int)r.laps.size();historyOffset=std::clamp(historyOffset,0,std::max(0,((historyTotal-1)/6)*6));
  text(g,wide(r.track)+L" · #"+std::to_wstring(r.number)+L" · "+wide(r.driver),28,301,600,28,17,GREEN,FontStyleBold);text(g,historyDate(r.started)+L"  /  "+sessionName(r.type)+L"  /  Mejor "+lapTime(r.best()),28,331,600,23,12,MUTED);
  text(g,L"V",36,362,32,24,11,MUTED,FontStyleBold);text(g,L"VUELTA",79,362,116,24,11,MUTED,FontStyleBold);for(int k=0;k<3;k++)text(g,L"S"+std::to_wstring(k+1),207.f+105*k,362,101,24,11,MUTED,FontStyleBold);text(g,L"ESTADO",528,362,100,24,11,MUTED,FontStyleBold);
  for(int i=0;i<6&&historyOffset+i<historyTotal;i++){const auto& l=r.laps[r.laps.size()-1-historyOffset-i];float y=391.f+i*28;fill(g,28,y,600,26,i%2?BG:CARD);text(g,std::to_wstring(l.number),36,y,32,26,12,FG,FontStyleBold);text(g,lapTime(l.timing.ms),79,y,116,26,14,l.timing.invalid?RED:FG,FontStyleBold);for(int k=0;k<3;k++)text(g,sectorTime(l.timing.splits[k]),207.f+105*k,y,101,26,13,MUTED);std::wstring label=l.timing.invalid||!l.timing.valid?L"INVÁLIDA":(l.pit||l.timing.in||l.timing.out?L"BOXES":(l.complete?L"LIMPIA":L"PARCIAL"));text(g,label,528,y,94,26,10,l.clean()?GREEN:GOLD,FontStyleBold);}
 }
 button(g,L"←",28,574,49,31,51);text(g,historyTotal?std::to_wstring(historyOffset+1)+L"–"+std::to_wstring(std::min(historyTotal,historyOffset+historyPageSize))+L" de "+std::to_wstring(historyTotal):L"Sin sesiones",84,574,450,31,12,MUTED,FontStyleRegular,StringAlignmentCenter);button(g,L"→",579,574,49,31,52);
}

void renderControl(Graphics& g){g.Clear(BG);hits.clear();fill(g,28,28,6,41,GREEN);text(g,L"LANDELTA",46,19,350,37,28,FG,FontStyleBold);text(g,L"ACC OVERLAY  /  BETA 0.12",48,57,400,20,13,MUTED,FontStyleBold);tag(g,demoMode?L"DEMO":L"WINDOWS x64",503,30,125,demoMode?GOLD:GREEN);
 fill(g,28,92,600,76,CARD);fill(g,44,111,7,7,demoMode?GOLD:(live.registered?GREEN:BLUE));text(g,status,61,102,550,29,16,FG,FontStyleBold);text(g,demoMode?L"Prueba el diseño y acomoda tus paneles sin abrir el juego.":configStatus,45,133,565,23,14,MUTED);
 button(g,L"Paneles",28,185,112,36,1,false,page==0);button(g,L"Conexión",150,185,112,36,2,false,page==1);button(g,L"Historial",272,185,112,36,4,false,page==3);button(g,L"Inicio",394,185,112,36,5,false,page==4);button(g,L"Ayuda",516,185,112,36,3,false,page==2);
 if(page==0){section(g,L"PANELES INDEPENDIENTES · ACTIVA SOLO LO QUE NECESITES",235);const wchar_t* titles[]={L"Clasi",L"Relativos",L"Vuelta / sectores",L"Pedales",L"Delta",L"Últimos sectores",L"Ritmo / alcance"};for(int i=0;i<PanelCount;i++){float x=28.f+(i%2)*306,y=263.f+(i/2)*48;fill(g,x,y,294,42,CARD);text(g,titles[i],x+10,y,189,42,16,FG,FontStyleBold);button(g,cfg.visible[i]?L"ON":L"OFF",x+210,y+7,72,28,10+i,false,cfg.visible[i]);}
 text(g,L"Delta",334,407,76,42,14,MUTED);button(g,L"−",413,414,33,28,27);text(g,num(cfg.deltaScale*100,0)+L"%",450,414,80,28,15,FG,FontStyleBold,StringAlignmentCenter);button(g,L"+",534,414,33,28,28);
 button(g,cfg.keepAfterRace?L"Fuera de sesión: ON":L"Fuera de sesión: OFF",334,596,294,25,29,false,cfg.keepAfterRace);
 text(g,L"Tamaño",28,464,94,27,14,MUTED);button(g,L"−",124,464,33,28,20);text(g,num(cfg.scale*100,0)+L"%",162,464,58,28,15,FG,FontStyleBold,StringAlignmentCenter);button(g,L"+",225,464,33,28,21);
 text(g,L"Fondo",296,464,85,28,14,MUTED);button(g,L"−",389,464,33,28,22);text(g,num(cfg.opacity/255.*100,0)+L"%",427,464,70,28,15,FG,FontStyleBold,StringAlignmentCenter);button(g,L"+",502,464,33,28,23);
 text(g,L"Filas clasi",28,506,124,28,14,MUTED);button(g,L"−",162,506,33,28,24);text(g,std::to_wstring(cfg.rows),203,506,39,28,15,FG,FontStyleBold,StringAlignmentCenter);button(g,L"+",251,506,33,28,25);button(g,L"Restablecer posiciones",316,506,312,31,26);
 button(g,locked?L"Desbloquear paneles":L"Bloquear para conducir",28,554,292,41,30,true);button(g,demoMode?L"Usar datos de ACC":L"Probar modo demo",332,554,296,41,31,false,demoMode);
 }else if(page==1){section(g,L"CONEXIÓN LOCAL AUTOMÁTICA",238);fill(g,28,271,600,163,CARD);text(g,L"Assetto Corsa Competizione",44,282,568,28,17,FG,FontStyleBold);text(g,sharedOK?L"Tu auto: memoria local conectada":L"Tu auto: esperando memoria local",44,315,568,24,13,sharedOK?GREEN:MUTED);text(g,pilotsStatus(),44,345,568,24,13,live.active(nowMs()).empty()?GOLD:GREEN);text(g,L"UDP 127.0.0.1:"+std::to_wstring(port)+L" · Paquetes recibidos: "+std::to_wstring(udpReceived[1]+udpReceived[2]+udpReceived[3]),44,372,568,23,13,MUTED);text(g,configPath.empty()?L"Elige la carpeta Documentos de ACC.":configPath.wstring(),44,401,568,22,10,MUTED);
 button(g,L"Reparar conexión de pilotos",28,441,292,38,40,true);button(g,L"Elegir carpeta de ACC",332,441,296,38,41);button(g,L"Reconectar",28,494,187,36,42);button(g,L"Ver configuración",225,494,194,36,43);button(g,L"Diagnóstico",429,494,199,36,44);
 text(g,L"Si activas la conexión con ACC abierto, ciérralo y vuelve a abrirlo.",28,550,600,22,14,GOLD);text(g,L"Entra en pista: el menú del juego no envía todos los datos.",28,576,600,22,14,MUTED);
 }else if(page==3){renderHistory(g);
 }else if(page==4){section(g,L"LISTO CUANDO ABRAS ACC",240);
 fill(g,28,277,600,111,CARD);text(g,L"Iniciar con Windows",44,286,354,32,22,FG,FontStyleBold);
 text(g,startupInfo.current?L"Activado para este ejecutable":(startupInfo.exists?L"Hay un acceso de otra ubicación":L"Desactivado"),44,326,388,30,15,startupInfo.current?GREEN:GOLD);
 button(g,startupInfo.current?L"Desactivar":L"Activar",459,300,150,42,60,false,startupInfo.current);
 if(startupInfo.exists&&!startupInfo.current)button(g,L"Quitar acceso anterior",44,359,234,25,61);
 text(g,startupNotice.empty()?startupInfo.error:startupNotice,28,396,600,27,14,FG);
 text(g,L"1  Windows inicia LANDELTA oculto, junto al reloj.",28,435,600,27,16,FG);
 text(g,L"Mostrar overlays al entrar en sesión",28,466,435,27,16,FG);button(g,cfg.autoShow?L"ON":L"OFF",507,466,121,27,64,false,cfg.autoShow);
 text(g,L"3  Fuera de sesión: puedes mantenerlos desde Paneles.",28,497,600,27,16,FG);
 button(g,L"Dejar en segundo plano",28,540,292,40,62,true);button(g,L"Salir de LANDELTA",332,540,296,40,63);
 text(g,hotkeyIssue.empty()?L"Conserva el EXE aquí; si lo mueves, vuelve a activar esta opción.":hotkeyIssue,28,589,600,26,13,MUTED);

 }else{section(g,L"GUÍA RÁPIDA",240);
 const wchar_t* tips[]={L"ACC en ventana sin bordes · arrastra y bloquea los paneles.",L"Ctrl + Alt + F10: bloquear   ·   F11: ocultar   ·   F12: control",L"Bandera azul / −1 V: doblado. Amarilla / +1 V: te dobla.",L"PEN: sanción propia. !: aviso ACC, no confirma una sanción.",L"REL: cercanía en pista (~). SEÑAL?: dato temporalmente viejo.",L"GAS / FRENO: barras de entrada; sin porcentajes.",L"Δ MEJOR: vuelta limpia local; Δ ACC: referencia del juego.",L"Delta: verde ganas; amarillo ganas en inválida; rojo pierdes.",L"SECT: 2 últimas vueltas frente a tu mejor limpia anterior.",L"×: vuelta no limpia. —: sin referencia. S*: sector vuelta previa."};
 for(int i=0;i<10;i++)text(g,tips[i],28,274.f+i*32,600,30,14,(i==2?BLUE:MUTED));}

 line(g,28,624,600);text(g,page==4?L"X / minimizar: segundo plano · Icono junto al reloj: abrir / salir":(locked?L"PANELES BLOQUEADOS · Los clics pasan al juego":L"EDICIÓN · Rueda sobre el delta para ajustar su tamaño"),28,636,600,25,11,locked?GREEN:MUTED);
}
// Opaque strokes and telemetry fills stay visible even with a transparent panel.
void ink(Graphics& g,float x,float y,float w,float h,Color c){bool overlay=renderingOverlay;renderingOverlay=false;fill(g,x,y,w,h,c);renderingOverlay=overlay;}
void smallFlag(Graphics& g,float x,float y,Color c){ink(g,x,y,2,22,FG);ink(g,x+2,y,15,11,c);}
void badge(Graphics& g,const std::wstring& label,float x,float y,float w,Color c){fill(g,x,y,w,29,BG);ink(g,x,y,3,29,c);text(g,label,x+5,y,w-8,29,18,c,FontStyleBold,StringAlignmentCenter);}
int panelW(int i){const int widths[]={600,520,240,300,340,340,520};return widths[i];}
int panelH(int i){const int heights[]={0,226,188,82,66,206,112};return i==0?26+cfg.rows*40:heights[i];}
void slimHeader(Graphics& g,int i,const std::wstring& label){fill(g,0,0,panelW(i),24,BG);ink(g,0,0,3,24,GREEN);text(g,label,10,0,panelW(i)-20.f,24,16,MUTED,FontStyleBold);}
void emptyPanel(Graphics& g,int i){text(g,L"Esperando datos de ACC",10,40,panelW(i)-20.f,36,22,MUTED,FontStyleBold);}
void renderStandings(Graphics& g){auto& s=display();auto cars=s.active(nowMs(),paused());slimHeader(g,0,L"CLASI");text(g,L"GAP",351,0,102,24,16,MUTED,FontStyleBold,StringAlignmentFar);text(g,L"MEJOR",463,0,126,24,16,MUTED,FontStyleBold,StringAlignmentFar);if(cars.empty()){emptyPanel(g,0);return;}
 int start=std::clamp(scroll,0,std::max(0,(int)cars.size()-cfg.rows));
 // Follow the player by default; mouse-wheel scrolling can inspect the rest.
 if(scroll<0)for(int k=0;k<int(cars.size());k++)if(cars[k].id==selectedID())start=std::clamp(k-cfg.rows/2,0,std::max(0,int(cars.size())-cfg.rows));
 for(int k=0;k<cfg.rows&&k+start<int(cars.size());k++){const auto& c=cars[k+start];float y=26.f+k*40;bool me=c.id==selectedID();fill(g,0,y,600,38,me?Color(255,15,60,42):(k%2?BG:CARD));if(me)ink(g,0,y,3,38,GREEN);
 text(g,c.position?std::to_wstring(c.position):L"—",8,y,36,38,24,me?GREEN:FG,FontStyleBold);text(g,wide(c.name()),48,y,224,38,24,FG,FontStyleBold);
 auto p=acc::penaltyBadge(c,nowMs(),demoMode?c.id==4:(sharedOK&&c.id==shared.player),demoMode?7:shared.penalty);
 if(p!=acc::PenaltyBadge::None)badge(g,p==acc::PenaltyBadge::Confirmed?L"PEN":L"!",278,y+4,62,p==acc::PenaltyBadge::Confirmed?RED:GOLD);else if(c.location>=2)badge(g,L"BOX",278,y+4,62,GOLD);
 text(g,wide(s.gapToLeader(c,cars[0])),346,y,107,38,23,me?GREEN:FG,FontStyleBold,StringAlignmentFar);text(g,lapTime(c.best.ms),461,y,129,38,22,c.best.ms&&c.best.ms==s.sessionBest.ms?PURPLE:FG,FontStyleBold,StringAlignmentFar);
 }
}
void renderRelative(Graphics& g){auto& s=display();auto me=selectedCar();slimHeader(g,1,L"REL");if(demoMode)text(g,L"DEMO",413,0,98,24,16,GOLD,FontStyleBold,StringAlignmentFar);if(!me){emptyPanel(g,1);return;}if(me->location!=1){text(g,L"BOX · esperando salida",12,65,494,40,23,GOLD,FontStyleBold);return;}
 auto neighbors=acc::relativeRows(s,*me,nowMs(),paused());
 auto row=[&](const acc::Car* c,int r,bool player,bool stale){float y=26.f+r*40;fill(g,0,y,520,38,player?Color(255,15,60,42):CARD);text(g,player?L"●":(r<2?L"↑":L"↓"),7,y,24,38,23,player?GREEN:FG,FontStyleBold,StringAlignmentCenter);
 if(!c){text(g,L"—",40,y,464,38,24,MUTED);return;}
 int relation=acc::lapRelation(*c,*me,s.type==10);Color color=player?GREEN:(relation<0?BLUE:(relation>0?GOLD:FG));ink(g,0,y,3,38,color);text(g,wide(c->name()),40,y,226,38,24,color,FontStyleBold);
 if(stale){badge(g,L"SEÑAL?",273,y+4,91,GOLD);text(g,L"—",370,y,140,38,26,MUTED,FontStyleBold,StringAlignmentFar);return;}
 if(relation){smallFlag(g,274,y+8,color);text(g,(relation<0?L"−":L"+")+std::to_wstring(std::max(1,int(std::round(std::abs(c->laps+double(c->spline)-me->laps-me->spline)))))+L"V",295,y,69,38,19,color,FontStyleBold,StringAlignmentFar);}
 double gap=player?0:s.relativeEstimate(*c,*me);text(g,player?L"TÚ":(std::isfinite(gap)?L"~"+num(gap,1,true):L"—"),370,y,140,38,27,player?GREEN:FG,FontStyleBold,StringAlignmentFar);
 };
 for(int i=0;i<2;i++){const auto* n=i<int(neighbors.ahead.size())?&neighbors.ahead[i]:nullptr;row(n?&n->car:nullptr,1-i,false,n&&n->stale);}row(me,2,true,false);for(int i=0;i<2;i++){const auto* n=i<int(neighbors.behind.size())?&neighbors.behind[i]:nullptr;row(n?&n->car:nullptr,3+i,false,n&&n->stale);}
}
void renderPace(Graphics& g){auto& s=display();auto me=selectedCar();if(!me){emptyPanel(g,6);return;}
 if(s.type!=10||s.phase!=5||s.replay||paused()){text(g,paused()?L"RITMO · pausa":L"RITMO · disponible en carrera",10,0,500,44,20,MUTED);return;}
 auto& a=displayAnalytics();for(int i=0;i<2;i++){bool ahead=i==0;float y=i*57.f;fill(g,0,y,520,55,CARD);auto rival=acc::paceRival(s,*me,ahead,nowMs());text(g,ahead?L"↑":L"↓",9,y,24,28,22,MUTED,FontStyleBold);
 if(!rival){text(g,L"Ritmo · sin rival cercano",40,y,464,55,20,MUTED);continue;}
 text(g,wide(rival->name()),40,y,284,29,22,FG,FontStyleBold);double own=a.pace(me->id),other=a.pace(rival->id),gap=std::abs(s.relativeEstimate(*rival,*me));auto f=acc::forecast(gap,ahead?own:other,ahead?other:own,s.end>s.elapsed?(s.end-s.elapsed)/1000.:NAN);
 if(f.state==acc::ForecastState::Waiting){text(g,L"Faltan 3 vueltas limpias de ambos",12,y+28,496,25,18,MUTED);continue;}
 bool good=ahead?f.closing>0:f.closing<0;Color c=std::abs(f.closing)<.1?MUTED:(good?GREEN:GOLD);text(g,num(std::abs(f.closing),2)+L" s/v",324,y,184,29,22,c,FontStyleBold,StringAlignmentFar);
 std::wstring message;if(f.state==acc::ForecastState::Stable)message=L"Ritmo similar";else if(f.state==acc::ForecastState::Opening)message=ahead?L"Se aleja":L"Te alejas";else if(f.state==acc::ForecastState::AfterFinish)message=L"Alcance después del final";else message=(ahead?L"Lo alcanzas en ~":L"Te alcanza en ~")+num(f.laps,1)+L" vueltas";text(g,message,12,y+28,496,25,18,c,FontStyleBold);
 }
}
void renderTiming(Graphics& g){auto me=selectedCar();bool sm=sharedOK&&!demoMode&&!live.replay;auto t=acc::timingView(me,sm?&playerTiming:nullptr,nowMs());fill(g,0,0,240,50,CARD);text(g,t.available?L"V"+std::to_wstring(t.lap):L"V",8,0,46,50,19,MUTED,FontStyleBold);text(g,lapTime(t.ms),58,0,174,50,29,t.invalid?RED:FG,FontStyleBold,StringAlignmentFar);
 for(int i=0;i<3;i++){float y=54.f+i*46;fill(g,0,y,240,42,CARD);if(t.available&&!t.previous&&i==t.active)ink(g,0,y,3,42,BLUE);text(g,L"S"+std::to_wstring(i+1)+(t.previous?L"*":L""),10,y,46,42,19,MUTED,FontStyleBold);int v=t.splits[i];Color c=FG;if(me&&v>0&&me->best.splits[i]>0)c=v<=me->best.splits[i]?GREEN:RED;text(g,sectorTime(v),60,y,172,42,27,c,FontStyleBold,StringAlignmentFar);}
}
void renderPedals(Graphics& g){bool available=demoMode||(sharedOK&&shared.status==2&&!live.replay&&pedals.valid&&!pedals.expired(nowMs()));float gas=demoMode?float(.5+.5*std::sin(nowMs()/850.)):pedals.value.gas,brake=demoMode?float(std::max(0.,std::sin(nowMs()/1250.))):pedals.value.brake;
 for(int i=0;i<2;i++){float y=i*43.f;float value=i?brake:gas;Color c=i?RED:GREEN;fill(g,0,y,300,39,CARD);text(g,i?L"FRENO":L"GAS",9,y,72,39,17,c,FontStyleBold);ink(g,83,y+14,207,11,EDGE);if(available&&value>0)ink(g,83,y+14,207*value,11,c);if(!available)text(g,L"—",263,y,27,39,22,MUTED,FontStyleBold,StringAlignmentFar);}
}
void renderDelta(Graphics& g){auto d=lapDelta.view(playerTiming,nowMs());bool own=d.available;if(!own)d=acc::drivingDelta(playerTiming,nowMs());if(!sharedOK||live.replay)d.available=false;bool finished=!sharedOK||(shared.status!=2&&shared.status!=3);if(!demoMode&&finished&&cfg.keepAfterRace)d=retainedDelta.value;if(demoMode){d.available=true;d.ms=-327;d.invalid=false;}Color c=!d.available||d.ms==0?MUTED:(d.ms<0?(d.invalid?GOLD:GREEN):RED);fill(g,0,0,340,66,CARD);text(g,!demoMode&&finished&&d.available?L"Δ FIN":(d.invalid?L"Δ INV":(own?L"Δ MEJOR":L"Δ ACC")),10,0,110,40,18,MUTED,FontStyleBold);std::wstring label=d.available?num(d.ms/1000.,3,true)+L" s":(paused()?L"PAUSA":(sharedOK&&!shared.valid?L"INVÁLIDA":L"SIN REF."));text(g,label,109,0,220,40,29,c,FontStyleBold,StringAlignmentFar);
 ink(g,10,46,320,12,EDGE);if(d.available&&d.ms){float width=float(std::min(1.,std::abs(double(d.ms))/1000.)*158);ink(g,d.ms<0?169-width:171,46,width,12,c);}ink(g,169,41,2,22,FG);
}
void renderSectorReview(Graphics& g){slimHeader(g,5,L"SECT · vs mejor previa");auto rows=acc::recentSectors(displayAnalytics(),selectedID());if(!demoMode&&(!analyticsTrackReady||!sharedOK||live.replay||analytics.epoch!=live.epoch))rows.clear();if(rows.empty()){text(g,L"Completa una vuelta",12,65,316,34,22,MUTED,FontStyleBold);text(g,L"Comparación S1 / S2 / S3",12,102,316,29,18,MUTED);return;}
 for(int k=0;k<2;k++){float x=46.f+k*148;if(k>=int(rows.size())){text(g,L"—",x,26,138,27,20,MUTED,FontStyleBold,StringAlignmentFar);continue;}const auto& r=rows[k];text(g,L"V"+std::to_wstring(r.lap.number)+(r.lap.clean()?L"":L" ×"),x,26,138,27,19,r.lap.clean()?FG:GOLD,FontStyleBold,StringAlignmentFar);
 for(int i=0;i<3;i++){float y=56.f+i*50;fill(g,x,y,142,48,CARD);int ms=r.lap.timing.splits[i];text(g,sectorTime(ms),x+3,y,132,25,21,r.lap.timing.invalid?RED:FG,FontStyleBold,StringAlignmentFar);bool ok=r.comparable[i];int diff=r.difference[i];text(g,ok?num(diff/1000.,3,true):L"—",x+3,y+25,132,22,18,!ok||diff==0?MUTED:(diff<0?GREEN:RED),FontStyleBold,StringAlignmentFar);}}
 for(int i=0;i<3;i++)text(g,L"S"+std::to_wstring(i+1),7,56.f+i*50,34,48,19,MUTED,FontStyleBold);
}
void renderPanel(Graphics& g,int i){switch(i){case 0:renderStandings(g);break;case 1:renderRelative(g);break;case 2:renderTiming(g);break;case 3:renderPedals(g);break;case 4:renderDelta(g);break;case 5:renderSectorReview(g);break;case 6:renderPace(g);break;}}
// Per-pixel alpha: text stays opaque; empty pixels stay completely transparent.
// Do not use SetLayeredWindowAttributes on these windows: it disables this path.
DWORD overlayError=0;
void presentOverlay(HWND window,int i){
 if(!window)return;int width=(int)(panelW(i)*cfg.scale*(i==4?cfg.deltaScale:1.f)),height=(int)(panelH(i)*cfg.scale*(i==4?cfg.deltaScale:1.f));
 BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=width;info.bmiHeader.biHeight=-height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
 void* pixels=nullptr;HDC mem=CreateCompatibleDC(nullptr);if(!mem)return;HBITMAP dib=CreateDIBSection(mem,&info,DIB_RGB_COLORS,&pixels,nullptr,0);if(!dib){DeleteDC(mem);return;}auto old=SelectObject(mem,dib);
 {Bitmap bitmap(width,height,width*4,PixelFormat32bppPARGB,(BYTE*)pixels);Graphics g(&bitmap);g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);g.Clear(Color(0,0,0,0));g.ScaleTransform(cfg.scale*(i==4?cfg.deltaScale:1.f),cfg.scale*(i==4?cfg.deltaScale:1.f));renderingOverlay=true;renderPanel(g,i);renderingOverlay=false;g.Flush(FlushIntentionSync);
 SIZE size{width,height};POINT source{0,0};BLENDFUNCTION blend{AC_SRC_OVER,0,255,AC_SRC_ALPHA};if(!UpdateLayeredWindow(window,nullptr,nullptr,&size,mem,&source,0,&blend,ULW_ALPHA))overlayError=GetLastError();else overlayError=0;}
 SelectObject(mem,old);DeleteObject(dib);DeleteDC(mem);
}
void updateWindows(){for(int i=0;i<PanelCount;i++){DWORD ex=WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_LAYERED|WS_EX_NOACTIVATE;if(locked)ex|=WS_EX_TRANSPARENT;SetWindowLongPtrW(panels[i],GWL_EXSTYLE,ex);int w=(int)(panelW(i)*cfg.scale*(i==4?cfg.deltaScale:1.f)),h=(int)(panelH(i)*cfg.scale*(i==4?cfg.deltaScale:1.f));RECT v{GetSystemMetrics(SM_XVIRTUALSCREEN),GetSystemMetrics(SM_YVIRTUALSCREEN),GetSystemMetrics(SM_XVIRTUALSCREEN)+GetSystemMetrics(SM_CXVIRTUALSCREEN),GetSystemMetrics(SM_YVIRTUALSCREEN)+GetSystemMetrics(SM_CYVIRTUALSCREEN)};if(cfg.x[i]+w<v.left||cfg.x[i]>v.right-25)cfg.x[i]=v.left+25;if(cfg.y[i]+h<v.top||cfg.y[i]>v.bottom-25)cfg.y[i]=v.top+90;SetWindowPos(panels[i],HWND_TOPMOST,cfg.x[i],cfg.y[i],w,h,SWP_NOACTIVATE|SWP_FRAMECHANGED);presentOverlay(panels[i],i);ShowWindow(panels[i],sessionVisibility.show(demoMode,cfg.hidden,cfg.visible[i],cfg.keepAfterRace)?SW_SHOWNOACTIVATE:SW_HIDE);}if(control){SetWindowPos(control,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);InvalidateRect(control,nullptr,FALSE);}adjustPolling();}
void adjustPolling(){if(!control)return;unsigned interval=acc::backgroundPollInterval(gameRunning,demoMode);if(interval!=mainInterval){SetTimer(control,1,interval,nullptr);mainInterval=interval;}bool needsPedals=IsWindowVisible(panels[3])&&(gameRunning||demoMode);if(needsPedals!=pedalTimer){if(needsPedals)SetTimer(control,2,33,nullptr);else KillTimer(control,2);pedalTimer=needsPedals;}}
void syncSessionVisibility(uint64_t now){bool before=sessionVisibility.active;bool udp=live.registered&&live.hasSession&&live.phase>0&&acc::fresh(live.sessionReceived,now,8000);bool local=sharedOK&&(shared.status==2||shared.status==3);
 sessionVisibility.update(now,gameRunning,sharedOffline,local,udp);
 bool started=sessionLaunch.started(sessionVisibility.active,local,shared.session,shared.sessionIndex,shared.laps,udp,live.event,live.session,live.type,shared.currentMs,live.elapsed,udpGeneration);
 if(started&&!demoMode){if(cfg.autoShow){cfg.hidden=false;hideControl();}locked=true;updateWindows();}
 else if(before!=sessionVisibility.active)updateWindows();
 else for(int i=0;i<PanelCount;i++){bool show=sessionVisibility.show(demoMode,cfg.hidden,cfg.visible[i],cfg.keepAfterRace);if(bool(IsWindowVisible(panels[i]))!=show){if(show)presentOverlay(panels[i],i);ShowWindow(panels[i],show?SW_SHOWNOACTIVATE:SW_HIDE);}}
 adjustPolling();
}
NOTIFYICONDATAW trayData(){NOTIFYICONDATAW n{};n.cbSize=sizeof n;n.hWnd=control;n.uID=1;return n;}
bool installTray(){if(!control)return false;auto n=trayData();n.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP|NIF_SHOWTIP;n.uCallbackMessage=TrayMessage;n.hIcon=LoadIconW(GetModuleHandleW(nullptr),MAKEINTRESOURCEW(101));std::wstring tip=L"LANDELTA · "+status;tip.copy(n.szTip,127);n.szTip[127]=0;trayPresent=Shell_NotifyIconW(NIM_ADD,&n)!=FALSE;trayV4=false;if(trayPresent){n.uVersion=NOTIFYICON_VERSION_4;trayV4=Shell_NotifyIconW(NIM_SETVERSION,&n)!=FALSE;}return trayPresent;}
void updateTray(){if(!trayPresent){installTray();return;}auto n=trayData();n.uFlags=NIF_TIP|NIF_SHOWTIP;std::wstring tip=L"LANDELTA · "+status;tip.copy(n.szTip,127);n.szTip[127]=0;if(!Shell_NotifyIconW(NIM_MODIFY,&n))trayPresent=false;}
void removeTray(){if(trayPresent){auto n=trayData();Shell_NotifyIconW(NIM_DELETE,&n);}trayPresent=false;}
void openControl(){refreshStartup();ShowWindow(control,SW_RESTORE);SetForegroundWindow(control);InvalidateRect(control,nullptr,FALSE);}
bool hideControl(){if(!trayPresent&&!installTray())return false;ShowWindow(control,SW_HIDE);return true;}
bool waitInTray(){if(!hideControl())return false;demoMode=false;updateStatus(nowMs());updateWindows();return true;}
void action(int id);
void trayMenu(){HMENU menu=CreatePopupMenu();if(!menu)return;AppendMenuW(menu,MF_STRING,70,L"Abrir LANDELTA");AppendMenuW(menu,MF_STRING,71,cfg.hidden?L"Mostrar overlays":L"Ocultar overlays");AppendMenuW(menu,MF_STRING,5,L"Inicio con Windows…");AppendMenuW(menu,MF_SEPARATOR,0,nullptr);AppendMenuW(menu,MF_STRING,63,L"Salir de LANDELTA");POINT point{};GetCursorPos(&point);SetForegroundWindow(control);UINT id=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,point.x,point.y,0,control,nullptr);DestroyMenu(menu);PostMessageW(control,WM_NULL,0,0);if(id==70)openControl();else if(id==5){page=4;openControl();}else if(id)action(id);else{auto n=trayData();Shell_NotifyIconW(NIM_SETFOCUS,&n);}}
BOOL CALLBACK findLANDELTAControl(HWND h,LPARAM arg){wchar_t name[64]{};GetClassNameW(h,name,64);if(!wcscmp(name,L"VortexACCWindow")&&GetWindowLongPtrW(h,GWLP_USERDATA)==0&&(GetWindowLongPtrW(h,GWL_STYLE)&WS_CAPTION)){*reinterpret_cast<HWND*>(arg)=h;return FALSE;}return TRUE;}
void diagnostic(){std::wostringstream s;s<<L"LANDELTA 0.12 - Diagnóstico\r\n"<<L"Estado: "<<status<<L"\r\nConfiguración: "<<configStatus<<L"\r\nArchivo: "<<configPath.wstring()<<L"\r\nPuerto: "<<port<<L"\r\nJuego detectado: "<<gameRunning<<L"\r\nMemoria compartida: "<<sharedOK<<L"\r\nUDP registrado: "<<live.registered<<L"\r\nID jugador: "<<selectedID()<<L"\r\nPilotos recientes: "<<live.active(nowMs()).size()<<L"\r\nPaquetes rechazados: "<<live.badPackets<<L"\r\nCircuito: "<<wide(live.track)<<L"\r\n";
 s<<L"Proceso ACC: "<<gamePID<<L"\r\nEvento / sesión / tipo / fase UDP: "<<live.event<<L" / "<<live.session<<L" / "<<live.type<<L" / "<<live.phase<<L"\r\nSesión local: "<<playerTiming.g.sessionIndex<<L"\r\nReinicios de conexión: "<<networkReconnects<<L"\r\nCambios de sesión detectados: "<<sessionResets<<L"\r\nÚltimo reinicio: "<<lastReset<<L"\r\nÚltimo error UDP: "<<networkError<<L"\r\nPaquetes rechazados (total): "<<rejectedPackets<<L"\r\n";
 s<<L"Inicio con Windows / esta copia: "<<startupInfo.exists<<L" / "<<startupInfo.current<<L"\r\nInicio oculto: "<<backgroundLaunch<<L"\r\nBandeja: "<<trayPresent<<L"\r\nSesión visible: "<<sessionVisibility.active<<L"\r\nFrecuencia de sondeo (ms): "<<mainInterval<<L"\r\nAtajos: "<<hotkeyIssue<<L"\r\n";
 s<<L"Delta local: referencia ms / muestras: "<<lapDelta.best<<L" / "<<lapDelta.reference.size()<<L"; grabando: "<<lapDelta.recording<<L"\r\n";
 s<<L"Delta ACC: mejor / actual / delta / signo / estimada / válida: "<<shared.bestMs<<L" / "<<shared.currentMs<<L" / "<<shared.deltaMs<<L" / "<<shared.deltaPositive<<L" / "<<shared.estimatedMs<<L" / "<<shared.valid<<L"\r\n";
 s<<L"Mostrar al iniciar sesión: "<<cfg.autoShow<<L"; sesión visible: "<<sessionVisibility.active<<L"; ocultos: "<<cfg.hidden<<L"\r\n";
 s<<L"Pedales: "<<pedals.valid<<L" / gas: "<<pedals.value.gas<<L" / freno: "<<pedals.value.brake<<L"\r\n";
 s<<L"Último problema de lectura de configuración: "<<wide(configReadIssue)<<L"\r\n";
 s<<L"Recuperaciones por parrilla detenida: "<<gridRecoveries<<L"\r\nÚltimo error de protocolo: "<<protocolError<<L"\r\nAutos recibidos antes de sesión válida: "<<gatedCars<<L"\r\n";
 for(int type=1;type<=8;type++)s<<L"UDP tipo "<<type<<L" recibidos / aceptados / rechazados: "<<udpReceived[type]<<L" / "<<udpAccepted[type]<<L" / "<<udpRejected[type]<<L"\r\n";
 s<<L"Sesiones guardadas: "<<analytics.history.size()<<L"\r\nHistorial: "<<wide(historyStore.error)<<L"\r\nPendientes de guardar: "<<analytics.dirty.size()<<L"\r\n";
 if(live.sessionReceived)s<<L"Edad del estado UDP (ms): "<<nowMs()-live.sessionReceived<<L"\r\n";size_t samples=0;for(const auto& kv:live.passages)samples+=kv.second.samples.size();s<<L"Muestras de pasos en pista: "<<samples<<L"\r\n";
 s<<L"Penalización del jugador: "<<(sharedOK?std::to_wstring(shared.penalty):L"sin dato")<<L"\r\nAvisos UDP recibidos (no confirman sanción pendiente):\r\n";for(const auto& kv:live.cars){const auto& c=kv.second;if(c.penaltyNoticeAt)s<<L"Auto "<<c.id<<L" / "<<wide(c.name())<<L": "<<wide(c.penaltyMessage)<<L"\r\n";}
 s<<L"\r\nSi no llegan pilotos: entra en una sesión, revisa broadcasting.json y reinicia ACC.\r\nSi no aparece el overlay: usa ventana sin bordes.\r\n";auto p=dataDir/L"diagnostico.txt";if(fileWrite(p,"\xef\xbb\xbf"+utf8(s.str())))ShellExecuteW(nullptr,L"open",p.c_str(),nullptr,nullptr,SW_SHOWNORMAL);}
void repairPilots(){
 if(gameRunning){configStatus=L"Cierra ACC y pulsa Reparar: el juego carga la conexión al arrancar.";networkError=configStatus;return;}
 try{locateACC();if(configPath.empty()){configureACC(true);return;}if(fs::exists(configPath)){auto backup=configPath;backup+=L".repair-"+std::to_wstring(nowMs())+L".bak";fs::copy_file(configPath,backup);}
 json j=json::object();try{if(fs::exists(configPath))j=json::parse(decodeConfig(fileRead(configPath)));}catch(...){j=json::object();}if(!j.is_object())j=json::object();
 bool validPort=j.contains("updListenerPort")&&j["updListenerPort"].is_number_integer()&&j["updListenerPort"].get<int64_t>()>0&&j["updListenerPort"].get<int64_t>()<=65535;
 if(!validPort)j["updListenerPort"]=9000;
 if(!j.contains("connectionPassword")||!j["connectionPassword"].is_string())j["connectionPassword"]="vortex-local";
 if(!j.contains("commandPassword")||!j["commandPassword"].is_string())j["commandPassword"]="";
 auto tmp=configPath;tmp+=L".repair.tmp";if(!fileWrite(tmp,encodeConfig(j.dump(2)))||!MoveFileExW(tmp.c_str(),configPath.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("No se pudo guardar");
 configureACC(true);reconnect();configStatus=L"Conexión revisada y respaldada. Abre ACC y entra en la carrera.";
 }catch(const std::exception& e){configStatus=L"No se pudo reparar: "+wide(e.what());networkError=configStatus;}
}
void chooseFolder(){BROWSEINFOW b{};b.hwndOwner=control;b.lpszTitle=L"Elige Documentos\\Assetto Corsa Competizione (la carpeta que contiene Config)";b.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE;auto item=SHBrowseForFolderW(&b);if(item){wchar_t path[MAX_PATH];if(SHGetPathFromIDListW(item,path)){fs::path p(path);if(fs::is_directory(p/L"Config")){configPath=p/L"Config"/L"broadcasting.json";configureACC(true);reconnect();}else MessageBoxW(control,L"Esta carpeta no contiene Config. Elige la carpeta de ACC dentro de Documentos.",L"LANDELTA",MB_ICONINFORMATION);}CoTaskMemFree(item);}}
void exportHistoryImage();
void action(int id){if(id>=100&&id<106&&id-100<(int)historyTargets.size()){if(historyView==0){historyTrack=historyTargets[id-100];historyView=1;}else if(historyView==1){historySelected=historyTargets[id-100];historyView=2;}historyOffset=0;}switch(id){case 1:page=0;break;case 2:page=1;break;case 3:page=2;break;case 4:page=3;break;case 5:page=4;refreshStartup();break;case 60:setStartup(!startupInfo.current);break;case 61:setStartup(false);break;case 64:cfg.autoShow=!cfg.autoShow;break;case 62:if(!waitInTray()){startupNotice=L"No se pudo crear el icono de bandeja; LANDELTA sigue abierto.";}break;case 63:DestroyWindow(control);return;case 71:cfg.hidden=!cfg.hidden;break;case 50:historyView=std::max(0,historyView-1);historyOffset=0;break;case 51:historyOffset=std::max(0,historyOffset-historyPageSize);break;case 52:if(historyOffset+historyPageSize<historyTotal)historyOffset+=historyPageSize;break;case 10:case 11:case 12:case 13:case 14:case 15:case 16:cfg.visible[id-10]=!cfg.visible[id-10];break;case 27:cfg.deltaScale=std::max(.5f,cfg.deltaScale-.1f);break;case 28:cfg.deltaScale=std::min(2.5f,cfg.deltaScale+.1f);break;case 29:cfg.keepAfterRace=!cfg.keepAfterRace;break;case 53:exportHistoryImage();break;case 20:cfg.scale=std::max(.65f,cfg.scale-.05f);break;case 21:cfg.scale=std::min(1.6f,cfg.scale+.05f);break;case 22:cfg.opacity=std::max(0,cfg.opacity-16);break;case 23:cfg.opacity=std::min(255,cfg.opacity+16);break;case 24:cfg.rows=std::max(4,cfg.rows-2);break;case 25:cfg.rows=std::min(30,cfg.rows+2);break;case 26:for(int i=0;i<PanelCount;i++){cfg.x[i]=Config{}.x[i];cfg.y[i]=Config{}.y[i];}scroll=-1;break;case 30:locked=!locked;if(locked&&!hideControl())ShowWindow(control,SW_MINIMIZE);break;case 31:analytics.endSession();demoMode=!demoMode;historyView=historyOffset=0;generateDemo(nowMs());break;case 40:repairPilots();break;case 41:chooseFolder();break;case 42:configureACC();reconnect();break;case 43:if(!configPath.empty()&&fs::exists(configPath))ShellExecuteW(control,L"open",L"notepad.exe",(L"\""+configPath.wstring()+L"\"").c_str(),nullptr,SW_SHOWNORMAL);break;case 44:diagnostic();break;}saveSettings();updateStatus(nowMs());updateWindows();}
LRESULT CALLBACK wndProc(HWND h,UINT m,WPARAM w,LPARAM l){int idx=(int)GetWindowLongPtrW(h,GWLP_USERDATA)-1;bool ctl=idx<0;
 if(ctl&&taskbarCreated&&m==taskbarCreated){trayPresent=false;installTray();return 0;}
 if(ctl&&m==TrayMessage){UINT event=trayV4?LOWORD(l):UINT(l);if(event==WM_LBUTTONUP||event==NIN_SELECT||event==NIN_KEYSELECT)openControl();else if(event==WM_CONTEXTMENU||(!trayV4&&event==WM_RBUTTONUP))trayMenu();return 0;}
 switch(m){case WM_NCCREATE:{auto c=(CREATESTRUCTW*)l;SetWindowLongPtrW(h,GWLP_USERDATA,(LONG_PTR)c->lpCreateParams);break;}
 case WM_ERASEBKGND:return 1;
 case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);if(!ctl){EndPaint(h,&ps);presentOverlay(h,idx);return 0;}RECT r;GetClientRect(h,&r);int width=r.right,height=r.bottom;if(width>0&&height>0){Bitmap bmp(width,height,PixelFormat32bppARGB);Graphics g(&bmp);g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);g.Clear(BG);g.ScaleTransform(width/656.f,height/680.f);renderControl(g);Graphics out(dc);out.DrawImage(&bmp,0,0);}EndPaint(h,&ps);return 0;}
 case WM_LBUTTONDOWN:if(ctl){RECT r;GetClientRect(h,&r);float x=LOWORD(l)*656.f/r.right,y=HIWORD(l)*680.f/r.bottom;for(auto& hit:hits)if(hit.first.Contains(x,y)){action(hit.second);break;}}else if(!locked){ReleaseCapture();SendMessageW(h,WM_NCLBUTTONDOWN,HTCAPTION,0);}return 0;
 case WM_MOUSEACTIVATE:if(!ctl)return MA_NOACTIVATE;break;
 case WM_MOUSEWHEEL:if(idx==4&&!locked){action(GET_WHEEL_DELTA_WPARAM(w)>0?28:27);return 0;}if(ctl&&page==3){action(GET_WHEEL_DELTA_WPARAM(w)>0?51:52);return 0;}if(idx==0&&!locked){if(scroll<0){auto cars=display().active(nowMs(),paused());scroll=0;for(int k=0;k<int(cars.size());k++)if(cars[k].id==selectedID())scroll=std::clamp(k-cfg.rows/2,0,std::max(0,int(cars.size())-cfg.rows));}scroll+=GET_WHEEL_DELTA_WPARAM(w)>0?-3:3;scroll=std::clamp(scroll,0,std::max(0,(int)display().active(nowMs()).size()-cfg.rows));InvalidateRect(h,nullptr,FALSE);}return 0;
 case WM_EXITSIZEMOVE:if(!ctl){RECT r;GetWindowRect(h,&r);cfg.x[idx]=r.left;cfg.y[idx]=r.top;saveSettings();}return 0;
 case WM_TIMER:if(ctl){uint64_t now=nowMs();if(w==2){pollInputs(now);if(IsWindowVisible(panels[3]))presentOverlay(panels[3],3);return 0;}if(now-lastCheck>=1000){DWORD pid=accProcess();if(pid!=gamePID){analytics.endSession();gamePID=pid;gameRunning=pid!=0;lapDelta.reset();closeMapping();reconnect();lastReset=gameRunning?L"Inicio / reinicio de ACC":L"ACC cerrado";++sessionResets;}lastCheck=now;}
  if(!configAttempted||now-lastConfigCheck>=5000){int oldPort=port;auto oldPassword=password;bool ready=configReady;configureACC();configAttempted=true;lastConfigCheck=now;if(port!=oldPort||password!=oldPassword||configReady!=ready)reconnect();}
  pollShared(now);if(sharedOK&&shared.status==3)analytics.pausedAt(now);pollUDP(now);if(!analytics.dirty.empty()&&now-lastHistoryFlush>=2000){historyStore.flush(analytics);lastHistoryFlush=now;}if(demoMode)generateDemo(now);updateStatus(now);syncSessionVisibility(now);if(now-lastTrayCheck>=5000){updateTray();lastTrayCheck=now;}if(IsWindowVisible(control)&&!IsIconic(control))InvalidateRect(control,nullptr,FALSE);for(int i=0;i<PanelCount;i++)if(i!=3&&IsWindowVisible(panels[i]))presentOverlay(panels[i],i);}return 0;
 case WM_HOTKEY:if(w==1)action(30);if(w==2){cfg.hidden=!cfg.hidden;updateWindows();}if(w==3)openControl();return 0;
 case WM_SYSCOMMAND:if(ctl&&(w&0xFFF0)==SC_MINIMIZE&&waitInTray())return 0;break;
 case WM_QUERYENDSESSION:return TRUE;
 case WM_ENDSESSION:if(ctl&&w)DestroyWindow(h);return 0;
 case WM_CLOSE:if(ctl&&!waitInTray())DestroyWindow(h);return 0;
 case WM_DESTROY:if(ctl){removeTray();KillTimer(h,1);KillTimer(h,2);historyStore.flush(analytics);closeSocket();closeMapping();saveSettings();for(auto p:panels)if(IsWindow(p))DestroyWindow(p);PostQuitMessage(0);}return 0;
 }return DefWindowProcW(h,m,w,l);}

// Deterministic offscreen rendering uses the same drawing code as the actual EXE.
// Export the selected session, not just the six rows visible in the history tab.
void exportHistoryImage(){
 const auto& records=displayAnalytics().history;
 auto it=std::find_if(records.begin(),records.end(),[](const acc::SessionRecord& r){return r.id==historySelected;});
 if(it==records.end()||it->laps.empty())return;
 const auto record=*it; // Stable copy while a live session continues recording.
 wchar_t filename[32768]=L"LANDELTA-tiempos.png";
 OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=control;dialog.lpstrFilter=L"Imagen PNG\0*.png\0\0";dialog.lpstrFile=filename;dialog.nMaxFile=32768;dialog.lpstrDefExt=L"png";dialog.lpstrTitle=L"Exportar tiempos de la sesión";dialog.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
 if(!GetSaveFileNameW(&dialog)){if(CommDlgExtendedError())MessageBoxW(control,L"No se pudo abrir el diálogo de guardado.",L"LANDELTA",MB_OK|MB_ICONERROR);return;}
 UINT count=0,bytes=0;CLSID encoder{};bool found=false;
 if(GetImageEncodersSize(&count,&bytes)==Ok&&bytes){std::vector<BYTE> storage(bytes);auto info=reinterpret_cast<ImageCodecInfo*>(storage.data());if(GetImageEncoders(count,bytes,info)==Ok)for(UINT i=0;i<count;i++)if(!wcscmp(info[i].MimeType,L"image/png")){encoder=info[i].Clsid;found=true;break;}}
 if(!found){MessageBoxW(control,L"No se encontró el codificador PNG.",L"LANDELTA",MB_OK|MB_ICONERROR);return;}
 const fs::path base(filename);const size_t rows=40,pages=(record.laps.size()+rows-1)/rows;
 std::vector<fs::path> targets,temporary;
 try{
  for(size_t page=0;page<pages;page++){
   auto path=page?base.parent_path()/(base.stem().wstring()+L"-"+std::to_wstring(page+1)+L".png"):base;
   if(page&&fs::exists(path)&&MessageBoxW(control,(L"¿Reemplazar también " + path.filename().wstring()+L"?").c_str(),L"LANDELTA",MB_YESNO|MB_ICONQUESTION)!=IDYES)return;
   targets.push_back(path);
  }
  for(size_t page=0;page<pages;page++){
   size_t n=std::min(rows,record.laps.size()-page*rows);int height=280+int(n)*42;
   Bitmap bitmap(1200,height,PixelFormat32bppARGB);if(bitmap.GetLastStatus()!=Ok)throw std::runtime_error("bitmap");
   {Graphics g(&bitmap);g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);g.Clear(BG);
    renderHistoryImage(g,record,page);g.Flush(FlushIntentionSync);
   }
   auto temp=targets[page];temp+=L".tmp-"+std::to_wstring(GetCurrentProcessId());temporary.push_back(temp);
   if(bitmap.Save(temp.c_str(),&encoder,nullptr)!=Ok)throw std::runtime_error("save");
  }
  for(size_t page=0;page<pages;page++)if(!MoveFileExW(temporary[page].c_str(),targets[page].c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("move");
  MessageBoxW(control,(L"Exportado: "+std::to_wstring(record.laps.size())+L" vueltas en "+std::to_wstring(pages)+L" imagen(es).\n"+base.parent_path().wstring()).c_str(),L"LANDELTA",MB_OK|MB_ICONINFORMATION);
 }catch(...){for(const auto& path:temporary){std::error_code error;fs::remove(path,error);}MessageBoxW(control,L"No se pudo completar la exportación. Comprueba el espacio y los permisos de la carpeta.",L"LANDELTA",MB_OK|MB_ICONERROR);}
}

void exportPreview(const fs::path& p){demoMode=true;generateDemo(25000);Bitmap bmp(1280,900,PixelFormat32bppARGB);Graphics g(&bmp);g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);g.Clear(Color(0,0,0,0));text(g,L"LANDELTA / COMPACT",28,14,1100,44,26,FG,FontStyleBold);text(g,L"BETA 0.12 · DATOS SIMULADOS · PANELES INDEPENDIENTES",30,60,1100,25,14,MUTED);renderingOverlay=true;const int x[]={28,668,28,292,668,292,668},y[]={105,105,477,477,361,583,461};for(int i=0;i<PanelCount;i++){g.TranslateTransform(x[i],y[i]);renderPanel(g,i);g.ResetTransform();}renderingOverlay=false;UINT n=0,size=0;GetImageEncodersSize(&n,&size);std::vector<BYTE> buf(size);auto info=(ImageCodecInfo*)buf.data();GetImageEncoders(n,size,info);for(UINT i=0;i<n;i++)if(!wcscmp(info[i].MimeType,L"image/png")){bmp.Save(p.c_str(),&info[i].Clsid,nullptr);break;}}

int WINAPI wWinMain(HINSTANCE inst,HINSTANCE,LPWSTR args,int show){backgroundLaunch=std::wstring(args).find(L"--background")!=std::wstring::npos;SetProcessDPIAware();CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);GdiplusStartupInput input;if(GdiplusStartup(&gdip,&input,nullptr)!=Ok)return 1;
 if(std::wstring(args).find(L"--preview")!=std::wstring::npos){exportPreview(fs::current_path()/L"LANDELTA-demo.png");GdiplusShutdown(gdip);CoUninitialize();return 0;}
 HANDLE mutex=CreateMutexW(nullptr,TRUE,L"Local\\VortexACCOverlay");if(GetLastError()==ERROR_ALREADY_EXISTS){if(!backgroundLaunch){HWND h=nullptr;EnumWindows(findLANDELTAControl,reinterpret_cast<LPARAM>(&h));if(h){ShowWindow(h,SW_RESTORE);SetForegroundWindow(h);}}CloseHandle(mutex);GdiplusShutdown(gdip);CoUninitialize();return 0;}
 WSADATA ws;if(WSAStartup(MAKEWORD(2,2),&ws))return 2;try{dataDir=knownFolder(FOLDERID_LocalAppData)/L"VortexACC";fs::create_directories(dataDir);settingsPath=dataDir/L"settings.json";historyStore.directory=dataDir/L"history";historyStore.load(analytics);loadSettings();locateStartup();refreshStartup();gamePID=accProcess();gameRunning=gamePID!=0;configureACC();configAttempted=true;}catch(...){configStatus=L"No se pudieron cargar los ajustes locales";}
 demoMode=!backgroundLaunch&&std::wstring(args).find(L"--demo")!=std::wstring::npos;locked=backgroundLaunch;taskbarCreated=RegisterWindowMessageW(L"TaskbarCreated");generateDemo(nowMs());updateStatus(nowMs());WNDCLASSEXW wc{};wc.cbSize=sizeof wc;wc.lpfnWndProc=wndProc;wc.hInstance=inst;wc.lpszClassName=L"VortexACCWindow";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hIcon=LoadIconW(inst,MAKEINTRESOURCEW(101));wc.hIconSm=wc.hIcon;RegisterClassExW(&wc);
 RECT rect{0,0,656,680};AdjustWindowRectEx(&rect,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,FALSE,0);control=CreateWindowExW(WS_EX_TOPMOST,wc.lpszClassName,L"LANDELTA ACC Overlay · Beta 0.12",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,100,60,rect.right-rect.left,rect.bottom-rect.top,nullptr,nullptr,inst,nullptr);for(int i=0;i<PanelCount;i++){panels[i]=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_LAYERED|WS_EX_NOACTIVATE,wc.lpszClassName,L"LANDELTA",WS_POPUP,cfg.x[i],cfg.y[i],panelW(i),panelH(i),nullptr,nullptr,inst,(void*)(INT_PTR)(i+1));}updateWindows();installTray();if(!backgroundLaunch||!trayPresent)ShowWindow(control,show);
 bool keys=RegisterHotKey(control,1,MOD_CONTROL|MOD_ALT|MOD_NOREPEAT,VK_F10);keys=RegisterHotKey(control,2,MOD_CONTROL|MOD_ALT|MOD_NOREPEAT,VK_F11)&&keys;keys=RegisterHotKey(control,3,MOD_CONTROL|MOD_ALT|MOD_NOREPEAT,VK_F12)&&keys;if(!keys)hotkeyIssue=L"Hay atajos ocupados; abre LANDELTA desde el icono junto al reloj.";adjustPolling();MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}for(int i=1;i<=3;i++)UnregisterHotKey(control,i);WSACleanup();ReleaseMutex(mutex);CloseHandle(mutex);GdiplusShutdown(gdip);CoUninitialize();return 0;
}

#pragma once
#include "ac_capture.hpp"
#include <thread>
#include <atomic>
#include <mutex>
#include <iomanip>
namespace acstudio {
struct Mapping {
 HANDLE handle=nullptr;const void* data=nullptr;
 void close(){if(data)UnmapViewOfFile(data);if(handle)CloseHandle(handle);data=nullptr;handle=nullptr;}
 ~Mapping(){close();}
 bool open(const wchar_t* name,size_t bytes){if(data)return true;handle=OpenFileMappingW(FILE_MAP_READ,FALSE,name);if(handle)data=MapViewOfFile(handle,FILE_MAP_READ,0,0,bytes);if(!data)close();return data;}
 template<class T>bool read(T& out){if(!data)return false;for(int i=0;i<3;i++){int a,b;memcpy(&a,data,4);MemoryBarrier();memcpy(&out,data,sizeof out);MemoryBarrier();memcpy(&b,data,4);if(a==b&&out.packet==a)return true;}return false;}
};
inline DWORD process(){DWORD found=0;HANDLE h=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(h==INVALID_HANDLE_VALUE)return 0;PROCESSENTRY32W e{};e.dwSize=sizeof e;
 if(Process32FirstW(h,&e))do{if(!_wcsicmp(e.szExeFile,L"AC2-Win64-Shipping.exe")){found=0;break;}if(!_wcsicmp(e.szExeFile,L"acs.exe")||!_wcsicmp(e.szExeFile,L"acs_x86.exe"))found=e.th32ProcessID;}while(Process32NextW(h,&e));CloseHandle(h);return found;}
template<size_t N>std::string label(const char16_t(&v)[N]){std::wstring w;for(auto c:v){if(!c)break;w.push_back(wchar_t(c));}return utf8(w);}
struct Studio {
 struct Snapshot {std::wstring track,car;int lap=0,currentMs=0,lastMs=0;float gas=0,brake=0,speed=0;bool active=false;};
 Snapshot snapshot;
 HWND window=nullptr,statusLabel=nullptr;std::thread worker;std::atomic<bool> running{false};std::mutex mutex;
 std::wstring status=L"Captura automática preparada";fs::path directory;uint64_t count=0;
 void message(std::wstring s){std::lock_guard<std::mutex> lock(mutex);status=std::move(s);snapshot.active=status.rfind(L"GRABANDO",0)==0;}
 std::pair<Snapshot,std::wstring> read(){std::lock_guard<std::mutex> lock(mutex);return {snapshot,status};}
 void stop(){running=false;if(worker.joinable())worker.join();}
 ~Studio(){stop();}
 void capture(){
  Mapping physics,graphics,info;std::ofstream csv;DWORD pid=0;uint64_t check=0,flush=0,lastSample=0,opened=0,probe=0,displayAt=0;
  int packet=-1,session=-1,segment=0;std::string identity;LapTracker tracker;
  auto finish=[&](){if(csv.is_open()){csv.flush();if(!csv)throw std::runtime_error("disk");csv.close();}packet=-1;tracker=LapTracker{};};
  try{fs::create_directories(directory);
   while(running){auto now=GetTickCount64();if(now-check>=1000){auto current=process();check=now;if(current!=pid){finish();physics.close();graphics.close();info.close();pid=current;identity.clear();}}
    if(!pid){message(L"Esperando Assetto Corsa (2014). Cierra ACC si está abierto.");std::this_thread::sleep_for(std::chrono::milliseconds(100));continue;}
    physics.open(L"Local\\acpmf_physics",sizeof(Physics));graphics.open(L"Local\\acpmf_graphics",sizeof(Graphics));info.open(L"Local\\acpmf_static",sizeof(Static));
    Physics p{};Graphics g{};Static st{};
    if(!physics.read(p)||!graphics.read(g)||!info.data){message(L"Esperando memoria de AC...");physics.close();graphics.close();info.close();std::this_thread::sleep_for(std::chrono::milliseconds(100));continue;}
    memcpy(&st,info.data,sizeof st);auto car=label(st.car),track=label(st.track),layout=label(st.layout);auto key=car+"|"+track+"|"+layout;
    if(g.status!=2){message(g.status==1?L"Replay: captura suspendida, sesión conservada.":L"Pausa / transición: sesión conservada.");if(now-probe>=1000){physics.close();graphics.close();info.close();probe=now;}std::this_thread::sleep_for(std::chrono::milliseconds(50));continue;}
    if(!valid(p,g)||car.empty()||track.empty()){message(L"Muestra no válida; no se guarda.");std::this_thread::sleep_for(std::chrono::milliseconds(10));continue;}
    if(p.packet==packet){if(lastSample&&now-lastSample>2000){message(L"Lectura detenida; reabriendo memoria...");physics.close();graphics.close();info.close();}std::this_thread::sleep_for(std::chrono::milliseconds(10));continue;}
    bool reset=key!=identity||session!=g.session;
    if(reset){finish();++segment;}if(!csv.is_open()){
     opened=now;auto base=safeName(track)+"_"+safeName(layout)+"_AC_"+std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())+"-"+std::to_string(++segment);
     csv.open(directory/(base+".csv"),std::ios::out|std::ios::trunc);csv.imbue(std::locale::classic());csv<<std::setprecision(9);
     csv<<"capture_ms,physics_packet,graphics_packet,lap,lap_ms,last_lap_ms,progress,speed_kmh,gas,brake,steer_native,gear_native,rpm,fuel_l,acc_x_g,acc_y_g,acc_z_g,world_x,world_y,world_z,sector,last_sector_ms,in_pit,tyres_out,penalty_s,gap_ms,stint,raw_lap,track_id,car_id,layout_id,track_length_m,microsector_50,game_version,compound,abs_setting,tc_setting,auto_clutch,auto_blip,stability\n";
     identity=key;session=g.session;lastSample=0;count=0;
    }
    int recordedLap=tracker.update(g);
    csv<<now-opened<<','<<p.packet<<','<<g.packet<<','<<recordedLap<<','<<g.currentMs<<','<<g.lastMs<<','<<g.spline<<','<<p.speed<<','<<p.gas<<','<<p.brake<<','<<p.steer<<','<<p.gear<<','<<p.rpm<<','<<p.fuel;
    for(float a:p.acceleration)csv<<','<<a;for(float x:g.coordinates)csv<<','<<x;
    csv<<','<<g.sector<<','<<g.lastSector<<','<<(g.pit||g.pitLane)<<','<<p.tyresOut<<','<<g.penalty<<','<<(lastSample?now-lastSample:0)<<','<<tracker.stint<<','<<g.laps+1<<','<<encoded(track)<<','<<encoded(car)<<','<<encoded(layout)<<','<<(std::isfinite(st.length)?st.length:0)<<','<<std::min(50,int(g.spline*50)+1)<<','<<encoded(label(st.version))<<','<<encoded(label(g.compound))<<','<<p.abs<<','<<p.tc<<','<<st.autoClutch<<','<<st.autoBlip<<','<<st.stability<<'\n';
    packet=p.packet;lastSample=now;++count;
    if(now-displayAt>=100){std::lock_guard<std::mutex> lock(mutex);snapshot={wide(track),wide(car),recordedLap,g.currentMs,g.lastMs,p.gas,p.brake,p.speed,true};displayAt=now;}
    if(now-flush>=1000){csv.flush();if(!csv)throw std::runtime_error("disk");flush=now;double hz=now>opened?count*1000./(now-opened):0;
     message(L"GRABANDO · "+std::to_wstring(count)+L" muestras · "+std::to_wstring(int(hz))+L" Hz medios");}
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
   }finish();message(L"Grabación detenida y archivos guardados.");
  }catch(...){message(L"ERROR de captura / escritura. Grabación detenida. Comprueba espacio y permisos.");}running=false;
 }
 void start(){if(running)return;stop();running=true;worker=std::thread([this]{capture();});}
};
inline Studio studio;
inline void sessions(HWND owner){std::error_code ec;fs::create_directories(studio.directory,ec);ShellExecuteW(owner,L"open",studio.directory.c_str(),nullptr,nullptr,SW_SHOW);}
inline void analyze(HWND owner){wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);auto path=fs::path(exe).parent_path()/L"tools"/L"ac-analyzer.html";if(fs::exists(path))ShellExecuteW(owner,L"open",path.c_str(),nullptr,nullptr,SW_SHOW);else MessageBoxW(owner,L"Falta el visor. Ejecuta el instalador para repararlo.",L"LANDELTA",MB_OK|MB_ICONINFORMATION);}
}

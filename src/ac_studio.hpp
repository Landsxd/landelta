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
 HWND window=nullptr,statusLabel=nullptr;std::thread worker;std::atomic<bool> running{false};std::mutex mutex;
 std::wstring status=L"Listo. Abre AC (2014), entra a pista y pulsa Grabar.";fs::path directory;uint64_t count=0;
 void message(std::wstring s){std::lock_guard<std::mutex> lock(mutex);status=std::move(s);}
 void stop(){running=false;if(worker.joinable())worker.join();}
 ~Studio(){stop();}
 void capture(){
  Mapping physics,graphics,info;std::ofstream csv;DWORD pid=0;uint64_t check=0,flush=0,lastSample=0,opened=0,probe=0;
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
     csv<<"capture_ms,physics_packet,graphics_packet,lap,lap_ms,last_lap_ms,progress,speed_kmh,gas,brake,steer_native,gear_native,rpm,fuel_l,acc_x_g,acc_y_g,acc_z_g,world_x,world_y,world_z,sector,last_sector_ms,in_pit,tyres_out,penalty_s,gap_ms,stint,raw_lap,track_id,car_id,layout_id,track_length_m,microsector_50\n";
     identity=key;session=g.session;lastSample=0;count=0;
    }
    int recordedLap=tracker.update(g);
    csv<<now-opened<<','<<p.packet<<','<<g.packet<<','<<recordedLap<<','<<g.currentMs<<','<<g.lastMs<<','<<g.spline<<','<<p.speed<<','<<p.gas<<','<<p.brake<<','<<p.steer<<','<<p.gear<<','<<p.rpm<<','<<p.fuel;
    for(float a:p.acceleration)csv<<','<<a;for(float x:g.coordinates)csv<<','<<x;
    csv<<','<<g.sector<<','<<g.lastSector<<','<<(g.pit||g.pitLane)<<','<<p.tyresOut<<','<<g.penalty<<','<<(lastSample?now-lastSample:0)<<','<<tracker.stint<<','<<g.laps+1<<','<<encoded(track)<<','<<encoded(car)<<','<<encoded(layout)<<','<<(std::isfinite(st.length)?st.length:0)<<','<<std::min(50,int(g.spline*50)+1)<<'\n';
    packet=p.packet;lastSample=now;++count;
    if(now-flush>=1000){csv.flush();if(!csv)throw std::runtime_error("disk");flush=now;double hz=now>opened?count*1000./(now-opened):0;
     message(L"GRABANDO · "+wide(track)+L" · "+wide(car)+L"\nVuelta "+std::to_wstring(g.laps+1)+L" · "+std::to_wstring(g.currentMs/1000.)+L" s\n"+std::to_wstring(count)+L" muestras · "+std::to_wstring(int(hz))+L" Hz medios (incluye pausas)\nGas "+std::to_wstring(int(p.gas*100))+L"% · Freno "+std::to_wstring(int(p.brake*100))+L"% · "+std::to_wstring(int(p.speed))+L" km/h");}
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
   }finish();message(L"Grabación detenida y archivos guardados.");
  }catch(...){message(L"ERROR de captura / escritura. Grabación detenida. Comprueba espacio y permisos.");}running=false;
 }
 void start(){if(running)return;stop();running=true;worker=std::thread([this]{capture();});}
};
inline Studio studio;
inline LRESULT CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l){switch(m){
 case WM_CREATE:{auto inst=GetModuleHandleW(nullptr);studio.statusLabel=CreateWindowW(L"STATIC",L"",WS_CHILD|WS_VISIBLE,24,24,720,150,h,nullptr,inst,nullptr);
  CreateWindowW(L"BUTTON",L"Grabar AC",WS_CHILD|WS_VISIBLE,24,190,150,40,h,(HMENU)1,inst,nullptr);
  CreateWindowW(L"BUTTON",L"Detener y guardar",WS_CHILD|WS_VISIBLE,184,190,170,40,h,(HMENU)2,inst,nullptr);
  CreateWindowW(L"BUTTON",L"Abrir sesiones",WS_CHILD|WS_VISIBLE,364,190,160,40,h,(HMENU)3,inst,nullptr);
  CreateWindowW(L"BUTTON",L"Analizar vueltas",WS_CHILD|WS_VISIBLE,534,190,180,40,h,(HMENU)4,inst,nullptr);
  CreateWindowW(L"STATIC",L"AC Studio · sesión única\n\n1. Abre AC (no ACC), entra a pista y pulsa Grabar.\n2. Conduce; se guardan todas las muestras nuevas, incluso vueltas parciales.\n3. Detén la captura. Abre Analizar vueltas y carga el CSV.\n\nEl análisis es local. Comparación por distancia, pedales, velocidad y microsectores.\nAdjunta el CSV al chat: no necesitas grabar video para analizar estos canales.\n\nMinimizar o cerrar esta ventana mantiene la captura. Para finalizar usa Detener.\nSalir de LANDELTA también guarda y detiene la captura.",WS_CHILD|WS_VISIBLE,24,255,740,250,h,nullptr,inst,nullptr);SetTimer(h,1,500,nullptr);return 0;}
 case WM_TIMER:{std::lock_guard<std::mutex> lock(studio.mutex);SetWindowTextW(studio.statusLabel,studio.status.c_str());return 0;}
 case WM_COMMAND:if(LOWORD(w)==1)studio.start();if(LOWORD(w)==2)studio.stop();if(LOWORD(w)==3){std::error_code ec;fs::create_directories(studio.directory,ec);ShellExecuteW(h,L"open",studio.directory.c_str(),nullptr,nullptr,SW_SHOW);}if(LOWORD(w)==4){wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);auto path=fs::path(exe).parent_path()/L"tools"/L"ac-analyzer.html";if(fs::exists(path))ShellExecuteW(h,L"open",path.c_str(),nullptr,nullptr,SW_SHOW);else MessageBoxW(h,L"Falta el visor de telemetría. Vuelve a ejecutar el instalador de LANDELTA para repararlo.",L"AC Studio",MB_OK|MB_ICONINFORMATION);}return 0;
 case WM_CLOSE:ShowWindow(h,SW_HIDE);return 0;
 }return DefWindowProcW(h,m,w,l);}
inline void open(const fs::path& data){if(!studio.window){studio.directory=data/L"ac-telemetry";WNDCLASSW c{};c.lpfnWndProc=proc;c.hInstance=GetModuleHandleW(nullptr);c.lpszClassName=L"LANDELTAACStudio";c.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);c.hCursor=LoadCursorW(nullptr,IDC_ARROW);RegisterClassW(&c);studio.window=CreateWindowW(c.lpszClassName,L"LANDELTA · AC Studio",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,820,600,nullptr,nullptr,c.hInstance,nullptr);}ShowWindow(studio.window,SW_RESTORE);SetForegroundWindow(studio.window);}
}

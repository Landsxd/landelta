"""Test actual visibility/timer functions with Win32 windows simulated."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/main.cpp').read_text()
shim=r'''
#include "runtime.hpp"
#include "background.hpp"
#include <cassert>
#include <iostream>
using HWND=int;using DWORD=unsigned;struct RECT{int left,top,right,bottom;};
const int WS_EX_TOPMOST=1,WS_EX_TOOLWINDOW=2,WS_EX_LAYERED=4,WS_EX_NOACTIVATE=8,WS_EX_TRANSPARENT=16,GWL_EXSTYLE=1,HWND_TOPMOST=1,SWP_NOACTIVATE=2,SWP_FRAMECHANGED=4,SWP_NOMOVE=8,SWP_NOSIZE=16,SW_SHOWNOACTIVATE=4,SW_HIDE=0,FALSE=0,SM_XVIRTUALSCREEN=1,SM_YVIRTUALSCREEN=2,SM_CXVIRTUALSCREEN=3,SM_CYVIRTUALSCREEN=4;
'''+s[s.index('constexpr int PanelCount'):s.index('Config cfg;')]+r'''
uint64_t udpGeneration=0;Config cfg;acc::State live;acc::Graphics shared{};acc::SessionVisibility sessionVisibility;acc::SessionLaunch sessionLaunch;
HWND control=8,panels[PanelCount]={1,2,3,4,5,6,7};bool windows[9]{},sharedOffline=false,sharedOK=false,demoMode=false,gameRunning=false,locked=false,pedalTimer=false;unsigned mainInterval=0,mainTimer=0,inputTimer=0;int shows=0;
bool paused(){return sharedOK&&shared.status==3;}
int panelW(int){return 300;}int panelH(int){return 100;}
void presentOverlay(HWND,int){}
void SetWindowLongPtrW(int,int,int){}
int GetSystemMetrics(int field){return field==3?1920:(field==4?1080:0);}
void SetWindowPos(int,int,int,int,int,int,int flags){assert(flags&SWP_NOACTIVATE);}
void InvalidateRect(int,void*,int){}
int IsWindowVisible(int w){return windows[w];}
void ShowWindow(int w,int mode){assert(mode==SW_HIDE||mode==SW_SHOWNOACTIVATE);windows[w]=mode!=SW_HIDE;++shows;}
void SetTimer(int,int id,unsigned interval,void*){(id==1?mainTimer:inputTimer)=interval;}
void KillTimer(int,int id){(id==1?mainTimer:inputTimer)=0;}
void adjustPolling();bool hideControl(){windows[8]=false;return true;}
'''
functions=s[s.index('void updateWindows(){'):s.index('NOTIFYICONDATAW trayData')]
cases=r'''
int main(){
 updateWindows();assert(!windows[1]&&!windows[4]&&mainTimer==1000&&inputTimer==0&&!windows[8]);
 cfg.hidden=true;gameRunning=true;syncSessionVisibility(100);assert(!windows[1]&&mainTimer==100);
 sharedOK=true;shared.status=2;shared.sessionIndex=1;syncSessionVisibility(1000);assert(windows[1]&&windows[4]&&!windows[7]&&inputTimer==33&&locked&&!windows[8]);
 cfg.hidden=true;updateWindows();assert(!windows[1]&&!windows[4]&&inputTimer==0);syncSessionVisibility(1050);assert(!windows[1]);cfg.hidden=false;updateWindows();assert(windows[1]);
 sharedOK=false;syncSessionVisibility(2000);assert(windows[1]);syncSessionVisibility(4050);assert(!windows[1]&&inputTimer==0);
 sharedOK=true;syncSessionVisibility(4100);assert(windows[1]);shared.status=3;syncSessionVisibility(200000);assert(windows[1]);
 cfg.hidden=true;updateWindows();shared.session=2;shared.sessionIndex=2;syncSessionVisibility(200050);assert(windows[1]&&!cfg.hidden&&!windows[8]);
 cfg.hidden=true;updateWindows();shared.laps=2;syncSessionVisibility(200060);assert(!windows[1]);shared.laps=0;syncSessionVisibility(200070);assert(windows[1]);
 shared.currentMs=5000;syncSessionVisibility(200072);cfg.hidden=true;updateWindows();shared.currentMs=0;syncSessionVisibility(200075);assert(windows[1]);
 cfg.autoShow=false;cfg.hidden=true;updateWindows();shared.sessionIndex=3;syncSessionVisibility(200080);assert(!windows[1]&&cfg.hidden);cfg.autoShow=true;cfg.hidden=false;
 sharedOffline=true;syncSessionVisibility(200100);assert(!windows[1]&&!windows[4]);gameRunning=false;syncSessionVisibility(200200);assert(mainTimer==1000&&inputTimer==0);
 gameRunning=true;sharedOffline=false;sharedOK=false;live.registered=live.hasSession=true;live.phase=2;live.sessionReceived=201000;live.session=8;syncSessionVisibility(201001);assert(windows[1]&&live.cars.empty());
 cfg.hidden=true;updateWindows();syncSessionVisibility(201100);assert(!windows[1]);live.session=9;syncSessionVisibility(201200);assert(windows[1]);
 live.elapsed=30000;syncSessionVisibility(201210);cfg.hidden=true;updateWindows();live.elapsed=0;syncSessionVisibility(201220);assert(windows[1]);
 cfg.hidden=true;updateWindows();++udpGeneration;syncSessionVisibility(201250);assert(windows[1]);
 gameRunning=false;syncSessionVisibility(201300);demoMode=true;updateWindows();assert(windows[1]&&inputTimer==33&&mainTimer==100);demoMode=false;updateWindows();assert(!windows[1]&&inputTimer==0&&mainTimer==1000);
 cfg.keepAfterRace=true;updateWindows();assert(windows[5]);cfg.hidden=true;updateWindows();assert(!windows[5]);cfg.hidden=false;cfg.visible[4]=false;updateWindows();assert(!windows[5]);
 std::cout<<"PASS: production visibility, idle timers, input timer stop/start, session lock, manual hide, reconnect, pause, menu, demo; no control focus or window activation\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(shim+functions+cases)
 subprocess.run(['g++','-std=c++17','-I',str(root/'src'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)

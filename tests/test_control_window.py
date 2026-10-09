"""Exercise production control-window events with a simulated Windows shell."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/main.cpp').read_text()
shim=r'''
#include "control_layout.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
using HWND=int;using UINT=unsigned;using WPARAM=unsigned long;using LPARAM=long;using DWORD=unsigned;
struct RECT{int left,top,right,bottom;};struct POINT{int x,y;};struct MINMAXINFO{POINT ptReserved{},ptMaxSize{},ptMaxPosition{},ptMinTrackSize{},ptMaxTrackSize{};};
const int WM_GETMINMAXINFO=1,WM_SIZE=2,WM_CLOSE=3,WM_SYSCOMMAND=4,SC_MINIMIZE=0xF020,SIZE_MINIMIZED=1,SIZE_MAXIMIZED=2,SIZE_RESTORED=0,GWL_STYLE=1,GWL_EXSTYLE=2,FALSE=0,SW_MINIMIZE=6;
'''+s[s.index('constexpr int PanelCount'):s.index('Config cfg;')]+r'''
Config cfg;HWND control=1;RECT client{0,0,656,680};bool trayWorks=true;int trayCalls=0,minimizes=0,saves=0,redraws=0;
long GetWindowLongPtrW(HWND,int){return 0;}
void AdjustWindowRectEx(RECT* r,DWORD,int,DWORD){r->right+=16;r->bottom+=39;}
void GetClientRect(HWND,RECT* r){*r=client;}
void InvalidateRect(HWND,void*,int){redraws++;}
void saveSettings(){saves++;}
bool waitInTray(){trayCalls++;return trayWorks;}
void ShowWindow(HWND,int mode){assert(mode==SW_MINIMIZE);minimizes++;}
'''
handler=s[s.index('bool controlWindowMessage'):s.index('LRESULT CALLBACK wndProc')]
cases=r'''
void layout(int width,int height){acc::ControlLayout t(width,height);assert(t.scale>0);float x=0,y=0;
 assert(t.point(t.x+500*t.scale,t.y+570*t.scale,x,y));assert(std::abs(x-500)<.001&&std::abs(y-570)<.001);
 assert(!t.point(t.x-1,t.y,x,y));assert(!t.point(t.x,t.y-1,x,y));assert(!t.point(t.x+656*t.scale,t.y,x,y));
 assert(std::abs(656*t.scale+2*t.x-width)<.001&&std::abs(680*t.scale+2*t.y-height)<.001);}
int main(){
 for(auto size:{std::pair<int,int>{492,510},{656,680},{1920,1041},{3840,2082},{600,1000},{1200,600}})layout(size.first,size.second);
 float x,y;assert(!acc::ControlLayout(0,0).point(0,0,x,y));
 MINMAXINFO info;assert(controlWindowMessage(1,WM_GETMINMAXINFO,0,reinterpret_cast<LPARAM>(&info)));assert(info.ptMinTrackSize.x==508&&info.ptMinTrackSize.y==549);
 cfg.controlMaximized=true;assert(!controlWindowMessage(9,WM_SIZE,SIZE_RESTORED,0));assert(cfg.controlMaximized); // create-time messages don't discard saved maximize
 assert(!controlWindowMessage(1,WM_SYSCOMMAND,SC_MINIMIZE,0));assert(trayCalls==0); // DefWindowProc minimizes to taskbar
 assert(!controlWindowMessage(1,WM_SIZE,SIZE_MINIMIZED,0));assert(cfg.controlMaximized&&cfg.controlWidth==656&&cfg.controlHeight==680&&redraws==0);
 client={0,0,1920,1041};assert(!controlWindowMessage(1,WM_SIZE,SIZE_MAXIMIZED,0));assert(cfg.controlMaximized&&cfg.controlWidth==656&&cfg.controlHeight==680);
 client={0,0,900,800};assert(!controlWindowMessage(1,WM_SIZE,SIZE_RESTORED,0));assert(!cfg.controlMaximized&&cfg.controlWidth==900&&cfg.controlHeight==800);
 assert(controlWindowMessage(1,WM_CLOSE,0,0));assert(trayCalls==1&&minimizes==0);
 trayWorks=false;assert(controlWindowMessage(1,WM_CLOSE,0,0));assert(trayCalls==2&&minimizes==1); // failed tray remains reachable on taskbar
 std::cout<<"PASS: proportional sizing / click transform, maximize persistence, resize bounds, taskbar minimize, tray close and tray failure fallback\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(shim+handler+cases)
 subprocess.run(['g++','-std=c++17','-I',str(root/'src'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)

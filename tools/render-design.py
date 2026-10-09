"""Render the actual C++ UI drawing functions to SVG for visual layout checks.
This is a design preview, not a Windows/game screenshot.
Requires g++ and cairosvg (for PNG export); runtime app does not require these.
"""
from pathlib import Path
import subprocess
import tempfile
import cairosvg
root=Path(__file__).resolve().parents[1]
s=(root/'src/main.cpp').read_text()
header=r'''
#include "runtime.hpp"
#include "relatives.hpp"
#include "driving.hpp"
#include "history.hpp"
#include <ctime>
#include <sstream>
#include <iostream>
#include <fstream>
#include <codecvt>
#include <locale>
#include <filesystem>
using namespace std;
namespace fs=std::filesystem;
struct Color {int a,r,g,b;Color(int aa,int rr,int gg,int bb):a(aa),r(rr),g(gg),b(bb){};string css()const{char h[10];snprintf(h,10,"#%02x%02x%02x",r,g,b);return h;}};
struct RectF {float x,y,w,h;RectF(float a,float b,float c,float d):x(a),y(b),w(c),h(d){}};
enum {FontStyleRegular=0,FontStyleBold=1,StringAlignmentNear=0,StringAlignmentCenter=1,StringAlignmentFar=2};
using StringAlignment=int;
struct Graphics {ostringstream out;float tx=0,ty=0;int clip=0;void Clear(Color c){out<<"<rect width='1280' height='900' fill-opacity='"<<c.a/255.0<<"' fill='"<<c.css()<<"'/>";}void TranslateTransform(float x,float y){tx+=x;ty+=y;}void ResetTransform(){tx=ty=0;}};
string escape(const string&s){string r;for(auto c:s){if(c=='&')r+="&amp;";else if(c=='<')r+="&lt;";else if(c=='>')r+="&gt;";else if(c=='\"')r+="&quot;";else r+=c;}return r;}
wstring wide(const string&s){return wstring_convert<codecvt_utf8_utf16<wchar_t>>().from_bytes(s);}
string utf8(const wstring&s){return wstring_convert<codecvt_utf8_utf16<wchar_t>>().to_bytes(s);}
'''
colors=s[s.index('const Color BG'):s.index('std::wstring wide')]
fmt=s[s.index('std::wstring num'):s.index('bool fileWrite')]
cfg=s[s.index('constexpr int PanelCount'):s.index('Config cfg;')]
globals=r'''
struct StartupDisplay{bool exists=false,current=false;wstring error;};StartupDisplay startupInfo;wstring startupNotice,hotkeyIssue;
bool analyticsTrackReady=false;acc::LapDelta lapDelta;acc::DeltaSnapshot retainedDelta;acc::PedalState pedals;acc::Analytics analytics,demoAnalytics;acc::HistoryStore historyStore;int historyView=0,historyOffset=0,historyTotal=0,historyPageSize=4;string historyTrack,historySelected;vector<string> historyTargets;
std::array<uint64_t,9> udpReceived{};
Config cfg;acc::State live,demo;acc::Graphics shared{};acc::PlayerTiming playerTiming;acc::ReconnectPolicy retry;bool sharedOK=false,sharedOffline=false,demoMode=true,locked=false,gameRunning=false,configReady=true;int page=0,port=9000,scroll=-1;vector<pair<RectF,int>> hits;wstring status=L"DEMO · datos simulados",configStatus=L"ACC preparado",networkError;fs::path configPath;
uint64_t nowMs(){return 25000;}
'''
data=s[s.index('void generateDemo'):s.index('void fill')]
helpers=r'''
void fill(Graphics&g,float x,float y,float w,float h,Color c){int alpha=renderingOverlay&&w>5&&h>5?cfg.opacity:c.a;g.out<<"<rect x='"<<x+g.tx<<"' y='"<<y+g.ty<<"' width='"<<w<<"' height='"<<h<<"' fill-opacity='"<<alpha/255.0<<"' fill='"<<c.css()<<"'/>";}
void text(Graphics&g,const wstring&s,float x,float y,float w,float h,float size=14,Color c=FG,int weight=0,StringAlignment align=0){int id=++g.clip;g.out<<"<defs><clipPath id='c"<<id<<"'><rect x='"<<x+g.tx<<"' y='"<<y+g.ty<<"' width='"<<w<<"' height='"<<h<<"'/></clipPath></defs>";float a=align==2?w:(align==1?w/2:0);g.out<<"<text clip-path='url(#c"<<id<<")' x='"<<x+g.tx+a<<"' y='"<<y+g.ty+h/2+size*.35<<"' text-anchor='"<<(align==2?"end":align==1?"middle":"start")<<"' font-family='DejaVu Sans,sans-serif' font-size='"<<size<<"' font-weight='"<<(weight?700:400)<<"' fill='"<<c.css()<<"'>"<<escape(utf8(s))<<"</text>";}
void line(Graphics&g,float x,float y,float w,Color c=EDGE){g.out<<"<path d='M "<<x+g.tx<<" "<<y+g.ty<<" h "<<w<<"' stroke='"<<c.css()<<"'/>";}
void tag(Graphics&g,const wstring&s,float x,float y,float w,Color c=GREEN){fill(g,x,y,w,23,Color(255,31,52,47));text(g,s,x,y,w,23,11,c,1,1);}
void button(Graphics&g,const wstring&s,float x,float y,float w,float h,int,bool accent=false,bool active=false){fill(g,x,y,w,h,accent?GREEN:(active?Color(255,33,67,57):CARD));text(g,s,x+4,y,w-8,h,13,accent?BG:(active?GREEN:FG),1,1);}
void section(Graphics&g,const wstring&s,float y){text(g,s,28,y,600,25,11,MUTED,1);}
'''
draw=s[s.index('void renderHistoryImage'):s.index('// Per-pixel alpha')]
main=r'''
int main(int argc,char**argv){generateDemo(nowMs());Graphics g;g.out<<"<svg xmlns='http://www.w3.org/2000/svg' width='1280' height='900' viewBox='0 0 1280 900'>";g.Clear(Color(0,0,0,0));text(g,L"LANDELTA / COMPACT",28,14,1100,47,26,FG,1);text(g,L"BETA 0.11    ·    FONDO 25%    ·    DATOS SIMULADOS",30,61,1100,25,12,MUTED);renderingOverlay=true;const int x[]={28,668,28,292,668,292,668},y[]={105,105,477,477,361,583,461};for(int i=0;i<PanelCount;i++){g.TranslateTransform(x[i],y[i]);renderPanel(g,i);g.ResetTransform();}renderingOverlay=false;g.out<<"</svg>";ofstream(argv[1])<<g.out.str();Graphics c;c.out<<"<svg xmlns='http://www.w3.org/2000/svg' width='656' height='680'>";c.Clear(BG);renderControl(c);c.out<<"</svg>";ofstream(argv[2])<<c.out.str();page=3;Graphics h;h.out<<"<svg xmlns='http://www.w3.org/2000/svg' width='656' height='680'>";h.Clear(BG);renderControl(h);h.out<<"</svg>";ofstream(argv[3])<<h.out.str();historyView=2;historySelected=demoAnalytics.history[0].id;Graphics l;l.out<<"<svg xmlns='http://www.w3.org/2000/svg' width='656' height='680'>";l.Clear(BG);renderControl(l);l.out<<"</svg>";ofstream(argv[4])<<l.out.str();Graphics e;e.out<<"<svg xmlns='http://www.w3.org/2000/svg' width='1200' height='658'>";e.Clear(BG);renderHistoryImage(e,demoAnalytics.history[0],0);e.out<<"</svg>";ofstream("Vista-export.svg")<<e.out.str();page=1;Graphics n;n.out<<"<svg xmlns='http://www.w3.org/2000/svg' width='656' height='680'>";n.Clear(BG);renderControl(n);n.out<<"</svg>";ofstream(argv[5])<<n.out.str();page=4;Graphics a;a.out<<"<svg xmlns='http://www.w3.org/2000/svg' width='656' height='680'>";a.Clear(BG);renderControl(a);a.out<<"</svg>";ofstream(argv[6])<<a.out.str();}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'render.cpp').write_text(header+colors+fmt+cfg+globals+data+helpers+draw+main)
 subprocess.run(['g++','-std=c++17','-I',str(root/'src'),'-I',str(root/'vendor'),str(p/'render.cpp'),'-o',str(p/'render')],check=True)
 subprocess.run([str(p/'render'),str(root/'Vista-demo.svg'),str(root/'Vista-control.svg'),str(root/'Vista-historial.svg'),str(root/'Vista-sesion.svg'),str(root/'Vista-conexion.svg'),str(root/'Vista-inicio.svg')],check=True)
for name in ['Vista-export','Vista-demo','Vista-control','Vista-historial','Vista-sesion','Vista-conexion','Vista-inicio']:
 cairosvg.svg2png(url=str(root/(name+'.svg')),write_to=str(root/(name+'.png')))
print('Preview rendered from original C++ UI functions')

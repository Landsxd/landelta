"""Exercise the production image renderer with captured text instead of GDI+."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/main.cpp').read_text()
renderer=s[s.index('void renderHistoryImage'):s.index('void renderHistory(Graphics')]
shim=r'''
#include "analytics.hpp"
#include <cassert>
#include <iostream>
#include <string>
struct Color{int id;};const Color GREEN{1},FG{2},MUTED{3},GOLD{4},BG{5},CARD{6};const int FontStyleBold=1;
struct Graphics{std::vector<std::wstring> labels;};bool demoMode=false;
std::wstring wide(const std::string&s){return {s.begin(),s.end()};}
std::wstring sessionName(int){return L"CARRERA";}
std::wstring historyDate(int64_t){return L"FECHA";}
std::wstring lapTime(int ms){return wide(acc::formatTimeMs(ms));}
std::wstring sectorTime(int ms){return lapTime(ms);}
void fill(Graphics&,float,float,float,float,Color){}
void text(Graphics&g,const std::wstring&s,float,float,float,float,float,Color,int=0){g.labels.push_back(s);}
'''
cases=r'''
bool has(const Graphics&g,const std::wstring&s){return std::find(g.labels.begin(),g.labels.end(),s)!=g.labels.end();}
int main(){acc::SessionRecord record;record.track="Circuito de prueba";record.driver="Piloto demo";
 for(int i=1;i<=41;i++){acc::RecordedLap lap;lap.number=i;lap.complete=true;lap.timing.valid=true;lap.timing.ms=100000+i;lap.timing.splits={65034,20000,15000};record.laps.push_back(lap);}
 record.laps[0].timing.invalid=true;record.laps[0].timing.ms=80000;
 Graphics first,last;renderHistoryImage(first,record,0);renderHistoryImage(last,record,1);
 assert(has(first,L"1")&&has(first,L"40")&&!has(first,L"41"));assert(has(last,L"41")&&!has(last,L"40"));
 assert(has(first,L"1:05.034")&&has(first,L"INVÁLIDA"));assert(has(first,L"FECHA · Mejor limpia 1:40.002 · 1 / 2"));
 assert(has(last,L"FECHA · Mejor limpia 1:40.002 · 2 / 2"));
 record.laps.resize(40);Graphics exact;renderHistoryImage(exact,record,0);assert(has(exact,L"FECHA · Mejor limpia 1:40.002 · 1 / 1"));
 record.laps.clear();Graphics empty;renderHistoryImage(empty,record,0);assert(!has(empty,L"LIMPIA"));
 std::cout<<"PASS: production history image columns, 40/41 lap pagination, minute sectors, invalid flags and clean best\n";}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(shim+renderer+cases)
 subprocess.run(['g++','-std=c++17','-I',str(root/'src'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)

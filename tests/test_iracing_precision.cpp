#include "iracing.hpp"
#include "monitors.hpp"
#include <cassert>
#include <iostream>
void value(iracing::Packet& p,const std::string& name,double n){auto it=p.vars.find(name);if(it==p.vars.end()){iracing::Variable v{};v.type=5;v.count=1;v.offset=int(p.data.size());p.data.resize(p.data.size()+8);it=p.vars.emplace(name,v).first;}memcpy(p.data.data()+it->second.offset,&n,8);}
int main(){
 iracing::Packet p;value(p,"LapDeltaToSessionBestLap",-.321);value(p,"LapDeltaToSessionBestLap_OK",1);value(p,"LapBestLapTime",92.5);value(p,"LapDistPct",.4);value(p,"Lap",2);
 assert(std::abs(iracing::sessionDelta(p,true)+321)<.001);
 value(p,"LapDeltaToSessionBestLap_OK",0);assert(std::isnan(iracing::sessionDelta(p,true)));
 value(p,"LapDeltaToBestLap",-44);value(p,"LapDeltaToBestLap_OK",1);assert(std::isnan(iracing::sessionDelta(p,true))); // no alternate reference
 value(p,"LapDeltaToSessionBestLap_OK",1);value(p,"OnPitRoad",1);assert(std::isnan(iracing::sessionDelta(p,true)));
 value(p,"OnPitRoad",0);value(p,"IsReplayPlaying",1);assert(std::isnan(iracing::sessionDelta(p,true)));
 value(p,"IsReplayPlaying",0);value(p,"LapBestLapTime",0);assert(std::isnan(iracing::sessionDelta(p,true)));
 value(p,"LapBestLapTime",92.5);value(p,"SessionState",3);assert(std::isnan(iracing::sessionDelta(p,true)));
 value(p,"SessionState",4);value(p,"LapDeltaToSessionBestLap",NAN);assert(std::isnan(iracing::sessionDelta(p,true)));
 race::Sample sample;sample.delta=-321;assert(iracing::visibleDelta(sample,true,1010,1000)==-321);assert(std::isnan(iracing::visibleDelta(sample,true,1300,1000)));assert(std::isnan(iracing::visibleDelta(sample,false,1010,1000)));sample.pit=true;assert(std::isnan(iracing::visibleDelta(sample,true,1010,1000)));
 iracing::Quality q;for(int i=0;i<=60;++i)q.add(i,1000+uint64_t(i*1000./60),60);assert(q.hz==60&&q.missed==0);q.add(60,2010,60);q.add(63,2050,60);assert(q.missed==2);
 std::vector<race::Monitor> monitors={{L"main",0,0,1920,1080,true},{L"left",-2560,-200,2560,1440,false},{L"portrait",1920,0,1080,1920,false}};
 assert(race::monitorFor(monitors,-2400,100,600,200)==1);assert(race::monitorFor(monitors,2300,100,600,200)==2);
 auto pos=race::contain(monitors[1],-20,1300,600,200);assert(pos.first==-600&&pos.second==1040);
 race::ScreenTransform t{-2560,-200,2560,1440,228,234,928,340};auto mini=t.toMini(-2400,100);auto desktop=t.toDesktop(mini.first,mini.second,600,200);assert(desktop.first==-2400&&desktop.second==100);
 acc::State state;acc::Car me,other;me.id=0;other.id=1;me.spline=.2;other.spline=.3;me.best.ms=100000;me.location=other.location=1;
 assert(std::isfinite(state.relativeEstimate(other,me)));assert(std::isnan(state.relativeEstimate(other,me,false)));
 std::cout<<"PASS iRacing strict delta, no-reference/pit/replay/stale guards, measured tick cadence, conservative gaps and independent monitors\n";
}

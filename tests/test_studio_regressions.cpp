#include "race_studio.hpp"
#include <cassert>
#include <iostream>

race::Sample point(int lap,int ms,uint64_t at,double progress,int last,int packet){
 race::Sample p;p.lap=lap;p.ms=ms;p.at=at;p.progress=progress;p.last=last;p.packet=packet;p.valid=1;return p;
}
int main(){
 // First packet after meta still carries V1's 100.100, not V2's 100.000.
 race::Session s;s.samples={point(2,0,0,0,100100,0),point(2,99900,99900,.999,100100,1),point(3,0,100000,0,100100,2)};
 race::analyze(s);assert(!s.laps[0].complete);
 s.samples.push_back(point(3,100,100100,.001,100000,3));
 race::analyze(s);assert(s.laps[0].complete&&s.laps[0].ms==100000);
 // Truly identical times are accepted only once stable and finish-compatible.
 s.samples[0].last=s.samples[1].last=s.samples[2].last=100000;
 s.samples.pop_back();race::analyze(s);assert(!s.laps[0].complete);
 s.samples.push_back(point(3,600,100600,.006,100000,3));race::analyze(s);assert(s.laps[0].ms==100000);
 // A stale, much slower previous time must never be reused, even if stable.
 for(auto& p:s.samples)p.last=120000;
 race::analyze(s);assert(!s.laps[0].complete);
 // Sector result can arrive after the sector index, but only latched once.
 race::Session sectors;for(int i=0;i<4;++i){auto p=point(1,30000+i*100,1000+i*100,.3+i*.001,0,i);p.sector=i?1:0;p.lastSector=i<2?32000:(i==2?30100:30500);sectors.samples.push_back(p);}
 race::analyze(sectors);assert(sectors.laps[0].sectors[0]==30100);
 sectors.samples[2].at=2000;sectors.samples[3].at=2100;race::analyze(sectors);assert(sectors.laps[0].sectors[0]==0);
 // Missing time at a lap boundary counts as a gap; no synthetic microsector.
 s.samples[2].at=110000;s.samples[3].at=110600;race::analyze(s);assert(s.gaps==3&&std::isnan(s.laps[0].micro[9]));
 race::Lap partial;partial.ms=80000;partial.complete=true;s.laps={partial};assert(s.best()==0);s.laps[0].partial=false;assert(s.best()==80000);
 // A slow file read cannot override a later game switch or return to live.
 race::ReviewRequest request;request.begin(1);assert(request.accepts(1)&&!request.accepts(2));request.cancel();assert(!request.accepts(1));request.begin(0);assert(request.accepts(0));
 auto folder=std::filesystem::temp_directory_path()/"landelta-studio-regressions";
 std::filesystem::create_directories(folder);
 race::Recorder r;auto a=point(1,0,100,0,0,1);a.delta=-123.5;a.best=100000;a.stint=4;
 r.add(a,"A","iRacing","Test track","Car",folder,123);
 auto b=point(1,99900,300,.999,0,2);b.stint=4;r.add(b,"A","iRacing","Test track","Car",folder,123);
 auto c=point(2,0,400,0,100000,3);c.stint=4;r.add(c,"A","iRacing","Test track","Car",folder,123);
 assert(r.session.laps.size()==1); // analysis was throttled
 r.pause();assert(r.session.laps.size()==2&&r.session.laps[0].complete&&!r.online);
 auto path=r.session.path;r.close();auto loaded=race::loadCSV(path);
 assert(loaded.samples.size()==3&&loaded.samples[0].delta==-123.5&&loaded.samples[0].best==100000&&loaded.samples[0].stint==4);
 // A new short capture clock must reset the one-second analysis timer.
 a.at=0;r.add(a,"B","AC","Test track","Car",folder,124);a.at=200;a.packet=2;r.add(a,"B","AC","Test track","Car",folder,124);auto second=r.session.path;r.close();
 std::filesystem::remove(path);std::filesystem::remove(second);std::filesystem::remove(folder);
 std::cout<<"PASS Studio regressions: delayed official time, same-time laps, late sectors, gaps, partial best, canceled review, final analysis and telemetry roundtrip\n";
}

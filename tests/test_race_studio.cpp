#include "race_studio.hpp"
#include <cassert>
#include <iostream>
int main(){
 assert((race::observedSplits(2,3,94000,100,0,{30000,31000,33000})==std::array<int,3>{}));assert((race::observedSplits(3,3,32000,32100,1,{30000,31000,33000})==std::array<int,3>{30000,0,0}));
 race::Sample before,after;before.lap=after.lap=1;before.ms=90000;after.ms=100;before.progress=.99;after.progress=.01;assert(!race::restarted(before,after));before.progress=.5;assert(race::restarted(before,after));
 race::Session s;for(int lap=1;lap<=3;++lap)for(int j=0;j<1000;++j){race::Sample p;p.at=100+(lap-1)*100000+j*100;p.packet=int(p.at);p.lap=lap;p.ms=j*100;p.last=lap>1?100000:0;p.progress=j/1000.;p.valid=1;p.gas=.5;p.brake=0;p.steer=.1;p.sector=j<330?0:j<660?1:2;p.lastSector=j<330?0:(j<660?33000:33001);s.samples.push_back(p);}
 race::analyze(s);assert(s.laps.size()==3&&s.laps[0].complete&&s.laps[1].complete&&!s.laps[2].complete);assert(s.laps[0].ms==100000);assert(s.laps[0].sectors[0]==33000&&s.laps[0].sectors[2]==33999);assert(s.laps[0].micro[0]==10000&&s.laps[0].micro[9]==10000);assert(std::abs(race::localDelta(s))<.01);
 s.samples[1000].at+=1000;s.samples[1000].progress=.001;s.samples[1000].ms=100;race::analyze(s);assert(std::isnan(s.laps[0].micro[9]));assert(std::isnan(s.laps[1].micro[0]));
 race::Session partial;partial.samples.assign(s.samples.begin()+100,s.samples.begin()+999);race::analyze(partial);assert(partial.laps[0].partial&&!partial.laps[0].complete&&std::isnan(partial.laps[0].micro[0]));
 race::ScreenTransform t{-1920,0,3840,1080,228,224,928,365};auto p=t.toMini(-1500,200);auto q=t.toDesktop(p.first,p.second,600,200);assert(q.first==-1500&&q.second==200);auto off=t.toDesktop(-9999,-9999,600,200);assert(off.first==-1920&&off.second==0);
 assert(race::decode(race::encode("Nürburgring GP, layout"))=="Nürburgring GP, layout");assert(race::cells("a,b\r")[1]=="b");
 auto folder=std::filesystem::temp_directory_path()/"landelta-studio-test";std::filesystem::create_directories(folder);
 std::filesystem::path saved;{race::Recorder r;auto a=s.samples[0];r.add(a,"session-A","ACC","Monza","McLaren",folder,123456);r.add(a,"session-A","ACC","Monza","McLaren",folder,123456);assert(r.session.samples.size()==1);saved=r.session.path;r.close();auto loaded=race::loadCSV(saved);assert(loaded.samples.size()==1&&loaded.track=="Monza"&&loaded.game=="ACC"&&loaded.started==123456);}
 std::filesystem::remove(saved);std::filesystem::remove(folder);
 std::cout<<"PASS Race Studio: finish crossings, unknown gaps, lap states, CSV roundtrip, deduplication, desktop mapping\n";
}

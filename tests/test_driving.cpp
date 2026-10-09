#include "driving.hpp"
#include <cassert>
#include <iostream>
using namespace acc;
RecordedLap lap(int n,int ms,std::array<int,3> split){RecordedLap l;l.number=n;l.complete=true;l.timing.valid=true;l.timing.ms=ms;l.timing.splits=split;return l;}
int main(){
 PedalState p;p.update({1,.5f,.75f},100);assert(p.valid&&p.value.brake==.75f);p.update({1,.5f,.75f},1100);assert(!p.valid);p.waitForPacket(1);p.update({1,1,1},1200);assert(!p.valid);p.update({2,1,0},1233);assert(p.valid&&p.value.gas==1);p.update({3,NAN,0},1266);assert(!p.valid);p.update({4,0,1.2f},1299);assert(!p.valid);p.update({5,-.001f,1.001f},1333);assert(p.valid&&p.value.gas==0&&p.value.brake==1);p.reset();assert(!p.valid&&!p.have);
 Graphics g{};g.packet=1;g.status=2;g.player=7;g.valid=1;g.bestMs=100000;g.estimatedMs=99673;g.currentMs=45000;g.deltaMs=327;g.deltaPositive=0;PlayerTiming timing;timing.update(g,1000);assert(drivingDelta(timing,1001).available&&drivingDelta(timing,1001).ms==-327);
 g.packet++;g.deltaPositive=1;timing.update(g,1100);assert(drivingDelta(timing,1101).ms==327);g.packet++;g.deltaMs=-327;timing.update(g,1150);assert(drivingDelta(timing,1151).ms==327);
 g.packet++;g.deltaMs=0;timing.update(g,1200);assert(drivingDelta(timing,1201).available&&drivingDelta(timing,1201).ms==0);assert(!drivingDelta(timing,4200).available);
 for(int kind=0;kind<6;kind++){auto bad=g;bad.packet++;if(kind==0)bad.bestMs=0;if(kind==1)bad.valid=0;if(kind==2)bad.inPit=1;if(kind==3)bad.inPitLane=1;if(kind==4)bad.deltaMs=INT32_MAX;if(kind==5)bad.status=3;timing.update(bad,1400);if(kind==1)assert(drivingDelta(timing,1401).available&&drivingDelta(timing,1401).invalid);else assert(!drivingDelta(timing,1401).available);}
 g.packet++;g.estimatedMs=0;timing.update(g,1500);assert(drivingDelta(timing,1501).available);
 DeltaSnapshot snapshot;g.valid=0;snapshot.observe(g,1,{true,-400,true},true);assert(snapshot.value.available&&snapshot.value.invalid&&snapshot.value.ms==-400);
 snapshot.observe({},2,{},false);assert(snapshot.value.ms==-400);g.sessionIndex++;snapshot.observe(g,2,{},true);assert(!snapshot.value.available);
 snapshot.observe(g,2,{true,120,false},true);g.laps=5;snapshot.observe(g,2,{},true);g.laps=0;snapshot.observe(g,2,{},true);assert(!snapshot.value.available);
 Analytics a;a.active=true;a.record=0;a.recordPlayer=7;SessionRecord r;r.laps.push_back(lap(1,90000,{30000,30000,30000}));a.history.push_back(r);auto rows=recentSectors(a,7);assert(rows.size()==1&&!rows[0].hasReference);
 a.history[0].laps.push_back(lap(2,89800,{29700,30100,30000}));rows=recentSectors(a,7);assert(rows.size()==2&&rows[0].referenceLap==1&&rows[0].difference[0]==-300&&rows[0].difference[1]==100&&rows[0].difference[2]==0);assert(rows[0].comparable[0]);
 auto invalid=lap(3,85000,{28000,28000,29000});invalid.timing.invalid=true;a.history[0].laps.push_back(invalid);a.history[0].laps.push_back(lap(4,89700,{29600,30150,29950}));rows=recentSectors(a,7);assert(rows[0].referenceLap==2&&rows[0].difference[0]==-100&&!rows[1].comparable[0]);
 a.history[0].laps.back().timing.splits[0]=0;rows=recentSectors(a,7);assert(!rows[0].comparable[0]);assert(recentSectors(a,99).empty());a.endSession();assert(recentSectors(a,7).empty());
 std::cout<<"PASS: physics layout, input bounds, stale and frozen packets, delta sign / pause / pits / missing reference, prior-best sector comparison and session isolation\n";
}

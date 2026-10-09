#include "driving.hpp"
#include <cassert>
#include <iostream>
using namespace acc;
int curve(float p,int total=100000){return int(std::llround(total*(.6*p+.4*p*p)));}
struct Simulation {
 Graphics g{};PlayerTiming timing;LapDelta delta;uint64_t now=100;int packet=0;
 Simulation(){g.status=2;g.player=9;g.valid=1;g.session=2;g.sessionIndex=1;}
 void tick(int lap,float spline,int ms){g.packet=++packet;g.laps=lap;g.spline=spline;g.currentMs=ms;now+=100;timing.update(g,now);delta.observe(timing,now);}
 void finish(int lap,int total){for(int i=0;i<100;i++)tick(lap,i/100.f,curve(i/100.f,total));g.lastMs=total;tick(lap+1,0,0);}
 DeltaView view(){return delta.view(timing,now);}
};
int main(){
 Simulation s;s.finish(0,100000);assert(s.delta.best==100000&&s.delta.reference.size()>=100);assert(!s.view().available);
 // Native ACC fields can be absent: compare actual elapsed times by position.
 s.g.bestMs=s.g.estimatedMs=0;s.tick(1,.5f,curve(.5f,98000));assert(s.view().available&&std::abs(s.view().ms+800)<=1);
 s.tick(1,.51f,curve(.51f)+423);assert(s.view().available&&std::abs(s.view().ms-423)<=1);
 s.g.valid=0;s.tick(1,.52f,curve(.52f));assert(!s.view().available);s.g.valid=1;s.g.lastMs=85000;s.tick(2,0,0);assert(s.delta.best==100000);
 s.finish(2,98000);assert(s.delta.best==98000);s.tick(3,.5f,curve(.5f));assert(std::abs(s.view().ms-800)<=1);
 s.g.status=3;s.tick(3,.5f,curve(.5f));assert(!s.view().available&&s.delta.best==98000);s.g.status=2;s.tick(3,.51f,curve(.51f));assert(s.view().available);
 s.g.inPitLane=1;s.tick(3,.52f,curve(.52f));assert(!s.view().available);s.g.inPitLane=0;
 s.g.sessionIndex=2;s.tick(0,.5f,45000);assert(s.delta.best==0&&!s.view().available);s.g.lastMs=95000;s.tick(1,0,0);assert(s.delta.best==0); // joined midlap is never a reference
 s.finish(1,96000);assert(s.delta.best==96000);s.tick(2,.5f,curve(.5f,96000));assert(!s.delta.view(s.timing,s.now+3001).available);
 s.delta.interrupt();assert(s.delta.best==96000);s.g.player=10;s.tick(2,.51f,50000);assert(s.delta.best==0);
 // A premature lap-counter update just before the spline wraps can still arm.
 Simulation seam;seam.tick(0,.99f,curve(.99f));seam.g.lastMs=100000;seam.tick(1,.999f,0);seam.tick(1,.001f,60);for(int i=1;i<100;i++)seam.tick(1,i/100.f,curve(i/100.f));seam.g.lastMs=100000;seam.tick(2,0,0);assert(seam.delta.best==100000);
 // Pit, invalid, reverse and large telemetry gaps cannot replace a clean lap.
 for(int kind=0;kind<4;kind++){Simulation q;q.finish(0,100000);for(int i=1;i<100;i++){if(kind==0&&i==20)q.g.inPit=1;if(kind==1&&i==20)q.g.valid=0;if(kind==2&&i==20){q.tick(1,.1f,curve(.2f,90000));continue;}if(kind==3&&i>=20&&i<25)continue;q.tick(1,i/100.f,curve(i/100.f,90000));}q.g.inPit=0;q.g.valid=1;q.g.lastMs=90000;q.tick(2,0,0);assert(q.delta.best==100000);}
 std::cout<<"PASS: continuous nonlinear reference, signed gains/losses, missing native fields, fastest clean reference, pause, pits, invalid/partial laps, seam, dropout, reversal, player and session resets\n";
}

#pragma once
#include "runtime.hpp"
#include "analytics.hpp"

namespace acc {
// Only the documented prefix is mapped; later physics fields are not required.
#pragma pack(push,4)
struct PhysicsInput {int32_t packet=0;float gas=0,brake=0;};
#pragma pack(pop)
static_assert(sizeof(PhysicsInput)==12 && offsetof(PhysicsInput,brake)==8,"ACC physics input layout");
struct PedalState {
 PhysicsInput value{};bool have=false,valid=false,blocked=false;int blockedPacket=0;uint64_t changedAt=0;
 void reset(){*this=PedalState{};}
 void waitForPacket(int packet){reset();blocked=true;blockedPacket=packet;}
 bool expired(uint64_t now)const{return have&&!fresh(changedAt,now,1000);}
 void update(PhysicsInput next,uint64_t now){
  if(blocked&&next.packet==blockedPacket){valid=false;return;}blocked=false;
  if(!std::isfinite(next.gas)||!std::isfinite(next.brake)||next.gas<-.01f||next.gas>1.01f||next.brake<-.01f||next.brake>1.01f){valid=false;return;}
  if(!have||value.packet!=next.packet)changedAt=now;
  next.gas=std::clamp(next.gas,0.f,1.f);next.brake=std::clamp(next.brake,0.f,1.f);value=next;have=true;valid=!expired(now);
 }
};
struct DeltaView {bool available=false;int ms=0;};
inline DeltaView drivingDelta(const PlayerTiming& timing,uint64_t now){
 const auto& g=timing.g;DeltaView v;
 if(!timing.valid||timing.expired(now)||g.status!=2||g.inPit||g.inPitLane||!g.valid||!timeValue(g.bestMs)||g.currentMs<=0||g.deltaMs==INT32_MAX||g.deltaMs==INT32_MIN||g.deltaPositive<0||g.deltaPositive>1)return v;
 // ACC supplies a magnitude and a separate sign. Keep the millisecond precision.
 int64_t magnitude=std::abs(int64_t(g.deltaMs));if(magnitude>3600000)return v;
 v.available=true;v.ms=int(magnitude)*(g.deltaPositive?1:-1);return v;
}
// Keep a time/distance trace of the fastest complete clean lap observed locally.
// Compare elapsed time at the same spline location, never a linear lap estimate.
struct LapDelta {
 struct Point {float spline;int ms;};
 std::vector<Point> current,reference;Graphics last{};
 bool have=false,recording=false,awaitStart=false;int best=0;
 void reset(){*this=LapDelta{};}
 void interrupt(){recording=false;awaitStart=false;current.clear();}
 void start(const Graphics& g){
  current.clear();awaitStart=true;recording=g.valid&&!g.inPit&&!g.inPitLane&&g.spline<=.015f&&g.currentMs>=0&&g.currentMs<2000;
  if(recording){awaitStart=false;current.push_back({0,0});if(g.spline>0)current.push_back({g.spline,g.currentMs});}
 }
 void observe(const PlayerTiming& timing,uint64_t now){
  if(!timing.valid||timing.expired(now)){interrupt();return;}
  const auto& g=timing.g;
  if(g.status==3)return; // Pause retains the reference and the unfinished lap.
  if(g.status!=2){reset();return;}
  bool session=have&&(g.session!=last.session||g.sessionIndex!=last.sessionIndex||g.player!=last.player||g.laps<last.laps);
  bool restart=have&&g.laps==last.laps&&double(g.currentMs)+1500<last.currentMs;
  if(session||restart)reset();
  if(!have){start(g);last=g;have=true;return;}
  if(g.packet==last.packet)return;
  if(g.laps!=last.laps){
   if(g.laps==last.laps+1&&recording&&current.size()>=20&&current.back().spline>=.97f&&timeValue(g.lastMs)&&g.lastMs>=current.back().ms&&(!best||g.lastMs<best)){
    current.push_back({1,g.lastMs});reference=current;best=g.lastMs;
   }
   start(g);
  }else if(!recording&&awaitStart){
   if(g.currentMs>=2000)awaitStart=false;else start(g);
  }else if(recording){
   if(!g.valid||g.inPit||g.inPitLane||g.spline+.0001f<last.spline||g.spline-last.spline>.03f||g.currentMs<last.currentMs||current.size()>=12000)interrupt();
   else if(g.spline>current.back().spline+.00001f&&g.currentMs>=current.back().ms)current.push_back({g.spline,g.currentMs});
  }
  last=g;
 }
 DeltaView view(const PlayerTiming& timing,uint64_t now)const{
  DeltaView v;const auto& g=timing.g;
  if(reference.size()<2||!have||!timing.valid||timing.expired(now)||g.status!=2||!g.valid||g.inPit||g.inPitLane||g.currentMs<=0||g.player!=last.player||g.session!=last.session||g.sessionIndex!=last.sessionIndex)return v;
  auto hi=std::lower_bound(reference.begin(),reference.end(),g.spline,[](const Point& p,float x){return p.spline<x;});
  if(hi==reference.end())return v;
  double expected=hi->ms;
  if(hi!=reference.begin()){auto lo=hi-1;double width=hi->spline-lo->spline;if(width<=0||width>.031)return v;expected=lo->ms+(hi->ms-lo->ms)*(g.spline-lo->spline)/width;}
  double diff=g.currentMs-expected;if(std::abs(diff)>3600000)return v;v.available=true;v.ms=int(std::llround(diff));return v;
 }
};
struct SectorReview {RecordedLap lap;bool hasReference=false;int referenceLap=0;std::array<int,3> difference{};std::array<bool,3> comparable{};};
inline std::vector<SectorReview> recentSectors(const Analytics& a,int player,int count=2){
 std::vector<SectorReview> rows;
 if(!a.active||a.record==SIZE_MAX||a.record>=a.history.size()||a.recordPlayer!=player)return rows;
 const auto& laps=a.history[a.record].laps;
 for(int i=int(laps.size())-1;i>=0&&int(rows.size())<count;--i){
  SectorReview row;row.lap=laps[i];const RecordedLap* reference=nullptr;
  for(int k=0;k<i;k++)if(laps[k].clean()&&(!reference||laps[k].timing.ms<reference->timing.ms))reference=&laps[k];
  if(reference){row.hasReference=true;row.referenceLap=reference->number;for(int s=0;s<3;s++){
   row.comparable[s]=row.lap.clean()&&row.lap.timing.splits[s]>0&&reference->timing.splits[s]>0;
   if(row.comparable[s])row.difference[s]=row.lap.timing.splits[s]-reference->timing.splits[s];
  }}
  rows.push_back(row);
 }
 return rows;
}
}

#pragma once
#include "telemetry.hpp"

namespace acc {
struct ReconnectPolicy {
 uint64_t next=0,started=0;unsigned failures=0,attempts=0;bool awaiting=false;
 bool due(uint64_t now)const{return !awaiting&&now>=next;}
 void begin(uint64_t now){started=now;awaiting=true;++attempts;}
 bool timedOut(uint64_t now)const{return awaiting&&now>=started&&now-started>=3000;}
 void failed(uint64_t now){awaiting=false;failures=std::min(5u,failures+1);next=now+std::min(10000u,1000u<<(failures-1));}
 void connected(){awaiting=false;failures=0;next=0;}
};

// Session heartbeats alone do not prove that the grid subscription is alive.
inline bool gridTimedOut(uint64_t now,uint64_t started,uint64_t lastCar,bool registered,bool session,bool paused){
 if(!registered||!session||paused)return false;uint64_t reference=std::max(started,lastCar);return now>=reference&&now-reference>=8000;
}

inline int broadcastSession(int sharedType){switch(sharedType){case 0:return 0;case 1:return 4;case 2:return 10;case 3:return 11;case 7:return 12;case 8:return 13;default:return -1;}}
enum class SharedChange {None,Session,LapReset,Offline};
struct PlayerTiming {
 Graphics g{};std::array<int,3> current{},previous{};
 bool valid=false,have=false,blocked=false;int blockedPacket=0;
 uint64_t changedAt=0,lapAt=0;
 void reset(){*this=PlayerTiming{};}
 void waitForPacket(int packet){reset();blocked=true;blockedPacket=packet;}
 bool expired(uint64_t now)const{return have&&g.status!=3&&!fresh(changedAt,now,3000);}
 bool showPrevious(uint64_t now)const{return fresh(lapAt,now,5000)&&g.currentMs<5000;}
 SharedChange update(const Graphics& next,uint64_t now){
  if(blocked&&next.packet==blockedPacket){valid=false;return SharedChange::None;}blocked=false;
  if(next.status<0||next.status>3){valid=false;return SharedChange::None;}
  if(next.status==0){bool had=have;reset();g=next;return had?SharedChange::Offline:SharedChange::None;}
  // Replay timing comes from the focused broadcast car, never the player's old lap.
  if(next.status==1){bool had=have;reset();g=next;return had?SharedChange::Session:SharedChange::None;}
  if(next.laps<0||next.laps>100000||next.sector<0||next.sector>2||next.player<0||!std::isfinite(next.spline)||next.spline<0||next.spline>1){valid=false;return SharedChange::None;}
  bool session=have&&(g.sessionIndex!=next.sessionIndex||g.session!=next.session||next.laps<g.laps);
  bool identity=have&&next.player!=g.player;
  bool resetLap=have&&next.laps==g.laps&&double(next.currentMs)+1500<g.currentMs;
  bool first=!have||session||identity||resetLap;
  if(first){current={};previous={};lapAt=0;}
  if(!first&&next.laps>g.laps){
   previous={};if(next.laps==g.laps+1&&timeValue(next.lastMs)){previous=current;if(previous[0]&&previous[1]){int64_t s3=int64_t(next.lastMs)-previous[0]-previous[1];previous[2]=s3>0&&s3<INT32_MAX?int(s3):0;}}
   current={};lapAt=now;
  }
  if(next.sector>0&&(first||next.laps!=g.laps||next.sector!=g.sector||next.lastSector!=g.lastSector))current[next.sector-1]=timeValue(next.lastSector);
  if(first||next.packet!=g.packet||next.status!=g.status)changedAt=now;
  g=next;have=true;valid=!expired(now);
  return session?SharedChange::Session:(identity||resetLap?SharedChange::LapReset:SharedChange::None);
 }
};

struct TimingView {bool available=false,invalid=false,previous=false;int lap=0,ms=0,active=0;std::array<int,3> splits{};};
inline TimingView timingView(const Car* car,const PlayerTiming* player,uint64_t now){
 TimingView v;
 if(player&&player->valid){
  const auto& g=player->g;v.available=true;v.lap=g.laps+1;v.ms=timeValue(g.currentMs);v.invalid=!g.valid;v.active=g.sector;v.previous=player->showPrevious(now);v.splits=v.previous?player->previous:player->current;
  if(car&&car->id==g.player&&car->laps==g.laps&&fresh(car->seen,now,1500)){
   if(v.previous&&car->last.ms==g.lastMs){for(int i=0;i<3;i++)if(car->last.splits[i]>0)v.splits[i]=car->last.splits[i];}
   else if(!v.previous&&std::abs(double(car->current.ms)-g.currentMs)<1500){for(int i=0;i<v.active;i++)if(car->current.splits[i]>0)v.splits[i]=car->current.splits[i];}
  }
 }else if(car){
  v.available=true;v.lap=car->laps+1;v.ms=car->current.ms;v.invalid=car->current.invalid;v.splits=car->current.splits;
  int64_t sum=0;for(int i=0;i<3;i++){sum+=v.splits[i];if(!v.splits[i]||sum>int64_t(v.ms)+250){for(int j=i;j<3;j++)v.splits[j]=0;break;}v.active=std::min(2,i+1);}
  if(v.ms<5000&&car->laps>0&&car->last.ms){v.previous=true;v.splits=car->last.splits;v.active=0;}
 }
 return v;
}
}

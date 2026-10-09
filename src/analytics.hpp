#pragma once
#include "telemetry.hpp"
#include <set>

namespace acc {
inline uint64_t historySequence=0;
struct RecordedLap {
 int number=0; Lap timing; std::string driver; bool complete=false,pit=false;
 bool clean()const{return complete&&!pit&&timing.valid&&!timing.invalid&&!timing.in&&!timing.out&&timing.ms>0;}
};
struct SessionRecord {
 std::string id,track,driver,team; int trackId=-1,type=0,model=0,number=0; int64_t started=0;
 std::vector<RecordedLap> laps;
 int best()const{int ms=0;for(const auto& l:laps)if(l.timing.valid&&!l.timing.invalid&&!l.timing.in&&!l.timing.out&&l.timing.ms>0)ms=ms?std::min(ms,l.timing.ms):l.timing.ms;return ms;}
};
struct PaceTracker {
 int lap=-1,driver=-1; uint64_t seen=0; int currentMs=0; bool complete=false,pit=false;
 std::deque<RecordedLap> recent;
 double pace()const{std::vector<int> times;for(const auto& l:recent)if(l.clean()&&lap-l.number<5)times.push_back(l.timing.ms);if(times.size()<3)return NAN;std::sort(times.begin(),times.end());size_t n=times.size();return (n%2?times[n/2]:(times[n/2-1]+double(times[n/2]))/2)/1000.;}
};
struct Analytics {
 std::map<int,PaceTracker> trackers; std::vector<SessionRecord> history; std::set<size_t> dirty;
 bool active=false; int event=-1,session=-1,type=-1,trackId=-1,phase=0; std::string track;
 float elapsed=0; uint64_t epoch=UINT64_MAX; int64_t started=0; size_t record=SIZE_MAX; int recordPlayer=-1;
 void endSession(){active=false;trackers.clear();record=SIZE_MAX;recordPlayer=-1;epoch=UINT64_MAX;}
 // A transport interruption preserves the session file, but not unobserved pace.
 void disconnect(){trackers.clear();epoch=UINT64_MAX;}
 void pausedAt(uint64_t now){for(auto& kv:trackers)kv.second.seen=now;}
 bool sync(const State& s,int64_t wallMs){
  if(s.replay){endSession();return false;}
  if(!s.hasSession||s.track.empty()||s.trackId<0)return false;
  bool boundary=active&&(event!=s.event||session!=s.session||type!=s.type||trackId!=s.trackId||track!=s.track||s.elapsed<elapsed-2000||(phase>=5&&s.phase>=1&&s.phase<5)||(phase>=6&&s.phase==5)||(epoch!=UINT64_MAX&&s.epoch!=epoch));
  if(boundary)endSession();
  if(!active){active=true;started=wallMs;event=s.event;session=s.session;type=s.type;trackId=s.trackId;track=s.track;}
  elapsed=s.elapsed;phase=s.phase;epoch=s.epoch;return true;
 }
 void observe(const State& s,int carId,int playerId,uint64_t now,int64_t wallMs){
  if(!sync(s,wallMs))return;auto it=s.cars.find(carId);if(it==s.cars.end())return;const Car& c=it->second;
  auto& t=trackers[c.id];
  if(t.lap<0){t.lap=c.laps;t.driver=c.driver;t.seen=now;t.currentMs=c.current.ms;t.pit=c.location!=1;return;}
  // Ignore delayed previous-lap datagrams. Actual restarts are handled by sync.
  if(c.laps<t.lap)return;
  bool discontinuity=c.driver!=t.driver||now<t.seen||now-t.seen>4000;
  if(discontinuity){t.recent.clear();t.complete=false;}
  if(c.laps==t.lap&&c.current.ms+2000<t.currentMs){t.pit=true;t.recent.clear();}
  if(c.laps>t.lap){
   RecordedLap l;l.number=c.laps;l.timing=c.last;l.driver=c.name();l.pit=t.pit||c.location!=1;l.complete=t.complete&&!discontinuity&&c.laps==t.lap+1;
   if(c.last.ms>0){
    t.recent.push_back(l);while(t.recent.size()>5)t.recent.pop_front();
    if(c.id==playerId){
     if(recordPlayer!=c.id)record=SIZE_MAX;
     if(record==SIZE_MAX){SessionRecord r;r.id=std::to_string(wallMs)+"-"+std::to_string(now)+"-"+std::to_string(historySequence++);r.started=started;r.track=track;r.trackId=trackId;r.type=type;r.driver=c.name();r.team=c.team;r.number=c.number;r.model=c.model;history.push_back(r);record=history.size()-1;recordPlayer=c.id;}
     auto& r=history[record];if(r.laps.empty()||r.laps.back().number<l.number){r.laps.push_back(l);dirty.insert(record);}
    }
   }
   t.lap=c.laps;t.complete=!discontinuity;t.pit=c.location!=1;
  }
  if(c.location!=1){t.pit=true;t.recent.clear();}
  t.driver=c.driver;t.seen=now;t.currentMs=c.current.ms;
 }
 double pace(int id)const{auto it=trackers.find(id);return it==trackers.end()?NAN:it->second.pace();}
};
enum class ForecastState {Waiting,Stable,Opening,Catching,AfterFinish};
struct Forecast {ForecastState state=ForecastState::Waiting;double closing=NAN,laps=NAN;};
inline Forecast forecast(double gap,double pursuerPace,double targetPace,double remainingSeconds=NAN){
 Forecast f;if(!std::isfinite(gap)||gap<0||!std::isfinite(pursuerPace)||!std::isfinite(targetPace)||pursuerPace<=0||targetPace<=0)return f;
 f.closing=targetPace-pursuerPace;if(std::abs(f.closing)<.1){f.state=ForecastState::Stable;return f;}
 if(f.closing<0){f.state=ForecastState::Opening;return f;}
 f.laps=gap/f.closing;f.state=std::isfinite(remainingSeconds)&&remainingSeconds>0&&f.laps*pursuerPace>remainingSeconds?ForecastState::AfterFinish:ForecastState::Catching;return f;
}
// Nearest same-lap opponent in each direction. Lapped traffic remains in relatives,
// but is deliberately not used as a race-position target for these predictions.
inline const Car* paceRival(const State& s,const Car& me,bool ahead,uint64_t now){
 const Car* best=nullptr;double distance=.5;
 for(const auto& kv:s.cars){const Car& c=kv.second;if(c.id==me.id||c.location!=1||!fresh(c.seen,now)||!c.position||!me.position||lapRelation(c,me,true)!=0)continue;
  double d=c.laps+double(c.spline)-me.laps-me.spline;
  if((ahead&&c.position>=me.position)||(!ahead&&c.position<=me.position))continue;
  d=ahead?d:-d;if(d>0&&d<distance){distance=d;best=&c;}
 }return best;
}
}

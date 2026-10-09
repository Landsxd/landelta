#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstddef>
#include <cstring>
#include <deque>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace acc {
struct Reader {
 const uint8_t* p; size_t n, at=0;
 template<class T> T get() { if(at+sizeof(T)>n) throw std::runtime_error("truncated packet"); T v; std::memcpy(&v,p+at,sizeof v); at+=sizeof v; return v; }
 uint8_t u8(){return get<uint8_t>();} uint16_t u16(){return get<uint16_t>();} int32_t i32(){return get<int32_t>();} float f32(){return get<float>();}
 std::string str(){auto k=u16(); if(at+k>n) throw std::runtime_error("truncated string"); std::string s((const char*)p+at,k); at+=k; return s;}
};
struct Writer {
 std::vector<uint8_t> b;
 template<class T> void put(T v){auto p=(uint8_t*)&v; b.insert(b.end(),p,p+sizeof v);}
 void str(const std::string& s){if(s.size()>65535)throw std::runtime_error("string too long");put<uint16_t>((uint16_t)s.size());b.insert(b.end(),s.begin(),s.end());}
};
inline Writer registration(const std::string& password){Writer w;w.put<uint8_t>(1);w.put<uint8_t>(4);w.str("LANDELTA ACC Overlay");w.str(password);w.put<int32_t>(100);w.str("");return w;}
inline Writer command(uint8_t type,int id){Writer w;w.put(type);w.put<int32_t>(id);return w;}
inline int timeValue(int n){return n>0 && n<INT32_MAX?n:0;}
inline std::string initial(const std::string& name){if(name.empty())return {};unsigned char c=name[0];size_t n=c<0x80?1:((c&0xe0)==0xc0?2:((c&0xf0)==0xe0?3:((c&0xf8)==0xf0?4:1)));return name.substr(0,std::min(n,name.size()));}
inline std::string formatTimeMs(int ms){if(!timeValue(ms))return "--:--.---";char b[40];snprintf(b,sizeof b,"%d:%02d.%03d",ms/60000,(ms/1000)%60,ms%1000);return b;}
struct Lap {int ms=0;std::array<int,3> splits{};bool invalid=false, valid=false, out=false,in=false;};
inline Lap readLap(Reader& r){Lap l;l.ms=timeValue(r.i32());r.u16();r.u16();int count=r.u8();if(count>32)throw std::runtime_error("split count");for(int i=0;i<count;i++){int v=timeValue(r.i32());if(i<3)l.splits[i]=v;}l.invalid=r.u8()!=0;l.valid=r.u8()!=0;l.out=r.u8()!=0;l.in=r.u8()!=0;return l;}
struct Car {int id=0,model=0,number=0,cup=0,driver=0,position=0,laps=0,location=0,speed=0,delta=0;float spline=0;std::string team,penaltyMessage;std::vector<std::string> drivers;Lap best,last,current;uint64_t seen=0,penaltyNoticeAt=0;
 std::string name()const{if(driver>=0 && driver<(int)drivers.size())return drivers[driver];if(!team.empty())return team;return "Auto #"+std::to_string(number?number:id);}
};
inline bool fresh(uint64_t at,uint64_t now,uint64_t limit=4000){return at&&now>=at&&now-at<limit;}
// Actual passage timestamps, kept separately so drawing a grid never copies histories.
struct Passages {
 struct Sample {double progress;uint64_t at;};std::deque<Sample> samples;
 void add(const Car& c,uint64_t now){
  if(c.location!=1){samples.clear();return;}double p=c.laps+double(c.spline);
  if(!samples.empty()){const auto& last=samples.back();if(now<=last.at)return;
   if(now-last.at>2000||p<last.progress-.0001||p-last.progress>.10)samples.clear();
   else if(p<=last.progress+.000001)return;
  }
  samples.push_back({p,now});while(samples.size()>6000||(!samples.empty()&&now-samples.front().at>900000))samples.pop_front();
 }
 double at(double p)const{
  if(samples.size()<2||p<samples.front().progress||p>samples.back().progress)return NAN;
  auto hi=std::lower_bound(samples.begin(),samples.end(),p,[](const Sample& s,double q){return s.progress<q;});
  if(hi==samples.end())return NAN;if(hi->progress==p)return double(hi->at);if(hi==samples.begin())return NAN;
  auto lo=std::prev(hi);double d=hi->progress-lo->progress;if(d<=0||hi->at-lo->at>2000)return NAN;
  return lo->at+(hi->at-lo->at)*(p-lo->progress)/d;
 }
};
struct State {
 std::map<int,Car> cars;std::map<int,Passages> passages;int connection=-1,focus=-1,event=-1,session=-1,type=0,phase=0,meters=0,trackId=-1;float elapsed=0,end=0,replayClock=0,pendingClock=0;bool registered=false,replay=false,hasSession=false,rollbackPending=false;std::string error,track,resetReason="inicio";Lap sessionBest;uint64_t received=0,sessionReceived=0,epoch=0;int badPackets=0;std::string parseError;
 void clearSession(const std::string& reason="cambio de sesión",bool dropTrack=false){cars.clear();passages.clear();sessionBest={};focus=-1;hasSession=false;sessionReceived=0;rollbackPending=false;resetReason=reason;++epoch;if(dropTrack){track.clear();meters=0;trackId=-1;}}
 // Parse into temporaries: malformed datagrams never partially mutate telemetry.
 int parse(const uint8_t* p,size_t n,uint64_t now){try{Reader r{p,n};int msg=r.u8();switch(msg){
 case 1:{int id=r.i32();bool ok=r.u8()>0;r.u8();auto err=r.str();connection=id;registered=ok;error=err;break;}
 case 2:{int e=r.u16(),s=r.u16(),t=r.u8(),ph=r.u8();float tm=r.f32(),en=r.f32();int f=r.i32();r.str();r.str();r.str();bool rep=r.u8()>0;float rc=tm;if(rep){rc=r.f32();r.f32();}r.f32();for(int i=0;i<5;i++)r.u8();Lap best=readLap(r);if(!std::isfinite(tm)||!std::isfinite(en)||!std::isfinite(rc)||ph>8)throw std::runtime_error("session float/phase");
  bool reset=!hasSession||event!=e||session!=s||type!=t||replay!=rep||(phase>=5&&ph>=1&&ph<5)||(phase>=6&&ph==5)||(rep&&rc<replayClock-250);
  if(!reset&&!rep&&tm<elapsed){
   if(tm<elapsed-2000){if(rollbackPending&&tm>=pendingClock&&tm-pendingClock<2000)reset=true;else{pendingClock=tm;rollbackPending=true;return 0;}}
   else return 0; // A late UDP session update must not rewind the displayed clock.
  }
  if(reset)clearSession(rep?"replay / salto de tiempo":"sesión / fase / reinicio");rollbackPending=false;
  event=e;session=s;type=t;phase=ph;elapsed=tm;end=en;focus=f;replay=rep;replayClock=rc;sessionBest=best;received=sessionReceived=now;hasSession=true;return reset?12:msg;}
 case 3:{int id=r.u16();Car c;auto it=cars.find(id);if(it!=cars.end())c=it->second;c.id=id;c.driver=r.u16();r.u8();r.u8();r.f32();r.f32();r.f32();c.location=r.u8();c.speed=r.u16();c.position=r.u16();r.u16();r.u16();c.spline=r.f32();c.laps=r.u16();c.delta=r.i32();c.best=readLap(r);c.last=readLap(r);c.current=readLap(r);if(!std::isfinite(c.spline)||c.spline<0||c.spline>1||c.position>1000||c.location>4)throw std::runtime_error("car float/location");c.seen=now;passages[id].add(c,now);cars[id]=c;received=now;break;}
 case 4:{int conn=r.i32();int k=r.u16();if(k>1000)throw std::runtime_error("entry count");std::vector<int> ids;for(int i=0;i<k;i++)ids.push_back(r.u16());if(registered&&conn!=connection)return 0;for(auto it=cars.begin();it!=cars.end();)if(std::find(ids.begin(),ids.end(),it->first)==ids.end()){passages.erase(it->first);it=cars.erase(it);}else ++it;for(int id:ids){cars[id].id=id;}break;}
 case 5:{int conn=r.i32();auto tr=r.str();int tid=r.i32(),m=r.i32();int sets=r.u8();for(int i=0;i<sets;i++){r.str();int count=r.u8();for(int j=0;j<count;j++)r.str();}int hud=r.u8();for(int i=0;i<hud;i++)r.str();if(m<0||m>1000000)throw std::runtime_error("track length");if(registered&&conn!=connection)return 0;bool changed=trackId>=0&&(tid!=trackId||tr!=track);if(changed)clearSession("cambio de circuito");track=tr;trackId=tid;meters=m;if(changed)return 12;break;}
 case 6:{int id=r.u16();Car c;auto it=cars.find(id);if(it!=cars.end())c=it->second;c.id=id;c.model=r.u8();c.team=r.str();c.number=r.i32();c.cup=r.u8();c.driver=r.u8();r.u16();int count=r.u8();c.drivers.clear();for(int i=0;i<count;i++){auto first=r.str(),last=r.str(),shortname=r.str();r.u8();r.u16();auto name=first.empty()?last:(initial(first)+". "+last);c.drivers.push_back(name.empty()?shortname:name);}cars[id]=c;break;}
 // PenaltyCommMsg is a notification, not an authoritative pending-penalty flag.
 // Preserve the message for diagnostics; do not infer issued/served from its text.
 case 7:{int type=r.u8();auto message=r.str();r.i32();int id=r.i32();if(type==3&&id>=0&&id<=65535){auto& c=cars[id];c.id=id;c.penaltyMessage=message;c.penaltyNoticeAt=now;}break;}
 default:return 0;}
 return msg;
 }catch(const std::exception& e){badPackets++;parseError=e.what();return -1;}}
 std::vector<Car> active(uint64_t now,bool paused=false)const {std::vector<Car> v;for(auto& kv:cars)if(fresh(kv.second.seen,now)||(paused&&kv.second.seen))v.push_back(kv.second);std::sort(v.begin(),v.end(),[](const Car&a,const Car&b){int pa=a.position?a.position:INT32_MAX,pb=b.position?b.position:INT32_MAX;return pa==pb?a.id<b.id:pa<pb;});return v;}
 double passage(int id,double progress)const{auto it=passages.find(id);return it==passages.end()?NAN:it->second.at(progress);}
 double relativeEstimate(const Car& other,const Car& me)const;
 std::string gapToLeader(const Car& c,const Car& leader)const;
};
inline double lapSeconds(const Car& c){int m=c.last.ms&&c.last.valid&&!c.last.invalid&&!c.last.in&&!c.last.out?c.last.ms:c.best.ms;return m?m/1000.0:0;}
// Physical neighbors on circuit, not race position. Negative = ahead, positive = behind.
inline double relativeGap(const Car& other,const Car& me){double d=double(other.spline)-me.spline;while(d>.5)d-=1;while(d<-.5)d+=1;double sec=lapSeconds(me);if(!sec)sec=lapSeconds(other);return sec?-d*sec:NAN;}
inline double State::relativeEstimate(const Car& other,const Car& me)const{
 double d=double(other.spline)-me.spline;while(d>.5)d-=1;while(d<-.5)d+=1;
 if(!replay&&other.location==1&&me.location==1){double at=d>=0?passage(other.id,other.laps+double(other.spline)-d):passage(me.id,me.laps+double(me.spline)+d);double gap=d>=0?-(double(me.seen)-at)/1000.:(double(other.seen)-at)/1000.;if(std::isfinite(gap)&&((d>=0&&gap<=0)||(d<0&&gap>=0)))return gap;}
 return relativeGap(other,me);
}
// Compare total progress, so two same-lap cars straddling the finish stay neutral.
// -1 = lapped by the player (blue), +1 = lapping the player (amber).
inline int lapRelation(const Car& other,const Car& me,bool race){if(!race||other.id==me.id)return 0;double d=double(other.laps)+other.spline-me.laps-me.spline;return d<-.5?-1:(d>.5?1:0);}
enum class PenaltyBadge {None,Confirmed,Notice};
inline PenaltyBadge penaltyBadge(const Car& c,uint64_t now,bool playerData=false,int penalty=0){if(playerData&&penalty>=0)return penalty>0?PenaltyBadge::Confirmed:PenaltyBadge::None;return c.penaltyNoticeAt&&now>=c.penaltyNoticeAt&&now-c.penaltyNoticeAt<60000?PenaltyBadge::Notice:PenaltyBadge::None;}
inline std::string raceGap(const Car& c,const Car& leader,bool race){char b[64];if(c.id==leader.id)return "LIDER";if(!race){if(!c.best.ms||!leader.best.ms)return "--";snprintf(b,sizeof b,"%+.3f",(c.best.ms-leader.best.ms)/1000.);return b;}double d=double(leader.laps)+leader.spline-c.laps-c.spline;if(d>=1.0){return "+"+std::to_string((int)std::floor(d))+" V";}double sec=lapSeconds(leader);if(!sec)return "--";snprintf(b,sizeof b,"~+%.1f",std::max(0.,d*sec));return b;}
inline std::string State::gapToLeader(const Car& c,const Car& leader)const{
 if(!c.position||!leader.position)return "--";
 if(type!=10||c.id==leader.id)return raceGap(c,leader,type==10);if(phase<5)return "--";
 double d=leader.laps+double(leader.spline)-c.laps-c.spline;if(d<-.01)return "--";if(d>=1)return "+"+std::to_string(int(std::floor(d)))+" V";
 if(!replay){double at=passage(leader.id,c.laps+double(c.spline));double sec=(double(c.seen)-at)/1000.;if(std::isfinite(sec)&&sec>=0){char b[48];snprintf(b,sizeof b,"~+%.1f",sec);return b;}}
 return raceGap(c,leader,true);
}
#pragma pack(push,4)
struct Graphics {
 int32_t packet,status,session;uint16_t currentTime[15],lastTime[15],bestTime[15],split[15];
 int32_t laps,position,currentMs,lastMs,bestMs;float remaining,distance;int32_t inPit,sector,lastSector,numberLaps;uint16_t compound[33];float replayMultiplier,spline;int32_t activeCars;float coordinates[60][3];int32_t carIds[60],player;
 float penaltyTime;int32_t flag,penalty,idealLine,inPitLane;float grip;int32_t mandatory;float wind,direction;int32_t setup,mainDisplay,secondaryDisplay,tc,tcCut,map,abs;float fuelPerLap;int32_t rainLights,flashing,lights;float exhaust;int32_t wiper,stintTotal,stint,rainTyres,sessionIndex;float usedFuel;uint16_t deltaString[15];int32_t deltaMs;uint16_t estimatedString[15];int32_t estimatedMs,deltaPositive,splitMs,valid;float fuelLaps;
};
#pragma pack(pop)
static_assert(offsetof(Graphics,player)==1216,"ACC layout: player ID");
static_assert(offsetof(Graphics,penalty)==1228,"ACC layout: player penalty");
static_assert(offsetof(Graphics,deltaMs)==1360,"ACC layout: delta");
}

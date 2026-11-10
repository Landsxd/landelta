#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// Shared presentation model. Missing telemetry stays missing; never synthesize laps.
namespace race {
struct Sample {
 uint64_t at=0; int packet=-1,lap=0,ms=0,last=0,best=0,sector=-1,lastSector=0,valid=-1,stint=0;
 double progress=NAN,speed=NAN,gas=NAN,brake=NAN,steer=NAN,x=NAN,z=NAN,delta=NAN;
 bool pit=false; int gear=0,rpm=0,sourceHz=0;uint64_t missedTicks=0; std::array<int,3> splits{};
};
struct Lap {
 int number=0,ms=0,valid=-1; bool complete=false,pit=false,partial=true;
 std::array<int,3> sectors{}; std::array<double,10> micro;
 Lap(){micro.fill(NAN);}
};
struct Session {
 std::string id,game,track,car,variant,steerUnit="native"; int64_t started=0;
 std::vector<Sample> samples; std::vector<Lap> laps; uint64_t gaps=0; double length=0;
 std::filesystem::path path;
 int best()const{int b=0;for(auto& l:laps)if(l.complete&&!l.partial&&l.valid!=0&&!l.pit&&l.ms>0&&(!b||l.ms<b))b=l.ms;return b;}
};
inline bool timeOK(int ms){return ms>0&&ms<INT32_MAX;}
inline std::array<int,3> observedSplits(int remoteLap,int localLap,int remoteMs,int localMs,int sector,const std::array<int,3>& splits){std::array<int,3> out{};if(remoteLap==localLap&&std::abs(int64_t(remoteMs)-localMs)<500)for(int i=0;i<std::clamp(sector,0,3);++i)out[i]=timeOK(splits[i])?splits[i]:0;return out;}
inline bool restarted(const Sample& before,const Sample& after){return after.lap<before.lap||(after.lap==before.lap&&int64_t(after.ms)+1500<before.ms&&!(before.progress>.9&&after.progress<.1));}
inline std::string safe(std::string s){for(auto& c:s)if(!std::isalnum((unsigned char)c)&&c!='-'&&c!='_')c='_';return s.substr(0,80);}
inline std::string encode(const std::string& s){std::string o;const char* h="0123456789ABCDEF";for(unsigned char c:s){if(std::isalnum(c)||c=='_'||c=='-')o+=c;else{o+='%';o+=h[c>>4];o+=h[c&15];}}return o;}
inline std::string decode(const std::string& s){std::string o;auto hex=[](char c)->int{if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;if(c>='a'&&c<='f')return c-'a'+10;return -1;};for(size_t i=0;i<s.size();++i){if(s[i]=='%'&&i+2<s.size()&&hex(s[i+1])>=0&&hex(s[i+2])>=0){o+=char(hex(s[i+1])*16+hex(s[i+2]));i+=2;}else o+=s[i];}return o;}
inline std::vector<std::string> cells(const std::string& s){std::vector<std::string> v;std::istringstream f(s);std::string c;while(std::getline(f,c,',')){if(!c.empty()&&c.back()=='\r')c.pop_back();v.push_back(c);}return v;}
inline void analyze(Session& s){
 s.laps.clear();s.gaps=0;if(s.samples.empty())return;
 for(size_t i=1;i<s.samples.size();++i)if(s.samples[i].at<s.samples[i-1].at||s.samples[i].at-s.samples[i-1].at>500)++s.gaps;
 size_t start=0;int previousNumber=-1;
 while(start<s.samples.size()){
  size_t end=start+1;while(end<s.samples.size()&&s.samples[end].lap==s.samples[start].lap&&s.samples[end].stint==s.samples[start].stint)++end;
  Lap l;l.number=s.samples[start].lap;l.partial=s.samples[start].progress>.02||s.samples[start].ms>2000;
  l.valid=s.samples[start].valid;std::array<double,11> crossing;crossing.fill(NAN);
  if(start&&previousNumber+1==l.number&&s.samples[start-1].stint==s.samples[start].stint&&s.samples[start-1].progress>.9&&s.samples[start].progress<.1&&s.samples[start].at>=s.samples[start-1].at&&s.samples[start].at-s.samples[start-1].at<=500)crossing[0]=0;
  if(s.samples[start].progress==0&&s.samples[start].ms==0)crossing[0]=0;
  int prevSector=-1,pendingSector=-1,carriedSector=0;uint64_t sectorAt=0;
  for(size_t i=start;i<end;++i){auto& p=s.samples[i];l.pit|=p.pit;if(p.valid==0)l.valid=0;else if(p.valid<0&&l.valid!=0)l.valid=-1;
   for(int k=0;k<3;++k)if(timeOK(p.splits[k]))l.sectors[k]=p.splits[k];
   // Only latch a sector on an observed crossing, not repeatedly inside next sector.
   if(prevSector>=0&&p.sector==prevSector+1&&p.sector<3&&i>start){pendingSector=prevSector;carriedSector=s.samples[i-1].lastSector;sectorAt=p.at;}
   if(pendingSector>=0){if(p.at<sectorAt||p.at-sectorAt>500||p.sector!=pendingSector+1)pendingSector=-1;
    else if(timeOK(p.lastSector)&&p.lastSector!=carriedSector){l.sectors[pendingSector]=p.lastSector;pendingSector=-1;}}
   prevSector=p.sector;
   if(i==start)continue;auto& a=s.samples[i-1];if(p.at<a.at||p.at-a.at>500)continue;
   if(p.progress<=a.progress||p.ms<a.ms)continue;
   for(int k=1;k<10;++k){double q=k/10.;if(a.progress<=q&&p.progress>=q)crossing[k]=a.ms+(p.ms-a.ms)*(q-a.progress)/(p.progress-a.progress);}
  }
  if(end<s.samples.size()&&s.samples[end].lap==l.number+1&&s.samples[end].stint==s.samples[start].stint){
   const auto& a=s.samples[end-1];const auto& b=s.samples[end];
   // Do not take the carried last-lap field at the first counter increment.
   // Wait for its update, or for a stable identical time compatible with a
   // tightly observed finish. A restart/teleport is not a completed lap.
   bool finish=a.progress>.9&&b.progress<.1&&b.at>=a.at;
   for(size_t i=end;finish&&i<s.samples.size()&&s.samples[i].lap==l.number+1&&s.samples[i].stint==a.stint&&s.samples[i].ms<3000;++i){
    const auto& p=s.samples[i];if(!timeOK(p.last)||p.last<a.ms)continue;
    bool tight=b.at-a.at<=500;
    if(tight&&std::abs(double(p.last)-(a.ms+double(b.at-a.at)-b.ms))>150)continue;
    bool updated=p.last!=a.last;
    bool stableSame=tight&&p.at>=b.at&&p.at-b.at>=500;
    if(updated||stableSame){l.ms=p.last;l.complete=true;}
   }
   if(l.complete&&a.progress>.9&&b.progress<.1&&b.at>=a.at&&b.at-a.at<=500)crossing[10]=l.ms;
   if(l.complete&&l.sectors[0]&&l.sectors[1]&&int64_t(l.ms)>int64_t(l.sectors[0])+l.sectors[1])l.sectors[2]=l.ms-l.sectors[0]-l.sectors[1];
  }
  for(int k=0;k<10;++k)if(std::isfinite(crossing[k])&&std::isfinite(crossing[k+1])&&crossing[k+1]>=crossing[k])l.micro[k]=crossing[k+1]-crossing[k];
  s.laps.push_back(l);previousNumber=l.number;start=end;
 }
}
inline double localDelta(const Session& s){
 if(s.samples.empty())return NAN;const auto& p=s.samples.back();if(std::isfinite(p.delta))return p.delta;
 int best=0,lap=-1;for(auto& l:s.laps)if(l.number!=p.lap&&l.complete&&!l.partial&&!l.pit&&l.valid!=0&&l.ms&&(!best||l.ms<best)){best=l.ms;lap=l.number;}
 for(size_t i=1;i<s.samples.size();++i){auto& a=s.samples[i-1];auto& b=s.samples[i];if(a.lap==lap&&b.lap==lap&&a.progress<=p.progress&&b.progress>=p.progress&&b.progress>a.progress&&b.at>=a.at&&b.at-a.at<=500)return p.ms-(a.ms+(b.ms-a.ms)*(p.progress-a.progress)/(b.progress-a.progress));}return NAN;
}
inline Session loadCSV(const std::filesystem::path& path,bool metadataOnly=false){
 Session s;s.path=path;s.id=path.stem().string();s.game="AC";s.track=s.id;
 std::ifstream f(path);std::string row;if(!std::getline(f,row))return s;auto names=cells(row);std::map<std::string,size_t> cols;for(size_t i=0;i<names.size();++i)cols[names[i]]=i;
 size_t count=0;int previousRaw=-1,previousStint=-1,lapOffset=0;while(std::getline(f,row)&&count++<1000000){if(row.size()>16384)break;auto v=cells(row);if(v.size()<names.size())continue;
  auto str=[&](const char* k){auto it=cols.find(k);return it!=cols.end()&&it->second<v.size()?v[it->second]:std::string();};
  auto n=[&](const char* k,double fallback=0.){try{auto a=str(k);if(a.empty())return fallback;double v=std::stod(a);return std::isfinite(v)&&std::abs(v)<1.e16?v:fallback;}catch(...){return fallback;}};
  auto integer=[&](const char* k,int fallback=0){double v=n(k,fallback);return v>=INT32_MIN&&v<=INT32_MAX?int(v):fallback;};
  Sample p;p.at=uint64_t(std::max(0.,n("capture_ms")));p.packet=integer("physics_packet",-1);p.lap=integer("lap");p.ms=integer("lap_ms");p.last=integer("last_lap_ms");p.best=integer("best_ms");p.sector=integer("sector",-1);p.lastSector=integer("last_sector_ms");p.valid=integer("valid",-1);p.progress=n("progress",NAN);p.speed=n("speed_kmh",NAN);p.gas=n("gas",NAN);p.brake=n("brake",NAN);p.steer=n("steer_native",NAN);p.delta=n("delta_ms",NAN);p.x=n("world_x",NAN);p.z=n("world_z",NAN);p.pit=n("in_pit")!=0;p.gear=integer("gear_native");p.rpm=integer("rpm");for(int i=0;i<3;++i)p.splits[i]=integer(("s"+std::to_string(i+1)).c_str());
  p.sourceHz=integer("sdk_hz");p.missedTicks=uint64_t(std::max(0.,n("missed_ticks")));p.stint=integer("stint");int raw=p.lap;if(previousRaw>=0&&(p.stint!=previousStint||raw<previousRaw)&&!s.samples.empty())lapOffset=s.samples.back().lap+1-raw;previousRaw=raw;previousStint=p.stint;p.lap+=lapOffset;
  if(!str("game").empty())s.game=decode(str("game"));if(!str("track_id").empty())s.track=decode(str("track_id"));s.car=decode(str("car_id"));s.variant=decode(str("layout_id"));s.length=n("track_length_m");if(!str("steer_unit").empty())s.steerUnit=str("steer_unit");s.started=int64_t(n("started_utc_ms"));
  if(metadataOnly)break;
  if(p.lap>=0&&p.ms>=0&&p.ms<INT32_MAX&&std::isfinite(p.progress)&&p.progress>=0&&p.progress<=1)s.samples.push_back(p);
 }
 if(!s.started){auto id=s.id;auto dash=id.find_last_of('-');auto underscore=id.find_last_of('_',dash);if(underscore!=std::string::npos)try{auto stamp=std::stoll(id.substr(underscore+1));if(stamp>1000000000000LL&&stamp<10000000000000LL)s.started=stamp;}catch(...){} }
 if(!metadataOnly)analyze(s);return s;
}
struct Recorder {
 Session session;std::string key,error;std::ofstream file;uint64_t lastFlush=0,lastAnalysis=0;bool online=false,dirty=false,retainSamples=true;int lastPacket=-1;
 void pause(){if(dirty){analyze(session);dirty=false;}if(file.is_open()){file.flush();if(!file)error="No se pudo guardar la sesión";}online=false;}
 void close(){pause();if(file.is_open())file.close();key.clear();lastPacket=-1;}
 void add(const Sample& p,const std::string& identity,const std::string& game,const std::string& track,const std::string& car,const std::filesystem::path& folder,int64_t wall,bool persist=true,const std::filesystem::path& original={}){
  if(p.lap<0||p.ms<0||p.ms>=INT32_MAX||!std::isfinite(p.progress)||p.progress<0||p.progress>1)return;
  if(identity!=key){close();session={};lastFlush=lastAnalysis=0;key=identity;session.game=game;session.track=track;session.car=car;session.started=wall;session.id=safe(track)+"_"+game+"_"+std::to_string(wall);session.steerUnit=game=="iRacing"?"rad":"native";session.path=original;
   if(persist){try{std::filesystem::create_directories(folder);session.path=folder/(session.id+".csv");file.open(session.path);file.imbue(std::locale::classic());file<<std::setprecision(12);file<<"capture_ms,physics_packet,lap,lap_ms,last_lap_ms,progress,speed_kmh,gas,brake,steer_native,gear_native,rpm,world_x,world_z,sector,last_sector_ms,in_pit,valid,s1,s2,s3,game,track_id,car_id,layout_id,track_length_m,steer_unit,started_utc_ms,best_ms,delta_ms,stint,sdk_hz,missed_ticks\n";if(!file)error="No se pudo crear la sesión";}catch(...){error="No se pudo crear la sesión";}}
  }
  online=true;if(p.packet>=0&&p.packet==lastPacket)return;lastPacket=p.packet;
  if(retainSamples&&session.samples.size()<1000000){session.samples.push_back(p);dirty=true;}else if(retainSamples)error="Límite de memoria de gráficas; los datos continúan guardándose";
  if(file.is_open()){file<<p.at<<','<<p.packet<<','<<p.lap<<','<<p.ms<<','<<p.last<<','<<p.progress<<','<<p.speed<<','<<p.gas<<','<<p.brake<<','<<p.steer<<','<<p.gear<<','<<p.rpm<<','<<p.x<<','<<p.z<<','<<p.sector<<','<<p.lastSector<<','<<p.pit<<','<<p.valid;for(int v:p.splits)file<<','<<v;file<<','<<encode(game)<<','<<encode(track)<<','<<encode(car)<<','<<encode(session.variant)<<','<<session.length<<','<<session.steerUnit<<','<<session.started<<','<<p.best<<','<<p.delta<<','<<p.stint<<','<<p.sourceHz<<','<<p.missedTicks<<'\n';
   if(p.at-lastFlush>=1000){file.flush();lastFlush=p.at;if(!file)error="Error al guardar: revisa espacio y permisos";}}
  if(retainSamples&&(p.at-lastAnalysis>=1000||session.laps.empty())){analyze(session);lastAnalysis=p.at;dirty=false;}
 }
 ~Recorder(){try{close();}catch(...){}}
};
// Loading a file is asynchronous. A later game/direct-view choice wins.
struct ReviewRequest {
 uint64_t revision=0,requested=0;int game=-1;
 void cancel(){++revision;}
 void begin(int mode){requested=++revision;game=mode;}
 bool accepts(int mode)const{return requested==revision&&game==mode;}
};
// Minimap mapping uses actual virtual desktop coordinates, including negative origins.
struct ScreenTransform {
 double left=0,top=0,width=1920,height=1080,x=0,y=0,w=800,h=450;
 double scale()const{return std::min(w/std::max(1.,width),h/std::max(1.,height));}
 std::pair<double,double> toMini(double px,double py)const{auto k=scale();return {x+(w-width*k)/2+(px-left)*k,y+(h-height*k)/2+(py-top)*k};}
 std::pair<int,int> toDesktop(double px,double py,int panelW,int panelH)const{auto origin=toMini(left,top);auto k=scale();return {int(std::lround(std::clamp(left+(px-origin.first)/k,left,left+std::max(0.,width-panelW)))),int(std::lround(std::clamp(top+(py-origin.second)/k,top,top+std::max(0.,height-panelH))))};}
};
}

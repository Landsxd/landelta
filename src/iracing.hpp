#pragma once
// Wire layout from iRacing's published SDK (see licenses/iRacing-SDK.txt).
// Read-only local connection: no account, web API, remote control or credentials.
#include "race_studio.hpp"
#include "telemetry.hpp"
#include <cstring>
#include <atomic>
namespace iracing {
struct Buffer {int32_t tick,offset,pad[2];};
struct Header {int32_t version,status,hz,sessionUpdate,sessionLen,sessionOffset,numVars,varOffset,numBuffers,bufferLen,pad[2];Buffer buffers[4];};
struct Variable {int32_t type,offset,count;uint8_t countAsTime,pad[3];char name[32],desc[64],unit[32];};
static_assert(sizeof(Header)==112&&sizeof(Variable)==144,"iRacing ABI");
inline bool range(size_t size,int64_t offset,int64_t length){return offset>=0&&length>=0&&uint64_t(offset)<=size&&uint64_t(length)<=size-uint64_t(offset);}
inline int width(int type){const int sizes[]={1,1,4,4,4,8};return type>=0&&type<6?sizes[type]:0;}
struct Packet {
 Header h{};std::map<std::string,Variable> vars;std::vector<uint8_t> data;std::string yaml;int tick=-1;
 bool read(const uint8_t* bytes,size_t size){
  if(size<sizeof(Header))return false;Header header;memcpy(&header,bytes,sizeof header);
  if(header.version<1||!(header.status&1)||header.numBuffers<1||header.numBuffers>4||header.numVars<0||header.numVars>4096||header.bufferLen<=0||header.bufferLen>4*1024*1024||!range(size,header.varOffset,int64_t(header.numVars)*sizeof(Variable))||header.sessionLen>4*1024*1024||!range(size,header.sessionOffset,header.sessionLen))return false;
  int b=0;for(int i=1;i<header.numBuffers;++i)if(header.buffers[i].tick>header.buffers[b].tick)b=i;
  if(!range(size,header.buffers[b].offset,header.bufferLen))return false;
  std::map<std::string,Variable> next;
  for(int i=0;i<header.numVars;++i){Variable v;memcpy(&v,bytes+header.varOffset+i*sizeof v,sizeof v);if(!width(v.type)||v.count<1||v.count>4096||!range(header.bufferLen,v.offset,int64_t(width(v.type))*v.count)||!memchr(v.name,0,32))return false;next[v.name]=v;}
  std::vector<uint8_t> row(bytes+header.buffers[b].offset,bytes+header.buffers[b].offset+header.bufferLen);
  std::string meta((const char*)bytes+header.sessionOffset,header.sessionLen);if(!meta.empty()&&meta.back()==0)meta.pop_back();
  // Reject torn rows and session metadata. Check after copies, not before.
  std::atomic_thread_fence(std::memory_order_seq_cst);Header after;memcpy(&after,bytes,sizeof after);
  if(after.buffers[b].tick!=header.buffers[b].tick||after.sessionUpdate!=header.sessionUpdate||after.varOffset!=header.varOffset||after.bufferLen!=header.bufferLen||!(after.status&1))return false;
  h=header;tick=header.buffers[b].tick;vars=std::move(next);data=std::move(row);yaml=std::move(meta);return true;
 }
 double get(const std::string& name,int index=0,double missing=NAN)const{auto it=vars.find(name);if(it==vars.end()||index<0||index>=it->second.count)return missing;auto& v=it->second;const auto* p=data.data()+v.offset+index*width(v.type);switch(v.type){case 0:case 1:return *p;case 2:case 3:{int32_t a;memcpy(&a,p,4);return a;}case 4:{float a;memcpy(&a,p,4);return std::isfinite(a)?a:missing;}case 5:{double a;memcpy(&a,p,8);return std::isfinite(a)?a:missing;}}return missing;}
};
inline std::string trim(std::string s){auto a=s.find_first_not_of(" \t\r\n");if(a==std::string::npos)return {};auto b=s.find_last_not_of(" \t\r\n");s=s.substr(a,b-a+1);if(s.size()>1&&((s.front()=='"'&&s.back()=='"')||(s.front()=='\''&&s.back()=='\'')))s=s.substr(1,s.size()-2);return s;}
// Only read scalar fields from the documented sections, never execute YAML tags.
inline std::string field(const std::string& yaml,const std::string& key){std::istringstream stream(yaml);std::string line;while(std::getline(stream,line)){auto s=trim(line);if(s.rfind(key+":",0)==0)return trim(s.substr(key.size()+1));}return {};}
inline std::map<int,std::map<std::string,std::string>> drivers(const std::string& yaml){
 std::map<int,std::map<std::string,std::string>> out;std::istringstream f(yaml);std::string line;bool section=false;int id=-1;
 while(std::getline(f,line)){auto s=trim(line);if(s=="Drivers:"){section=true;continue;}if(!section)continue;if(!line.empty()&&line[0]!=' '&&line[0]!='\t')break;
  if(s.rfind("- CarIdx:",0)==0){try{id=std::stoi(trim(s.substr(9)));}catch(...){id=-1;}if(id<0||id>255)id=-1;continue;}
  auto c=s.find(':');if(id>=0&&c!=std::string::npos)out[id][s.substr(0,c)]=trim(s.substr(c+1));
 }return out;
}
inline int milliseconds(double seconds){return std::isfinite(seconds)&&seconds>0&&seconds<INT32_MAX/1000.?int(seconds*1000+.5):0;}
#ifdef _WIN32
struct Client {
 HANDLE mapping=nullptr;const uint8_t* view=nullptr;size_t size=0;Packet packet;acc::State state;race::Sample sample;
 uint64_t lastNew=0,lastOpen=0;int previousTick=-1;unsigned generation=0;bool connected=false,onTrack=false;std::string identity,car,error;
 void close(){if(view)UnmapViewOfFile(view);if(mapping)CloseHandle(mapping);view=nullptr;mapping=nullptr;connected=onTrack=false;size=0;previousTick=-1;state={};}
 ~Client(){close();}
 bool poll(uint64_t now){
  if(!view){if(now-lastOpen<1000)return false;lastOpen=now;mapping=OpenFileMappingW(FILE_MAP_READ,FALSE,L"Local\\IRSDKMemMapFileName");if(!mapping)return false;view=(const uint8_t*)MapViewOfFile(mapping,FILE_MAP_READ,0,0,0);MEMORY_BASIC_INFORMATION info{};if(!view||!VirtualQuery(view,&info,sizeof info)){close();return false;}size=info.RegionSize;}
  if(!packet.read(view,size)){if(lastNew&&now-lastNew>3000)close();return false;}
  if(packet.tick==previousTick){if(lastNew&&now-lastNew>3000){error="Telemetría detenida; reconectando";close();}return false;}
  bool rewind=previousTick>=0&&packet.tick<previousTick;previousTick=packet.tick;lastNew=now;connected=true;error.clear();
  int sn=int(packet.get("SessionNum",0,-1));auto key=field(packet.yaml,"SessionID")+"/"+field(packet.yaml,"SubSessionID")+"/"+std::to_string(sn)+"/"+field(packet.yaml,"TrackName");
  if(identity!=key||rewind){state={};identity=key;++generation;}
  state.track=field(packet.yaml,"TrackDisplayName");if(state.track.empty())state.track=field(packet.yaml,"TrackName");
  try{state.meters=int(std::stod(field(packet.yaml,"TrackLength"))*1000);}catch(...){state.meters=0;}
  state.registered=state.hasSession=true;state.session=sn;state.epoch=generation;state.sessionReceived=state.received=now;
  state.focus=int(packet.get("PlayerCarIdx",0,-1));state.replay=packet.get("IsReplayPlaying",0,0)!=0;
  onTrack=packet.get("IsOnTrack",0,0)!=0&&!state.replay;
  int phase=int(packet.get("SessionState",0,0));state.phase=phase>=4?5:phase;
  auto sessionType=field(packet.yaml.substr(packet.yaml.find("SessionNum: "+std::to_string(sn))==std::string::npos?0:packet.yaml.find("SessionNum: "+std::to_string(sn))),"SessionType");state.type=sessionType=="Race"?10:(sessionType.find("Qual")!=std::string::npos?4:0);
  state.elapsed=float(packet.get("SessionTime",0,0)*1000);state.end=state.elapsed+float(packet.get("SessionTimeRemain",0,0)*1000);
  auto metadata=drivers(packet.yaml);for(auto& item:metadata){int id=item.first;double progress=packet.get("CarIdxLapDistPct",id);if(!std::isfinite(progress)||progress<0||progress>1)continue;
   auto& c=state.cars[id];c.id=id;c.team=item.second["UserName"];try{c.number=std::stoi(item.second["CarNumber"]);}catch(...){c.number=0;}c.position=int(packet.get("CarIdxPosition",id,0));c.spline=float(progress);c.laps=std::max(0,int(packet.get("CarIdxLapCompleted",id,0)));c.location=packet.get("CarIdxOnPitRoad",id,0)?2:1;c.last.ms=milliseconds(packet.get("CarIdxLastLapTime",id));c.best.ms=milliseconds(packet.get("CarIdxBestLapTime",id));c.seen=now;state.passages[id].add(c,now);
   if(id==state.focus)car=item.second["CarScreenName"];
  }
  race::Sample p;p.at=now;p.packet=packet.tick;p.lap=int(packet.get("Lap",0,0));p.ms=milliseconds(packet.get("LapCurrentLapTime"));p.last=milliseconds(packet.get("LapLastLapTime"));p.best=milliseconds(packet.get("LapBestLapTime"));p.progress=packet.get("LapDistPct");p.gas=packet.get("Throttle");p.brake=packet.get("Brake");p.speed=packet.get("Speed")*3.6;p.steer=packet.get("SteeringWheelAngle");p.gear=int(packet.get("Gear",0,0));p.rpm=int(packet.get("RPM",0,0));p.pit=packet.get("OnPitRoad",0,0)!=0;p.x=packet.get("Lon");p.z=packet.get("Lat");
  // Incident counts are not an official per-lap validity flag.
  p.valid=-1;if(packet.get("LapDeltaToBestLap_OK",0,0))p.delta=packet.get("LapDeltaToBestLap")*1000;sample=p;return true;
 }
};
#endif
}

// Main-thread bridge. AC's existing full-rate CSV stays the source of truth.
// ACC and iRacing record one CSV per detected session, independent of UI selection.
#include "iracing.hpp"
#include <future>
race::Recorder studioRecords[3];iracing::Client irClient;
acstudio::Mapping studioPhysics,studioStatic;
race::Session reviewed;bool reviewing=false,loadingSession=false;std::future<race::Session> loadJob;
int studioPage=0,sessionOffset=0,lapOffset=0,selectedLap=-1,compareLap=-1,sectorMode=0;
double graphCursor=-1;std::wstring studioNotice;std::vector<race::Session> catalog;
const wchar_t* gameLabel(int mode){return mode==0?L"AC":(mode==1?L"ACC":L"iRacing");}
race::Session& currentSession(){return reviewing?reviewed:studioRecords[cfg.gameMode].session;}
void refreshCatalog(){
 catalog.clear();for(auto folder:{dataDir/L"ac-telemetry",dataDir/L"studio-sessions"}){std::error_code ec;if(!fs::exists(folder,ec))continue;
  for(const auto& entry:fs::directory_iterator(folder,ec)){if(!entry.is_regular_file(ec)||entry.path().extension()!=L".csv")continue;
   try{auto s=race::loadCSV(entry.path(),true);if(s.game==utf8(gameLabel(cfg.gameMode)))catalog.push_back(std::move(s));}catch(...){}
  }
 }
 // Existing ACC history is available without importing or moving anything.
 if(cfg.gameMode==1)for(const auto& r:analytics.history){race::Session s;s.id=r.id;s.game="ACC";s.track=r.track;s.car=r.driver;s.started=r.started;for(const auto& l:r.laps){race::Lap a;a.number=l.number;a.ms=l.timing.ms;a.complete=l.complete;a.partial=!l.complete;a.pit=l.pit||l.timing.in||l.timing.out;a.valid=l.timing.invalid?0:(l.timing.valid?1:-1);a.sectors=l.timing.splits;s.laps.push_back(a);}catalog.push_back(std::move(s));}
 std::sort(catalog.begin(),catalog.end(),[](const auto& a,const auto& b){return a.started==b.started?a.id>b.id:a.started>b.started;});sessionOffset=0;
}
void selectSession(int index){if(index<0||index>=int(catalog.size())||loadingSession)return;auto s=catalog[index];selectedLap=compareLap=-1;lapOffset=0;if(s.path.empty()){reviewed=std::move(s);reviewing=true;return;}loadingSession=true;studioNotice=L"Abriendo sesión…";loadJob=std::async(std::launch::async,[p=s.path]{return race::loadCSV(p);});}
void studioTick(uint64_t now){
 if(loadingSession&&loadJob.wait_for(std::chrono::milliseconds(0))==std::future_status::ready){try{reviewed=loadJob.get();reviewing=true;studioNotice=L"Sesión cargada";}catch(...){studioNotice=L"No se pudo abrir la sesión";}loadingSession=false;}
 auto ac=acstudio::studio.read();if(ac.first.active){auto& s=ac.first;studioRecords[0].add(s.frame,utf8(s.path.wstring())+"/"+std::to_string(s.stint),"AC",utf8(s.track),utf8(s.car),{},wallMs(),false,s.path);}else studioRecords[0].online=false;
 if(gameRunning&&sharedOK&&shared.status==2&&!live.replay){
  studioPhysics.open(L"Local\\acpmf_physics",sizeof(acstudio::Physics));studioStatic.open(L"Local\\acpmf_static",sizeof(acstudio::Static));
  acstudio::Physics p{};bool physics=studioPhysics.read(p);race::Sample s;s.at=now;s.packet=physics?p.packet:shared.packet;s.lap=shared.laps+1;s.ms=shared.currentMs;s.last=shared.lastMs;s.best=shared.bestMs;s.progress=shared.spline;s.pit=shared.inPit||shared.inPitLane;s.valid=shared.valid?1:0;s.sector=shared.sector;s.lastSector=shared.lastSector;
  if(physics){s.gas=p.gas;s.brake=p.brake;s.speed=p.speed;s.steer=p.steer;s.gear=p.gear;s.rpm=p.rpm;}
  for(int i=0;i<std::min(60,std::max(0,shared.activeCars));++i)if(shared.carIds[i]==shared.player){s.x=shared.coordinates[i][0];s.z=shared.coordinates[i][2];break;}
  auto c=live.cars.find(shared.player);if(c!=live.cars.end())s.splits=c->second.current.splits;
  std::string track=live.track,car;acstudio::Static info{};if(studioStatic.data){memcpy(&info,studioStatic.data,sizeof info);car=acstudio::label(info.car);if(track.empty())track=acstudio::label(info.track);}
  auto key=std::to_string(gamePID)+"/"+std::to_string(shared.session)+"/"+std::to_string(shared.sessionIndex)+"/"+std::to_string(live.epoch);
  auto& r=studioRecords[1];if(!r.session.samples.empty()&&r.key==key&&s.lap<r.session.samples.back().lap)r.close();
  if(!track.empty())r.add(s,key,"ACC",track,car,dataDir/L"studio-sessions",wallMs());
 }else{studioRecords[1].online=false;studioPhysics.close();studioStatic.close();if(!gameRunning)studioRecords[1].close();}
 bool fresh=irClient.poll(now);if(fresh&&irClient.onTrack){auto& s=irClient.sample;studioRecords[2].add(s,irClient.identity+"/"+std::to_string(irClient.generation),"iRacing",irClient.state.track,irClient.car,dataDir/L"studio-sessions",wallMs());studioRecords[2].session.length=irClient.state.meters;}
 else if(!irClient.onTrack)studioRecords[2].online=false;
 if(!irClient.connected)studioRecords[2].close();
}
bool panelAllowed(int i){return i!=2&&i!=6&&(cfg.gameMode!=0||(i==3||i==4))&&(cfg.gameMode!=2||i!=5);}
bool studioActive(){return cfg.gameMode==1?sessionVisibility.active:(cfg.gameMode==0?studioRecords[0].online:irClient.onTrack);}
bool overlayVisible(int i){return panelAllowed(i)&&!cfg.hidden&&cfg.visible[i]&&(demoMode||studioActive()||cfg.keepAfterRace);}

#include "../src/history.hpp"
#include <cassert>
#include <iostream>
#include <chrono>
using namespace acc;
struct Run {
 Analytics a;State s;uint64_t now=1000;int64_t wall=1791309600000LL;
 Run(){s.hasSession=true;s.track="Monza";s.trackId=1;s.event=1;s.session=1;s.type=10;s.phase=5;s.epoch=1;s.cars[7].id=7;s.cars[7].number=31;s.cars[7].drivers={"L. Landeros"};s.cars[7].location=1;s.cars[7].current.ms=1000;tick();}
 void tick(int player=7){now+=100;s.elapsed+=100;s.cars[7].seen=now;a.observe(s,7,player,now,wall+now);}
 void finish(int ms=109000,bool valid=true){auto& c=s.cars[7];++c.laps;c.current.ms=100;c.last.ms=ms;c.last.valid=valid;c.last.invalid=!valid;c.last.splits={35000,36000,ms-71000};tick();}
 void cleanThree(){finish(109000);finish(109100);finish(109300);finish(109200);}
};
int main(){
 Run r;r.cleanThree();assert(r.a.history.size()==1&&r.a.history[0].laps.size()==4);assert(!r.a.history[0].laps[0].clean());assert(r.a.pace(7)==109.2);r.tick();r.tick();assert(r.a.history[0].laps.size()==4);
 // Last five completed laps, at least three clean, median resistant to one slow lap.
 r.finish(180000);assert(r.a.pace(7)==109.25);r.finish(110000,false);r.finish(109000,false);r.finish(109000,false);assert(!std::isfinite(r.a.pace(7)));
 // A delayed previous-lap update must neither duplicate a record nor reset a stint.
 auto c=r.s.cars[7];--r.s.cars[7].laps;r.tick();r.s.cars[7]=c;assert(r.a.history[0].laps.size()==8);
 // Pit entry/exit and a driver change need fresh observed laps.
 r.s.cars[7].location=2;r.tick();assert(!std::isfinite(r.a.pace(7)));r.s.cars[7].location=1;r.finish();assert(!r.a.history[0].laps.back().clean());r.cleanThree();assert(std::isfinite(r.a.pace(7)));r.s.cars[7].driver=1;r.tick();assert(!std::isfinite(r.a.pace(7)));
 // Transport reconnection resumes the same history, while discarding pace/baselines.
 size_t count=r.a.history[0].laps.size();r.a.disconnect();r.s.epoch=1;r.tick();r.finish();assert(r.a.history.size()==1&&r.a.history[0].laps.size()==count+1&&!r.a.history[0].laps.back().complete);
 // Unknown local player and spectator focus never append personal history.
 Run spectator;spectator.s.focus=7;for(int i=0;i<5;i++){spectator.s.cars[7].laps++;spectator.s.cars[7].last.ms=110000;spectator.tick(-1);}assert(spectator.a.history.empty());
 // No circuit metadata: no guessed circuit or misattributed history.
 Run unknown;unknown.a.endSession();unknown.s.track.clear();unknown.finish();assert(unknown.a.history.empty());
 // Circuit/session/restart boundaries preserve old records and reset pace.
 Run b;b.cleanThree();b.s.track="Spa";b.s.trackId=7;b.s.epoch++;b.s.cars[7].laps=0;b.tick();b.finish(140000);assert(b.a.history.size()==2&&b.a.history[1].track=="Spa"&&b.a.history[1].laps[0].number==1);
 b.s.elapsed=0;b.s.epoch++;b.s.cars[7].laps=0;b.tick();b.finish(141000);assert(b.a.history.size()==3);b.s.type=4;b.s.epoch++;b.s.cars[7].laps=0;b.tick();b.finish();assert(b.a.history.size()==4&&b.a.history.back().type==4);
 b.s.replay=true;b.finish();assert(b.a.history.size()==4&&b.a.trackers.empty());
 Run missing;missing.cleanThree();missing.now+=9000;missing.tick();assert(!std::isfinite(missing.a.pace(7)));missing.finish();assert(!missing.a.history[0].laps.back().complete);
 Run paused;paused.cleanThree();paused.now+=100000;paused.a.pausedAt(paused.now);paused.tick();assert(std::isfinite(paused.a.pace(7)));
 // Forecast arithmetic and guardrails.
 auto f=forecast(6,109,110,1800);assert(f.state==ForecastState::Catching&&f.laps==6&&f.closing==1);
 assert(forecast(6,109,110,100).state==ForecastState::AfterFinish);assert(forecast(6,110,109).state==ForecastState::Opening);assert(forecast(6,109,109.05).state==ForecastState::Stable);assert(forecast(NAN,109,110).state==ForecastState::Waiting);assert(forecast(6,NAN,110).state==ForecastState::Waiting);
 // Physical proximity with race direction: do not chase lapped traffic.
 State grid;Car me;me.id=7;me.laps=10;me.spline=.98f;me.location=1;me.position=4;me.seen=1000;grid.cars[7]=me;
 Car ahead=me;ahead.id=1;ahead.laps=11;ahead.spline=.01f;ahead.position=3;grid.cars[1]=ahead;Car lapped=ahead;lapped.id=2;lapped.laps=10;lapped.spline=.99f;lapped.position=8;grid.cars[2]=lapped;
 assert(paceRival(grid,me,true,1100)->id==1);grid.cars[1].location=2;assert(!paceRival(grid,me,true,1100));grid.cars[1]=ahead;assert(!paceRival(grid,me,true,6000));
 // Round-trip persistence, incremental overwrite, malformed-file preservation,
 // write errors retaining pending records, plus independently named session files.
 namespace fs=std::filesystem;auto dir=fs::temp_directory_path()/("vortex-history-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 HistoryStore store;store.directory=dir;store.flush(r.a);assert(r.a.dirty.empty()&&store.error.empty());Analytics loaded;store.load(loaded);assert(loaded.history.size()==1&&encodeHistory(loaded.history[0])==encodeHistory(r.a.history[0]));
 r.finish(109999);store.flush(r.a);Analytics updated;store.load(updated);assert(updated.history[0].laps.back().timing.ms==109999);
 auto malformed=dir/"999.json";{std::ofstream out(malformed);out<<"{broken";}Analytics recovered;store.load(recovered);assert(recovered.history.size()==1&&store.skipped==1&&fs::exists(malformed)&&!store.error.empty());
 auto bad=encodeHistory(r.a.history[0]);bad["id"]="../escape";try{decodeHistory(bad);assert(false);}catch(const std::exception&){}bad=encodeHistory(r.a.history[0]);bad["laps"][0]["splits"]=std::vector<int>{1,2};try{decodeHistory(bad);assert(false);}catch(const std::exception&){}
 store.directory=dir/"not-a-folder";{std::ofstream out(store.directory);out<<"occupied";}r.finish();store.flush(r.a);assert(!r.a.dirty.empty()&&!store.error.empty());store.directory=dir;store.flush(r.a);assert(r.a.dirty.empty());
 store.flush(b.a);Analytics all;store.load(all);assert(all.history.size()==5);fs::remove_all(dir);
 std::cout<<"PASS: observed clean pace, duplicates, invalid/pit/stint/stale data, session/track/replay reset, reconnect continuity, own-driver gating, forecasts, rival selection, history round-trip, corrupt preservation and failed-write retry\n";
}

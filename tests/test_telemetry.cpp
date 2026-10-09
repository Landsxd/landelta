#include "../src/runtime.hpp"
#include <cassert>
#include <iostream>
#include <random>
using namespace acc;
void lap(Writer& w,int ms,bool invalid=false){w.put<int32_t>(ms);w.put<uint16_t>(7);w.put<uint16_t>(0);w.put<uint8_t>(3);w.put<int32_t>(35000);w.put<int32_t>(36000);w.put<int32_t>(ms==INT32_MAX?INT32_MAX:ms-71000);w.put<uint8_t>(invalid);w.put<uint8_t>(!invalid);w.put<uint8_t>(0);w.put<uint8_t>(0);}
Writer car(int id=7,int laps=5,float spline=.98f,int position=2,int ms=INT32_MAX){Writer w;w.put<uint8_t>(3);w.put<uint16_t>(id);w.put<uint16_t>(0);w.put<uint8_t>(1);w.put<uint8_t>(5);w.put<float>(5);w.put<float>(6);w.put<float>(1);w.put<uint8_t>(1);w.put<uint16_t>(222);w.put<uint16_t>(position);w.put<uint16_t>(2);w.put<uint16_t>(2);w.put<float>(spline);w.put<uint16_t>(laps);w.put<int32_t>(-320);lap(w,109000);lap(w,110000,true);lap(w,ms);return w;}
Writer entry(){Writer w;w.put<uint8_t>(6);w.put<uint16_t>(7);w.put<uint8_t>(30);w.str("Vortex Racing");w.put<int32_t>(46);w.put<uint8_t>(0);w.put<uint8_t>(0);w.put<uint16_t>(29);w.put<uint8_t>(1);w.str("Eduardo");w.str("Landeros");w.str("LAN");w.put<uint8_t>(1);w.put<uint16_t>(29);return w;}
Writer session(int event,int sess,float ms,bool replay=false,int type=10,int phase=5,float replayMs=-1){Writer w;w.put<uint8_t>(2);w.put<uint16_t>(event);w.put<uint16_t>(sess);w.put<uint8_t>(type);w.put<uint8_t>(phase);w.put<float>(ms);w.put<float>(1800000);w.put<int32_t>(7);w.str("Drivable");w.str("Cockpit");w.str("Basic HUD");w.put<uint8_t>(replay);if(replay){w.put<float>(replayMs<0?ms:replayMs);w.put<float>(5000);}w.put<float>(43200);for(int i=0;i<5;i++)w.put<uint8_t>(0);lap(w,108000);return w;}
Writer event(int type,int id,const std::string& text){Writer w;w.put<uint8_t>(7);w.put<uint8_t>(type);w.str(text);w.put<int32_t>(2000);w.put<int32_t>(id);return w;}
Writer track(int id,const std::string& name,int connection=0){Writer w;w.put<uint8_t>(5);w.put<int32_t>(connection);w.str(name);w.put<int32_t>(id);w.put<int32_t>(5793);w.put<uint8_t>(1);w.str("Drivable");w.put<uint8_t>(1);w.str("Cockpit");w.put<uint8_t>(1);w.str("HUD");return w;}
int feed(State& s,const Writer& w,uint64_t now=1000){return s.parse(w.b.data(),w.b.size(),now);}
void sessionTests(){
 State s;for(int type:{0,4,10,12,13,9,11}){assert(feed(s,session(1,1,20000,false,type))==12);assert(s.cars.empty());feed(s,car());feed(s,event(3,7,"test"));assert(s.cars.size()==1);}
 assert(feed(s,session(1,1,21000,false,11))==2);assert(feed(s,session(1,1,20500,false,11))==0);assert(s.elapsed==21000&&s.cars.size()==1);
 assert(feed(s,session(1,1,0,false,11))==0);assert(s.cars.size()==1);assert(feed(s,session(1,1,22000,false,11))==2);assert(s.cars.size()==1);
 assert(feed(s,session(1,1,0,false,11))==0);assert(feed(s,session(1,1,100,false,11))==12);assert(s.cars.empty()&&s.passages.empty());
 feed(s,car());assert(feed(s,session(1,1,100,false,11,6))==2);assert(feed(s,session(1,1,100,false,11,5))==12);assert(s.cars.empty());
 feed(s,car());assert(feed(s,session(1,1,100,true,11,5,20000))==12);feed(s,car());assert(feed(s,session(1,1,100,true,11,5,21000))==2);assert(s.cars.size()==1);assert(feed(s,session(1,1,100,true,11,5,5000))==12);assert(s.cars.empty());
 feed(s,car());assert(feed(s,session(1,1,100,false,11))==12);assert(s.cars.empty());
 feed(s,track(1,"Monza"));feed(s,car());auto tr=track(2,"Spa");for(size_t n=0;n<tr.b.size();n++){assert(s.parse(tr.b.data(),n,2000)==-1);assert(s.track=="Monza"&&s.cars.size()==1);}assert(feed(s,tr)==12);assert(s.track=="Spa"&&s.cars.empty()&&!s.hasSession);
 s.registered=true;s.connection=42;assert(feed(s,track(3,"Nurburgring",10))==0);assert(s.track=="Spa");
}
void timingTests(){
 const std::array<int,3> empty{};
 PlayerTiming t;Graphics g{};g.status=2;g.player=7;g.session=2;g.sessionIndex=1;g.valid=1;g.packet=1;g.currentMs=20000;t.update(g,1000);assert(t.valid&&timingView(nullptr,&t,1000).lap==1);
 g.packet++;g.sector=1;g.lastSector=35000;g.currentMs=36000;t.update(g,2000);assert(t.current[0]==35000);
 g.packet++;g.sector=2;g.lastSector=39000;g.currentMs=75000;t.update(g,3000);assert(t.current[1]==39000);
 g.packet++;g.lastSector=39100;t.update(g,3100);assert(t.current[1]==39100);Car exact;exact.id=7;exact.laps=0;exact.current.ms=75000;exact.current.splits={35000,39000,0};exact.seen=3100;assert(timingView(&exact,&t,3100).splits[1]==39000);g.packet++;g.lastSector=39000;t.update(g,3200);
 g.packet++;g.sector=0;g.laps=1;g.lastMs=112000;g.currentMs=200;t.update(g,4000);auto v=timingView(nullptr,&t,4000);assert(v.lap==2&&v.previous&&v.splits[2]==38000);
 g.packet++;g.currentMs=6000;t.update(g,10000);v=timingView(nullptr,&t,10000);assert(!v.previous&&v.splits==empty);
 Car old;old.id=7;old.laps=0;old.seen=10000;old.current.ms=6000;old.current.splits={35000,39000,38000};assert(timingView(&old,&t,10000).splits==empty);
 g.packet++;g.sessionIndex=2;g.laps=0;g.currentMs=0;assert(t.update(g,11000)==SharedChange::Session);assert(t.current==empty&&t.previous==empty);
 g.packet++;g.sector=2;g.lastSector=38000;g.currentMs=78000;t.update(g,12000);assert(!t.current[0]&&t.current[1]==38000);g.packet++;g.sector=0;g.currentMs=0;assert(t.update(g,13000)==SharedChange::LapReset);assert(t.current==empty);
 g.packet++;g.session=0;assert(t.update(g,14000)==SharedChange::Session);g.packet++;g.status=3;t.update(g,15000);t.update(g,500000);assert(t.valid);g.status=2;t.update(g,500100);assert(t.valid);t.update(g,503100);assert(!t.valid&&t.expired(503100));
 int packet=g.packet;t.waitForPacket(packet);t.update(g,504000);assert(!t.valid);g.packet++;t.update(g,504100);assert(t.valid);
 g.status=0;g.player=-1;assert(t.update(g,505000)==SharedChange::Offline);assert(!t.valid&&t.current==empty);g.player=7;g.status=2;g.packet++;t.update(g,506000);assert(t.valid&&!t.previous[0]);g.status=1;t.update(g,507000);assert(!t.valid);
 assert(broadcastSession(0)==0&&broadcastSession(1)==4&&broadcastSession(2)==10&&broadcastSession(7)==12);
}
void connectionTests(){
 assert(!gridTimedOut(7999,0,0,true,true,false));assert(gridTimedOut(8000,0,0,true,true,false));
 assert(!gridTimedOut(9000,0,2000,true,true,false));assert(!gridTimedOut(9000,0,0,true,true,true));
 assert(!gridTimedOut(9000,0,0,false,true,false));assert(!gridTimedOut(9000,0,0,true,false,false));
 ReconnectPolicy r;assert(r.due(0));r.begin(1000);assert(!r.due(1001)&&!r.timedOut(3999)&&r.timedOut(4000));r.failed(4000);assert(!r.due(4999)&&r.due(5000));r.begin(5000);r.failed(8000);assert(r.next==10000);for(int i=0;i<8;i++)r.failed(10000);assert(r.next==20000);r.connected();assert(!r.awaiting&&!r.failures&&r.due(20000));
}
void passageTests(){
 State s;s.type=10;s.phase=5;Car leader,me;leader.id=1;leader.laps=1;leader.location=1;leader.best.ms=100000;leader.position=1;me=leader;me.id=2;me.position=2;me.spline=.02f;me.seen=4000;
 for(auto pair:std::vector<std::pair<float,uint64_t>>{{0,1000},{.01f,2000},{.04f,3000}}){leader.spline=pair.first;leader.seen=pair.second;s.passages[1].add(leader,pair.second);}leader.seen=4000;
 assert(std::abs(s.relativeEstimate(leader,me)+1.6666667)<.001);assert(std::abs(s.relativeEstimate(me,leader)-1.6666667)<.001);assert(s.gapToLeader(me,leader)=="~+1.7");
 leader.location=2;s.passages[1].add(leader,4100);assert(s.passages[1].samples.empty());leader.location=1;leader.spline=.5f;s.passages[1].add(leader,4200);leader.spline=.1f;s.passages[1].add(leader,4300);assert(s.passages[1].samples.size()==1);
 s.clearSession();assert(s.passages.empty());
}
void gridTests(){
 State s;uint64_t now=1000;
 for(int type:{0,4,10}){
  assert(feed(s,session(1,1,0,false,type),now)==12);assert(s.cars.empty()&&s.passages.empty());
  for(int frame=0;frame<1200;frame++){
   feed(s,session(1,1,float(frame*100),false,type),now);
   for(int id=0;id<60;id++){double p=frame/1090.+(59-id)*.003;assert(feed(s,car(id,int(p),float(p-std::floor(p)),id+1,int((p-std::floor(p))*109000)),now)==3);}
   assert(s.active(now).size()==60);now+=100;
  }
 }
 assert(s.cars[0].laps==1&&s.cars[59].laps==1);assert(s.passages[0].samples.size()<=6000);
 assert(feed(s,session(1,1,0,false,10),now)==0);assert(feed(s,session(1,1,100,false,10),now+100)==12);assert(s.cars.empty()&&s.passages.empty());
 feed(s,car(1,0,.1f,0,1000),now+200);assert(s.active(now+200).size()==1&&s.gapToLeader(s.cars[1],s.cars[1])=="--");
 assert(initial("Álvaro")=="Á"&&initial("李")=="李"&&initial("").empty());
 std::cout<<"PASS: 216000 car updates, 60 cars across practice/qualifying/race, same-ID restart, unclassified cars, UTF-8 initials\n";
}
int main(){
 sessionTests();timingTests();connectionTests();passageTests();gridTests();
 auto reg=registration("a");assert(reg.b[0]==1&&reg.b[1]==4);Reader r{reg.b.data()+2,reg.b.size()-2};assert(r.str()=="LANDELTA ACC Overlay");assert(r.str()=="a");assert(r.i32()==100);assert(r.str().empty());
 State s;auto c=car();assert(s.parse(c.b.data(),c.b.size(),1000)==3);assert(s.cars[7].speed==222&&s.cars[7].position==2);assert(s.cars[7].best.ms==109000);assert(s.cars[7].last.invalid);assert(s.cars[7].current.ms==0&&s.cars[7].current.splits[2]==0);
 auto e=entry();assert(s.parse(e.b.data(),e.b.size(),1000)==6);assert(s.cars[7].number==46&&s.cars[7].name()=="E. Landeros");assert(s.cars[7].speed==222);assert(s.active(4999).size()==1&&s.active(5000).empty());
 for(size_t i=0;i<c.b.size();i++){auto before=s.cars[7].speed;assert(s.parse(c.b.data(),i,1500)==-1);assert(s.cars[7].speed==before);assert(s.cars[7].seen==1000);}for(size_t i=0;i<e.b.size();i++){assert(s.parse(e.b.data(),i,1500)==-1);assert(s.cars[7].name()=="E. Landeros");}
 auto u=session(1,1,20000);assert(s.parse(u.b.data(),u.b.size(),2000)==12);assert(s.cars.empty());s.parse(c.b.data(),c.b.size(),2100);u=session(1,1,21000);assert(s.parse(u.b.data(),u.b.size(),2200)==2&&s.cars.size()==1);u=session(1,2,0);assert(s.parse(u.b.data(),u.b.size(),2300)==12&&s.cars.empty());assert(s.sessionBest.ms==108000);
 Car a,b;a.id=1;a.best.ms=100000;a.spline=.98f;a.laps=5;b.id=2;b.best.ms=101000;b.spline=.02f;b.laps=6;assert(std::abs(relativeGap(b,a)+4)<.001);assert(std::abs(relativeGap(a,b)-4.04)<.001);a.best.ms=0;a.last.ms=0;b.best.ms=0;assert(std::isnan(relativeGap(b,a)));a.best.ms=100000;b.best.ms=101000;assert(raceGap(b,a,false)=="+1.000");assert(raceGap(a,a,true)=="LIDER");b.laps=3;assert(raceGap(b,a,true)=="+2 V");
 // A malformed session must not erase the existing grid.
 s.parse(c.b.data(),c.b.size(),3000);u=session(9,9,0);assert(s.parse(u.b.data(),u.b.size()-1,4000)==-1);assert(s.cars.size()==1);
 auto notice=event(3,7,"Drive through / aviso");for(size_t i=0;i<notice.b.size();i++){assert(s.parse(notice.b.data(),i,4000)==-1);assert(s.cars[7].penaltyNoticeAt==0);}assert(s.parse(notice.b.data(),notice.b.size(),4000)==7);assert(s.cars[7].penaltyMessage=="Drive through / aviso");assert(penaltyBadge(s.cars[7],63999)==PenaltyBadge::Notice);assert(penaltyBadge(s.cars[7],64000)==PenaltyBadge::None);assert(penaltyBadge(s.cars[7],5000,true,7)==PenaltyBadge::Confirmed);assert(penaltyBadge(s.cars[7],5000,true,0)==PenaltyBadge::None);assert(penaltyBadge(s.cars[7],5000,true,-1)==PenaltyBadge::Notice);
 s.parse(c.b.data(),c.b.size(),5000);s.parse(e.b.data(),e.b.size(),5000);assert(s.cars[7].penaltyNoticeAt==4000);auto accident=event(4,7,"Accident");s.parse(accident.b.data(),accident.b.size(),5000);assert(s.cars[7].penaltyMessage=="Drive through / aviso");auto global=event(3,-1,"Global");s.parse(global.b.data(),global.b.size(),5000);assert(s.cars.size()==1);u=session(10,10,0);s.parse(u.b.data(),u.b.size(),5000);assert(s.cars.empty());
 assert(formatTimeMs(59999)=="0:59.999");assert(formatTimeMs(60000)=="1:00.000");assert(formatTimeMs(94340)=="1:34.340");assert(formatTimeMs(3600000)=="60:00.000");assert(formatTimeMs(0)=="--:--.---"&&formatTimeMs(INT32_MAX)=="--:--.---");
 a.laps=5;a.spline=.98f;b.laps=6;b.spline=.02f;assert(lapRelation(b,a,true)==0);b.laps=5;assert(lapRelation(b,a,true)==-1);assert(relativeGap(b,a)<0);b.laps=7;b.spline=.96f;assert(lapRelation(b,a,true)==1);assert(relativeGap(b,a)>0);assert(lapRelation(b,a,false)==0);b.position=999;b.laps=5;b.spline=.99f;assert(relativeGap(b,a)<0);b.position=1;assert(relativeGap(b,a)<0);
 std::mt19937 rng(47);for(int i=0;i<10000;i++){std::vector<uint8_t> bytes(rng()%128);for(auto& v:bytes)v=rng()%256;State fuzz;fuzz.parse(bytes.data(),bytes.size(),6000);}assert(offsetof(Graphics,sector)==164&&offsetof(Graphics,player)==1216&&sizeof(Graphics)==1416);
 std::cout<<"PASS: session/phase/track/replay resets, delayed packets, shared lap lifecycle + pause + garage + packet gating, reconnection backoff, measured passages, penalties, 10k fuzz packets, shared layout\n";
}

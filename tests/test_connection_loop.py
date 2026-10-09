"""Compile the production connection functions with deterministic Win32/socket fakes.
Exercises application wiring. This does not replace an ACC/Windows live test.
"""
from pathlib import Path
import subprocess
import tempfile
import os

root = Path(__file__).resolve().parents[1]
source = (root / "src/main.cpp").read_text()
functions = source[source.index("void closeInputs"):source.index("void generateDemo")]
test_source = (root / "tests/test_telemetry.cpp").read_text()
fixtures = test_source[test_source.index("void lap("):test_source.index("void sessionTests")]
shim = r'''
#include "runtime.hpp"
#include "analytics.hpp"
#include "driving.hpp"
#include <cassert>
#include <iostream>
#include <deque>
using namespace acc;
using SOCKET=int;using HANDLE=void*;using u_long=unsigned long;using u_short=unsigned short;
constexpr int INVALID_SOCKET=-1,SOCKET_ERROR=-1,WSAEWOULDBLOCK=10035;
constexpr int AF_INET=2,SOCK_DGRAM=2,IPPROTO_UDP=17,FIONBIO=1,SOL_SOCKET=1,SO_RCVBUF=1,INADDR_LOOPBACK=1,FILE_MAP_READ=1,FALSE=0;
struct sockaddr{};struct sockaddr_in {int sin_family;struct {int s_addr;}sin_addr;int sin_port;};
int htonl(int v){return v;}int htons(int v){return v;}
int opened=0,closed=0,recvError=WSAEWOULDBLOCK;bool available=true;Graphics mapped{};PhysicsInput mappedInputs{};bool inputsAvailable=true;
std::deque<std::vector<uint8_t>> incoming;std::vector<std::vector<uint8_t>> outgoing;
int socket(int,int,int){return ++opened;}int connect(int,sockaddr*,int){return 0;}
int ioctlsocket(int,int,u_long*){return 0;}int setsockopt(int,int,int,char*,int){return 0;}
int send(int,const char* b,int n,int){outgoing.emplace_back(b,b+n);return n;}
int recv(int,char* b,int n,int){if(incoming.empty())return SOCKET_ERROR;auto p=incoming.front();incoming.pop_front();assert(int(p.size())<=n);memcpy(b,p.data(),p.size());return int(p.size());}
void closesocket(int){++closed;incoming.clear();}int WSAGetLastError(){return recvError;}
HANDLE OpenFileMappingW(int,int,const wchar_t* name){if(std::wstring(name).find(L"physics")!=std::wstring::npos)return inputsAvailable?&mappedInputs:nullptr;return available?&mapped:nullptr;}
const void* MapViewOfFile(HANDLE h,int,int,int,size_t){return h;}void UnmapViewOfFile(const void*){}void CloseHandle(HANDLE){}void MemoryBarrier(){}
std::wstring wide(const std::string& s){return std::wstring(s.begin(),s.end());}
std::array<uint64_t,9> udpReceived{},udpAccepted{},udpRejected{};uint64_t udpGeneration=0;uint64_t gatedCars=0,gridWaitAt=0,lastCarPacket=0;std::wstring protocolError;int gridRecoveries=0;
Analytics analytics;bool analyticsTrackReady=false,demoMode=false;int64_t wallMs(){return 1791309600000LL;}
State live;Graphics shared{};PlayerTiming playerTiming;LapDelta lapDelta;DeltaSnapshot retainedDelta;ReconnectPolicy retry;SOCKET sock=INVALID_SOCKET;
HANDLE inputMapping=nullptr;const void* inputView=nullptr;PedalState pedals;uint64_t inputRetryAt=0;
HANDLE mapping=nullptr;const void* view=nullptr;bool sharedOK=false,sharedOffline=false,gameRunning=true,configReady=true;
int port=9000,scroll=0,networkReconnects=0,sessionResets=0,rejectedPackets=0;std::string password;
std::wstring networkError,lastReset;uint64_t lastEntry=0,connectedAt=0;
'''
cases = r'''
Writer registered(bool success=true){Writer w;w.put<uint8_t>(1);w.put<int32_t>(42);w.put<uint8_t>(success);w.put<uint8_t>(1);w.str(success?"":"password rejected");return w;}
void queue(const Writer& w){incoming.push_back(w.b);}
void handshake(uint64_t now){queue(registered());queue(session(1,1,1000));queue(car());queue(entry());queue(track(1,"Monza",42));pollUDP(now);assert(live.registered&&live.cars.size()==1&&live.hasSession);}
int main(){
 sharedOK=true;shared.status=2;mappedInputs={1,.7f,.2f};pollInputs(100);assert(pedals.valid&&pedals.value.gas==.7f);pollInputs(1100);assert(!pedals.valid&&!inputView);pollInputs(1400);assert(!pedals.valid);mappedInputs.packet++;pollInputs(1433);assert(pedals.valid);
 shared.status=3;pollInputs(1466);assert(!inputView&&!pedals.valid);shared.status=2;pollInputs(1500);assert(pedals.valid);live.replay=true;pollInputs(1533);assert(!pedals.valid&&!inputView);live.replay=false;sharedOK=false;
 pollUDP(1000);assert(opened==1&&retry.awaiting&&outgoing[0][0]==1);handshake(1100);assert(!retry.awaiting&&live.track=="Monza");
 lapDelta.best=123;queue(session(1,2,1200));pollUDP(1200);assert(lapDelta.best==0&&udpGeneration==1);
 // Pending good data is read before the eight-second timeout.
 queue(session(1,1,9000));queue(car());pollUDP(9500);assert(opened==1&&live.registered);
 pollUDP(17500);assert(sock==INVALID_SOCKET&&live.cars.empty()&&!retry.due(18000));pollUDP(18500);assert(opened==2&&retry.awaiting);handshake(18600);
 // Real receive errors must not be mistaken for an empty nonblocking queue.
 recvError=10054;pollUDP(18700);assert(sock==INVALID_SOCKET&&!networkError.empty());recvError=WSAEWOULDBLOCK;pollUDP(19700);assert(opened==3);queue(registered(false));pollUDP(19800);assert(!live.registered&&networkError.find(L"password rejected")!=std::wstring::npos);
 // Going to the menu clears all data and suppresses connection retries.
 mapped.status=0;mapped.player=-1;pollShared(20000);int count=opened;pollUDP(100000);assert(opened==count&&sharedOffline&&live.cars.empty()&&!sharedOK);
 mapped={};mapped.packet=1;mapped.status=2;mapped.session=2;mapped.player=7;mapped.valid=1;pollShared(100100);assert(sharedOK&&!sharedOffline);pollUDP(100100);handshake(100200);
 // UDP registration does not erase independently validated own timing.
 pollShared(100300);assert(sharedOK);mapped.packet++;pollShared(100400);assert(sharedOK);
 // A pinned, frozen shared mapping must stay invalid after being reopened.
 pollShared(103400);assert(!sharedOK);pollShared(103500);assert(!sharedOK);mapped.packet++;pollShared(103600);assert(sharedOK);
 // A local same-type boundary keeps a healthy UDP subscription.
 mapped.packet++;mapped.sessionIndex=2;pollShared(103700);assert(live.cars.empty()&&live.registered&&sharedOK);
 pollUDP(103800);handshake(103900);mapped.packet++;pollShared(104000);mapped.status=3;pollShared(104100);pollUDP(200000);assert(live.registered); // no false timeout while paused
 // Exercise the production analytics hook, including track and own-player gates.
 analytics.endSession();live.clearSession();analyticsTrackReady=false;
 queue(session(1,1,1000));queue(track(1,"Monza",42));queue(car(7,0));pollUDP(200100);
 assert(analytics.trackers.count(7)&&analytics.history.empty());
 sharedOK=true;shared.status=2;shared.session=2;shared.player=7;queue(car(7,1));pollUDP(200200);
 assert(analytics.history.size()==1&&analytics.history[0].laps.size()==1);
 queue(car(7,1));pollUDP(200300);assert(analytics.history[0].laps.size()==1);
 demoMode=true;queue(car(7,2));pollUDP(200400);assert(analytics.history[0].laps.size()==1);
 demoMode=false;sharedOK=false;queue(car(7,3));pollUDP(200500);assert(analytics.history[0].laps.size()==1);
 // Regression: live session heartbeats with a stalled car stream must reconnect.
 sharedOK=false;int recoveryBefore=gridRecoveries;
 queue(session(1,1,20000));pollUDP(209000);
 assert(sock==INVALID_SOCKET&&gridRecoveries==recoveryBefore+1);
 assert(networkError.find(L"pilotos")!=std::wstring::npos);
 pollUDP(210000);handshake(210100);assert(live.active(210100).size()==1);
 assert(udpReceived[3]>0&&udpAccepted[3]>0);
 // A malformed car packet is counted separately from missing UDP traffic.
 Writer malformed;malformed.put<uint8_t>(3);queue(malformed);pollUDP(210200);
 assert(udpRejected[3]==1&&!protocolError.empty());
 // Regression: qualifying -> race, with the shared-memory change arriving first.
 reconnect();closeMapping();mapped={};mapped.packet=1;mapped.status=2;mapped.player=7;mapped.valid=1;mapped.session=1;mapped.sessionIndex=1;
 pollShared(220000);pollUDP(220000);queue(registered());queue(session(2,1,90000,false,4));queue(track(1,"Monza",42));queue(entry());queue(car(7,3));pollUDP(220100);
 mapped.packet++;mapped.laps=3;mapped.currentMs=50000;pollShared(220200);assert(sharedOK);
 int transitionConnections=opened;mapped.packet++;mapped.session=2;mapped.sessionIndex=2;mapped.laps=0;mapped.currentMs=0;
 pollShared(220300);assert(sharedOK&&shared.session==2&&shared.laps==0);assert(live.registered&&opened==transitionConnections&&live.cars.empty());
 queue(session(2,1,91000,false,4));queue(car(7,4));pollUDP(220400);assert(sharedOK&&live.active(220400).empty());
 queue(session(2,2,0,false,10,2));queue(track(1,"Monza",42));queue(entry());queue(car(7,0));pollUDP(220500);
 assert(sharedOK&&live.registered&&live.type==10&&live.active(220500).size()==1&&opened==transitionConnections);
 // The broadcast change can arrive first too; it must not erase readable own timing.
 mapped.packet++;mapped.laps=2;mapped.currentMs=50000;pollShared(220600);
 queue(session(2,3,0,false,4));pollUDP(220700);assert(sharedOK&&shared.session==2);
 mapped.packet++;mapped.session=1;mapped.sessionIndex=3;mapped.laps=0;mapped.currentMs=0;pollShared(220800);
 queue(track(1,"Monza",42));queue(car(7,0));pollUDP(220900);assert(sharedOK&&live.registered&&live.type==4&&live.active(220900).size()==1);
 // Manual reconnect only restarts UDP, including when called repeatedly.
 for(int k=0;k<3;k++){int packet=playerTiming.g.packet;reconnect();assert(sharedOK&&playerTiming.valid&&playerTiming.g.packet==packet);pollUDP(221000+k*100);queue(registered());queue(session(2,3,1000+k*100,false,4));queue(track(1,"Monza",42));queue(car(7,0));pollUDP(221050+k*100);assert(live.registered&&sharedOK);}
 // A stalled old-session UDP heartbeat cannot pin a frozen shared mapping forever.
 queue(session(2,4,1500,false,10));pollUDP(221500);pollShared(224000);assert(!sharedOK);mapped.packet++;mapped.session=2;mapped.sessionIndex=4;pollShared(224100);assert(sharedOK);
 std::cout<<"PASS: qualifying/race boundaries in both arrival orders, delayed old-session cars, manual reconnection without losing own timing, stale-memory recovery\n";
 std::cout<<"PASS: stalled-grid recovery despite healthy session heartbeats, grid restored, packet diagnostics\n";
 std::cout<<"PASS: production analytics routing, metadata gate, own-player gate, duplicates and demo isolation\n";
 std::cout<<"PASS: production UDP/memory loops: handshake, timeout recovery, queue draining, socket errors, auth rejection, menu, stale mappings, session restart, pause\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
    p = Path(tmp)
    capture = source[source.index("void captureAnalytics"):source.index("uint64_t nowMs")]
    (p / "connections.cpp").write_text(shim + fixtures + capture + functions + cases)
    subprocess.run(["g++", "-std=c++17", "-g", "-fsanitize=address,undefined", "-I", str(root / "src"), str(p / "connections.cpp"), "-o", str(p / "connections")], check=True)
    subprocess.run([str(p / "connections")], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})

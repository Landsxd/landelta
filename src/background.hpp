#pragma once
#include <cstdint>
namespace acc {
// Gate the overlays on an actual session, not just the ACC launcher process.
// A brief telemetry interruption should not make the entire HUD flash off.
struct SessionVisibility {
 bool active=false;uint64_t lastSignal=0;
 bool update(uint64_t now,bool running,bool menu,bool sharedSession,bool udpSession){
  if(!running||menu){active=false;lastSignal=0;return false;}
  if(sharedSession||udpSession){active=true;lastSignal=now;}
  else if(active&&(now<lastSignal||now-lastSignal>=3000))active=false;
  return active;
 }
 bool show(bool demo,bool hidden,bool enabled)const{return enabled&&!hidden&&(demo||active);}
};
// Learn each source independently: a late UDP registration is not a new race.
struct SessionLaunch {
 bool wasActive=false,haveShared=false,haveUDP=false;
 int sharedType=0,index=0,laps=0,event=0,session=0,type=0,lapMs=-1;
 float elapsed=-1;uint64_t generation=0;
 void reset(){*this=SessionLaunch{};}
 bool started(bool active,bool sharedReady,int st,int si,int lap,bool udpReady,int ev,int se,int ty,int currentMs=-1,float udpElapsed=-1,uint64_t udpGeneration=0){
  if(!active){reset();return false;}
  bool start=!wasActive;
  if(sharedReady){if(haveShared&&(st!=sharedType||si!=index||lap<laps||(lap==laps&&currentMs>=0&&lapMs>=0&&double(currentMs)+1500<lapMs)))start=true;haveShared=true;sharedType=st;index=si;laps=lap;lapMs=currentMs;}
  if(udpReady){if(haveUDP&&(ev!=event||se!=session||ty!=type||udpGeneration!=generation||(udpElapsed>=0&&elapsed>=0&&double(udpElapsed)+5000<elapsed)))start=true;haveUDP=true;event=ev;session=se;type=ty;elapsed=udpElapsed;generation=udpGeneration;}
  wasActive=true;return start;
 }
};
inline unsigned backgroundPollInterval(bool gameRunning,bool demo){return gameRunning||demo?100u:1000u;}
}

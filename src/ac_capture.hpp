#pragma once
// AC (2014), not ACC. Layout prefixes verified against mdjarv/assettocorsasharedmemory.
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>
namespace acstudio {
#pragma pack(push,4)
struct Physics {
 int32_t packet; float gas,brake,fuel; int32_t gear,rpm; float steer,speed;
 float velocity[3],acceleration[3],slip[4],load[4],pressure[4],angularSpeed[4],wear[4],dirty[4],temperature[4],camber[4],travel[4];
 float drs,tc,heading,pitch,roll,cgHeight,damage[5]; int32_t tyresOut,pitLimiter; float abs;
};
struct Graphics {
 int32_t packet,status,session; char16_t current[15],last[15],best[15],split[15];
 int32_t laps,position,currentMs,lastMs,bestMs; float timeLeft,distance; int32_t pit,sector,lastSector,lapsTarget;
 char16_t compound[33]; float replayRate,spline,coordinates[3],penalty; int32_t flag,idealLine,pitLane; float grip; int32_t mandatoryPit;
};
struct Static {
 char16_t smVersion[15],version[15]; int32_t sessions,cars;
 char16_t car[33],track[33],name[33],surname[33],nick[33]; int32_t sectors;
 float maxTorque,maxPower; int32_t maxRpm; float maxFuel,travel[4],radius[4],boost,unused1,unused2;
 int32_t penalties; float fuelRate,tyreRate,damageRate; int32_t blankets; float stability; int32_t autoClutch,autoBlip,drs,ers,kers;
 float kersMax; int32_t engineBrake,ersModes; float length; char16_t layout[15];
};
#pragma pack(pop)
static_assert(offsetof(Graphics,laps)==132 && offsetof(Graphics,spline)==248,"AC graphics ABI");
static_assert(offsetof(Physics,tyresOut)==244 && offsetof(Static,length)==520,"AC physics/static ABI");
struct Point { double progress=0,ms=0; uint64_t captured=0; };
// Spatial interpolation is an estimate, never extra measured samples.
inline std::vector<double> microsectors(const std::vector<Point>& points,int bins){
 bins=std::clamp(bins,2,200);std::vector<double> result(bins,NAN),crossings(bins+1,NAN);
 for(size_t j=1;j<points.size();++j){auto a=points[j-1],b=points[j];
  if(!std::isfinite(a.progress)||!std::isfinite(b.progress)||!std::isfinite(a.ms)||!std::isfinite(b.ms)||b.progress<=a.progress||b.ms<a.ms||b.captured<a.captured||b.captured-a.captured>250)continue;
  for(int k=0;k<=bins;k++){double x=double(k)/bins;if(x>=a.progress&&x<=b.progress)crossings[k]=a.ms+(b.ms-a.ms)*(x-a.progress)/(b.progress-a.progress);}
 }
 for(int k=0;k<bins;k++)if(std::isfinite(crossings[k])&&std::isfinite(crossings[k+1]))result[k]=crossings[k+1]-crossings[k];
 return result;
}
inline bool valid(const Physics& p,const Graphics& g){return g.status==2&&g.laps>=0&&g.currentMs>=0&&g.currentMs<INT32_MAX&&std::isfinite(g.spline)&&g.spline>=0&&g.spline<=1&&std::isfinite(p.gas)&&p.gas>=0&&p.gas<=1&&std::isfinite(p.brake)&&p.brake>=0&&p.brake<=1&&std::isfinite(p.speed)&&p.speed>=0&&std::isfinite(p.steer);}
}

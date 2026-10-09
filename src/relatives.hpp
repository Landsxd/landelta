#pragma once
#include "telemetry.hpp"
namespace acc {
struct RelativeRow {Car car;double distance=0;bool stale=false;};
struct RelativeRows {std::vector<RelativeRow> ahead,behind;};
inline RelativeRows relativeRows(const State& state,const Car& player,uint64_t now,bool paused=false){
 RelativeRows rows;
 for(const auto& kv:state.cars){const auto& c=kv.second;if(c.id==player.id||c.location!=1||!c.seen||(!paused&&!fresh(c.seen,now,8000)))continue;
  double d=double(c.spline)-player.spline;while(d>.5)d-=1;while(d<-.5)d+=1;
  RelativeRow row{c,d,!paused&&!fresh(c.seen,now)};
  (d>0?rows.ahead:rows.behind).push_back(row);
 }
 auto order=[](const RelativeRow& a,const RelativeRow& b){if(a.stale!=b.stale)return !a.stale;double da=std::abs(a.distance),db=std::abs(b.distance);return da==db?a.car.id<b.car.id:da<db;};
 std::sort(rows.ahead.begin(),rows.ahead.end(),order);std::sort(rows.behind.begin(),rows.behind.end(),order);return rows;
}
}

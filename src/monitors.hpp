#pragma once
#include "race_studio.hpp"
namespace race {
struct Monitor {std::wstring device;int left=0,top=0,width=1920,height=1080;bool primary=false;};
inline int monitorFor(const std::vector<Monitor>& monitors,int x,int y,int w,int h){
 int selected=0;double best=-1;
 for(size_t i=0;i<monitors.size();++i){const auto& m=monitors[i];double area=double(std::max(0,std::min(x+w,m.left+m.width)-std::max(x,m.left)))*std::max(0,std::min(y+h,m.top+m.height)-std::max(y,m.top));
  if(area>best){best=area;selected=int(i);}}
 if(best>0)return selected;
 double distance=INFINITY;for(size_t i=0;i<monitors.size();++i){const auto& m=monitors[i];double dx=x-std::clamp(x,m.left,m.left+m.width),dy=y-std::clamp(y,m.top,m.top+m.height),d=dx*dx+dy*dy;if(d<distance){distance=d;selected=int(i);}}
 return selected;
}
inline std::pair<int,int> contain(const Monitor& m,int x,int y,int w,int h){return {std::clamp(x,m.left,m.left+std::max(0,m.width-w)),std::clamp(y,m.top,m.top+std::max(0,m.height-h))};}
}

#pragma once
#include <algorithm>

namespace acc {
// One transform for painting and hit testing; keep fonts and icons proportional.
struct ControlLayout {
 static constexpr int width=656,height=680,minWidth=492,minHeight=510;
 float scale=0,x=0,y=0;
 ControlLayout(int clientWidth,int clientHeight){
  if(clientWidth<=0||clientHeight<=0)return;
  scale=std::min(clientWidth/float(width),clientHeight/float(height));
  x=(clientWidth-width*scale)/2;y=(clientHeight-height*scale)/2;
 }
 bool point(float screenX,float screenY,float& logicalX,float& logicalY)const{
  if(scale<=0||screenX<x||screenY<y||screenX>=x+width*scale||screenY>=y+height*scale)return false;
  logicalX=(screenX-x)/scale;logicalY=(screenY-y)/scale;
  return logicalX>=0&&logicalY>=0&&logicalX<width&&logicalY<height;
 }
};
}

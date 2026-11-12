#include "car_brand.hpp"
#include <memory>
std::map<std::string,std::unique_ptr<Bitmap>> brandImages;
void brandLogo(Graphics& g,int model,float x,float y){
 std::string brand=acc::carBrand(model);if(brand.empty())return;
 auto it=brandImages.find(brand);if(it==brandImages.end()){
  wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);
  auto path=fs::path(exe).parent_path()/L"assets"/L"brands"/(wide(brand)+L".png");
  auto logo=std::make_unique<Bitmap>(path.c_str());if(logo->GetLastStatus()!=Ok)logo.reset();
  if(logo){
   int left=logo->GetWidth(),top=logo->GetHeight(),right=-1,bottom=-1;
   for(int py=0;py<int(logo->GetHeight());py++)for(int px=0;px<int(logo->GetWidth());px++){Color pixel;logo->GetPixel(px,py,&pixel);if(pixel.GetA()>24){left=std::min(left,px);top=std::min(top,py);right=std::max(right,px);bottom=std::max(bottom,py);}}
   // Pinned McLaren artwork: use its speedmark instead of unreadably small lettering.
   if(brand=="mclaren"){left=int(logo->GetWidth()*330/455);top=int(logo->GetHeight()*96/256);right=int(logo->GetWidth()*410/455)-1;bottom=int(logo->GetHeight()*151/256)-1;}
   if(right>=left&&bottom>=top){std::unique_ptr<Bitmap> cropped(logo->Clone(Rect(left,top,right-left+1,bottom-top+1),PixelFormat32bppARGB));if(cropped&&cropped->GetLastStatus()==Ok)logo=std::move(cropped);}
  }
  it=brandImages.emplace(brand,std::move(logo)).first;
 }
 if(!it->second)return;auto& logo=*it->second;
 // Neutral backing keeps dark manufacturer marks legible at low overlay opacity.
 SolidBrush plate(Color(255,235,239,242));g.FillRectangle(&plate,x,y,30.f,30.f);
 float factor=26.f/std::max(logo.GetWidth(),logo.GetHeight()),w=logo.GetWidth()*factor,h=logo.GetHeight()*factor;
 auto previous=g.GetInterpolationMode();g.SetInterpolationMode(InterpolationModeHighQualityBicubic);g.DrawImage(&logo,RectF(x+(30-w)/2,y+(30-h)/2,w,h));g.SetInterpolationMode(previous);
}
void positionText(Graphics& g,int position,float y,Color color){
 const auto value=position>0?std::to_wstring(position):L"—";FontFamily family(L"Segoe UI");
 StringFormat format;format.SetFormatFlags(StringFormatFlagsNoWrap);format.SetTrimming(StringTrimmingNone);format.SetAlignment(StringAlignmentCenter);format.SetLineAlignment(StringAlignmentCenter);
 float size=24;RectF measured;for(;size>10;size-=1){Font font(&family,size,FontStyleBold,UnitPixel);g.MeasureString(value.c_str(),int(value.size()),&font,PointF(0,0),&format,&measured);if(measured.Width<=52)break;}
 Font font(&family,size,FontStyleBold,UnitPixel);SolidBrush shadow(Color(230,0,0,0)),brush(color);
 if(renderingOverlay)g.DrawString(value.c_str(),int(value.size()),&font,RectF(5,y+1,54,38),&format,&shadow);
 g.DrawString(value.c_str(),int(value.size()),&font,RectF(4,y,54,38),&format,&brush);
}

#pragma once
#include <string>
#include <stdexcept>
#include <cstdint>
namespace acc {
// JSON itself is UTF-8. ACC/Windows tools can store it as UTF-16, with or
// without a BOM. Never erase interleaved zero bytes: that corrupts Unicode.
inline std::string decodeConfigBytes(const std::string& bytes){
 size_t start=0;int encoding=0;auto u=[&](size_t i){return static_cast<unsigned char>(bytes[i]);};
 if(bytes.size()>=4&&((u(0)==255&&u(1)==254&&u(2)==0&&u(3)==0)||(u(0)==0&&u(1)==0&&u(2)==254&&u(3)==255)))throw std::runtime_error("UTF-32 no compatible");
 if(bytes.size()>=2&&u(0)==255&&u(1)==254){encoding=1;start=2;}
 else if(bytes.size()>=2&&u(0)==254&&u(1)==255){encoding=2;start=2;}
 else if(bytes.size()>=2&&u(0)!=0&&u(1)==0)encoding=1;
 else if(bytes.size()>=2&&u(0)==0&&u(1)!=0)encoding=2;
 if(!encoding){std::string s=bytes;if(s.size()>=3&&u(0)==239&&u(1)==187&&u(2)==191)s.erase(0,3);while(!s.empty()&&s.back()==0)s.pop_back();return s;}
 if((bytes.size()-start)%2)throw std::runtime_error("UTF-16 incompleto; se reintentará la lectura");
 auto unit=[&](size_t i)->uint32_t{return encoding==1?u(i)|(uint32_t(u(i+1))<<8):(uint32_t(u(i))<<8)|u(i+1);};
 std::string result;
 for(size_t i=start;i<bytes.size();i+=2){uint32_t cp=unit(i);
  if(cp>=0xd800&&cp<=0xdbff){if(i+3>=bytes.size())throw std::runtime_error("UTF-16 incompleto");uint32_t low=unit(i+2);if(low<0xdc00||low>0xdfff)throw std::runtime_error("UTF-16 inválido");cp=0x10000+((cp-0xd800)<<10)+(low-0xdc00);i+=2;}
  else if(cp>=0xdc00&&cp<=0xdfff)throw std::runtime_error("UTF-16 inválido");
  if(cp<0x80)result.push_back(char(cp));
  else if(cp<0x800){result.push_back(char(0xc0|(cp>>6)));result.push_back(char(0x80|(cp&63)));}
  else if(cp<0x10000){result.push_back(char(0xe0|(cp>>12)));result.push_back(char(0x80|((cp>>6)&63)));result.push_back(char(0x80|(cp&63)));}
  else{result.push_back(char(0xf0|(cp>>18)));result.push_back(char(0x80|((cp>>12)&63)));result.push_back(char(0x80|((cp>>6)&63)));result.push_back(char(0x80|(cp&63)));}
 }
 while(!result.empty()&&result.back()==0)result.pop_back();
 return result;
}
inline bool retainConfigOnReadError(bool running,bool ready,bool samePath){return running&&ready&&samePath;}
}

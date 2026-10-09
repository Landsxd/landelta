#include "../src/config_codec.hpp"
#include <nlohmann/json.hpp>
#include <cassert>
#include <iostream>
using nlohmann::json;
std::string utf16(const std::u16string& s,bool be,bool bom){std::string result=bom?(be?"\xfe\xff":"\xff\xfe"):"";for(auto c:s){result.push_back(char(be?c>>8:c&255));result.push_back(char(be?c&255:c>>8));}return result;}
int main(){
 std::string expected=u8"{\"updListenerPort\":9000,\"connectionPassword\":\"áñ水🏁\",\"commandPassword\":\"\"}";
 std::u16string original=u"{\"updListenerPort\":9000,\"connectionPassword\":\"áñ水🏁\",\"commandPassword\":\"\"}";
 auto verify=[&](const std::string& bytes){auto decoded=acc::decodeConfigBytes(bytes);assert(json::parse(decoded)==json::parse(expected));assert(json::parse(decoded)["connectionPassword"]==u8"áñ水🏁");};
 verify(expected);verify("\xef\xbb\xbf"+expected);verify(expected+std::string(1,'\0'));
 for(bool be:{false,true})for(bool bom:{false,true}){verify(utf16(original,be,bom));verify(utf16(u"\r\n "+original,be,bom));verify(utf16(original+std::u16string(1,0),be,bom));}
 // Reproduce the former failure: UTF-16 without BOM reaches the JSON parser
 // unchanged and fails at its second byte. The production decoder now reads it.
 auto noBom=utf16(original,false,false);bool reproduced=false;try{json::parse(noBom).dump();}catch(const json::parse_error& e){reproduced=e.byte==2;}assert(reproduced);verify(noBom);
 auto odd=noBom;odd.pop_back();try{acc::decodeConfigBytes(odd);assert(false);}catch(const std::runtime_error&){}
 for(auto broken:{std::u16string{char16_t(0xd800)},std::u16string{char16_t(0xdc00)},std::u16string{char16_t(0xd800),u'x'}}){try{acc::decodeConfigBytes(utf16(broken,false,true));assert(false);}catch(const std::runtime_error&){}}
 auto truncated=utf16(u"{",false,false);try{json::parse(acc::decodeConfigBytes(truncated)).dump();assert(false);}catch(const json::parse_error&){}
 assert(acc::retainConfigOnReadError(true,true,true));assert(!acc::retainConfigOnReadError(false,true,true));assert(!acc::retainConfigOnReadError(true,false,true));assert(!acc::retainConfigOnReadError(true,true,false));
 std::cout<<"PASS: column-2 regression; UTF-8/BOM; UTF-16 LE/BE with/without BOM; leading whitespace; trailing terminator; Unicode password preserved; truncation/surrogates rejected; reload policy\n";
}

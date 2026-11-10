#include "iracing.hpp"
#include <cassert>
#include <iostream>
int main(){std::vector<uint8_t> b(2048);iracing::Header h{};h.version=2;h.status=1;h.hz=60;h.numVars=2;h.varOffset=112;h.numBuffers=1;h.bufferLen=64;h.buffers[0].offset=1024;h.buffers[0].tick=22;h.sessionOffset=512;std::string yaml="DriverInfo:\n Drivers:\n - CarIdx: 12\n   UserName: \"L. Landeros\"\n   CarScreenName: McLaren\nCameraInfo:\n";h.sessionLen=int(yaml.size());memcpy(b.data(),&h,sizeof h);memcpy(b.data()+512,yaml.data(),yaml.size());iracing::Variable v{};v.type=4;v.count=1;strcpy(v.name,"Throttle");memcpy(b.data()+112,&v,sizeof v);v.type=2;v.offset=4;strcpy(v.name,"Lap");memcpy(b.data()+256,&v,sizeof v);float throttle=.7f;int lap=12;memcpy(b.data()+1024,&throttle,4);memcpy(b.data()+1028,&lap,4);
 iracing::Packet p;assert(p.read(b.data(),b.size()));assert(p.tick==22&&std::abs(p.get("Throttle")-.7)<.001&&p.get("Lap")==12);assert(std::isnan(p.get("Unknown"))&&std::isnan(p.get("Throttle",7)));assert(iracing::drivers(p.yaml)[12]["UserName"]=="L. Landeros");
 h.buffers[0].offset=2040;memcpy(b.data(),&h,sizeof h);assert(!p.read(b.data(),b.size()));h.buffers[0].offset=1024;h.numVars=INT32_MAX;memcpy(b.data(),&h,sizeof h);assert(!p.read(b.data(),b.size()));h.numVars=2;h.sessionLen=-1;memcpy(b.data(),&h,sizeof h);assert(!p.read(b.data(),b.size()));assert(!p.read(b.data(),30));
 assert(iracing::milliseconds(94.123)==94123&&iracing::milliseconds(NAN)==0);std::cout<<"PASS iRacing SDK: checked wire layout, scalars, missing data, driver metadata, truncated/malformed buffers\n";
}

#pragma once
#include "race_studio.hpp"
namespace race {
inline uint32_t crc32(const std::string& s){uint32_t c=~0u;for(unsigned char v:s){c^=v;for(int i=0;i<8;++i)c=(c>>1)^((c&1)?0xedb88320u:0);}return ~c;}
inline void exportZIP(const Session& s,const std::filesystem::path& output){
 std::string raw;if(!s.path.empty()){std::ifstream f(s.path,std::ios::binary);if(!f)throw std::runtime_error("No se pudo leer la sesión");auto size=std::filesystem::file_size(s.path);if(size>512*1024*1024)throw std::runtime_error("Sesión demasiado grande para exportar");raw.assign(std::istreambuf_iterator<char>(f),{});}
 std::ostringstream laps;laps.imbue(std::locale::classic());laps<<"lap,time_ms,complete,partial,pit,valid,s1_ms,s2_ms,s3_ms";for(int i=1;i<=10;++i)laps<<",micro"<<i<<"_ms";laps<<'\n';
 for(auto& l:s.laps){laps<<l.number<<','<<l.ms<<','<<l.complete<<','<<l.partial<<','<<l.pit<<','<<l.valid;for(auto v:l.sectors)laps<<','<<v;for(auto v:l.micro){laps<<',';if(std::isfinite(v))laps<<v;}laps<<'\n';}
 nlohmann::json meta={{"schema",1},{"app","LANDELTA 0.14.2"},{"game",s.game},{"track",s.track},{"variant",s.variant},{"car",s.car},{"started_utc_ms",s.started},{"steering_unit",s.steerUnit},{"speed_unit","km/h"},{"pedals","normalized input 0..1, not physical force"},{"validity","1 valid, 0 invalid, -1 unknown"},{"microsectors","10 equal distance segments, interpolated; missing boundaries remain empty"},{"samples_loaded",s.samples.size()},{"gaps_over_500ms",s.gaps},{"ambient_temperature",nullptr},{"track_temperature",nullptr},{"car_version",nullptr},{"capture_note",s.game=="AC"?"Original AC CSV; live preview is downsampled":s.game=="iRacing"?"Independent SDK-event recorder; actual cadence and missed ticks are in samples.csv":"UI recorder at up to 10 Hz of new observed packets, not the full SDK tick rate"}};
 struct Entry{std::string name,body;uint32_t crc=0,offset=0;};std::vector<Entry> files={{"session.json",meta.dump(2)},{"laps.csv",laps.str()}};if(!raw.empty())files.push_back({"samples.csv",std::move(raw)});
 auto temp=output;temp+=".landelta-tmp";std::ofstream f(temp,std::ios::binary|std::ios::trunc);if(!f)throw std::runtime_error("No se pudo crear el ZIP");
 auto u16=[&](uint16_t v){f.put(char(v));f.put(char(v>>8));};auto u32=[&](uint32_t v){u16(v&65535);u16(v>>16);};
 for(auto& e:files){e.offset=uint32_t(f.tellp());e.crc=crc32(e.body);u32(0x04034b50);u16(20);u16(0);u16(0);u16(0);u16(33);u32(e.crc);u32(uint32_t(e.body.size()));u32(uint32_t(e.body.size()));u16(uint16_t(e.name.size()));u16(0);f<<e.name<<e.body;}
 uint32_t central=uint32_t(f.tellp());for(auto& e:files){u32(0x02014b50);u16(20);u16(20);u16(0);u16(0);u16(0);u16(33);u32(e.crc);u32(uint32_t(e.body.size()));u32(uint32_t(e.body.size()));u16(uint16_t(e.name.size()));u16(0);u16(0);u16(0);u16(0);u32(0);u32(e.offset);f<<e.name;}
 uint32_t length=uint32_t(f.tellp())-central;u32(0x06054b50);u16(0);u16(0);u16(uint16_t(files.size()));u16(uint16_t(files.size()));u32(length);u32(central);u16(0);f.close();if(!f)throw std::runtime_error("No se pudo completar el ZIP");
#ifdef _WIN32
 if(!MoveFileExW(temp.c_str(),output.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("No se pudo guardar el ZIP");
#else
 std::filesystem::rename(temp,output);
#endif
}
}

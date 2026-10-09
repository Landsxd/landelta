#pragma once
#include "analytics.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#ifdef _WIN32
#include <windows.h>
#endif

namespace acc {
using HistoryJson=nlohmann::json;
inline HistoryJson encodeHistory(const SessionRecord& r){
 HistoryJson j={{"schema",1},{"id",r.id},{"started",r.started},{"track",r.track},{"trackId",r.trackId},{"type",r.type},{"driver",r.driver},{"team",r.team},{"model",r.model},{"number",r.number},{"laps",HistoryJson::array()}};
 for(const auto& l:r.laps)j["laps"].push_back({{"number",l.number},{"ms",l.timing.ms},{"splits",l.timing.splits},{"valid",l.timing.valid},{"invalid",l.timing.invalid},{"in",l.timing.in},{"out",l.timing.out},{"complete",l.complete},{"pit",l.pit},{"driver",l.driver}});return j;
}
inline bool validHistoryId(const std::string& s){return !s.empty()&&s.size()<96&&s.find_first_not_of("0123456789-")==std::string::npos;}
inline SessionRecord decodeHistory(const HistoryJson& j){
 if(j.at("schema").get<int>()!=1)throw std::runtime_error("Versión de historial no compatible");
 SessionRecord r;r.id=j.at("id").get<std::string>();r.started=j.at("started").get<int64_t>();r.track=j.at("track").get<std::string>();r.trackId=j.at("trackId").get<int>();r.type=j.at("type").get<int>();r.driver=j.at("driver").get<std::string>();r.team=j.at("team").get<std::string>();r.model=j.at("model").get<int>();r.number=j.at("number").get<int>();
 if(!validHistoryId(r.id)||r.track.empty()||r.track.size()>1024||r.driver.size()>4096||r.started<0||!j.at("laps").is_array()||j.at("laps").size()>100000)throw std::runtime_error("Historial inválido");
 int previous=-1;for(const auto& a:j.at("laps")){RecordedLap l;l.number=a.at("number").get<int>();l.timing.ms=a.at("ms").get<int>();l.timing.splits=a.at("splits").get<std::array<int,3>>();l.timing.valid=a.at("valid").get<bool>();l.timing.invalid=a.at("invalid").get<bool>();l.timing.in=a.at("in").get<bool>();l.timing.out=a.at("out").get<bool>();l.complete=a.at("complete").get<bool>();l.pit=a.at("pit").get<bool>();l.driver=a.at("driver").get<std::string>();if(l.number<=previous||l.number>65535||!timeValue(l.timing.ms)||l.driver.size()>4096)throw std::runtime_error("Vuelta inválida en historial");for(int v:l.timing.splits)if(v<0||v==INT32_MAX)throw std::runtime_error("Sector inválido");previous=l.number;r.laps.push_back(l);}return r;
}
struct HistoryStore {
 std::filesystem::path directory;std::string error;int skipped=0;
 void load(Analytics& a){
  error.clear();skipped=0;try{std::filesystem::create_directories(directory);
   for(const auto& p:std::filesystem::directory_iterator(directory)){if(p.path().extension()!=".json")continue;try{if(p.file_size()>32*1024*1024)throw std::runtime_error("Archivo demasiado grande");std::ifstream f(p.path());if(!f)throw std::runtime_error("Lectura fallida");HistoryJson j;f>>j;auto r=decodeHistory(j);if(p.path().stem().string()!=r.id)throw std::runtime_error("Nombre de archivo inválido");a.history.push_back(std::move(r));}catch(...){++skipped;}}
   std::sort(a.history.begin(),a.history.end(),[](const SessionRecord& x,const SessionRecord& y){return x.started<y.started;});
   if(skipped)error="No se pudieron leer "+std::to_string(skipped)+" sesiones; los archivos se conservaron.";
  }catch(...){error="No se pudo abrir la carpeta de historial.";}
 }
 bool save(const SessionRecord& r){
  try{if(!validHistoryId(r.id))throw std::runtime_error("ID inválido");std::filesystem::create_directories(directory);auto path=directory/(r.id+".json"),tmp=directory/(r.id+".tmp");
   {std::ofstream f(tmp,std::ios::binary|std::ios::trunc);if(!f)throw std::runtime_error("Escritura fallida");f<<encodeHistory(r).dump(2);f.flush();f.close();if(f.fail())throw std::runtime_error("Escritura incompleta");}
#ifdef _WIN32
   if(!MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Reemplazo fallido");
#else
   std::filesystem::rename(tmp,path);
#endif
   return true;
  }catch(...){return false;}
 }
 void flush(Analytics& a){bool failed=false;for(auto it=a.dirty.begin();it!=a.dirty.end();){if(*it<a.history.size()&&save(a.history[*it]))it=a.dirty.erase(it);else{failed=true;++it;}}
  error=failed?"No se pudo guardar. Se reintentará; revisa espacio y permisos.":(skipped?"Hay "+std::to_string(skipped)+" sesiones ilegibles; sus archivos se conservaron.":"");
 }
};
}

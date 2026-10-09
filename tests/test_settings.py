"""Exercise the production settings migration and persistence for all seven panels."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/main.cpp').read_text()
source='''#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <nlohmann/json.hpp>
namespace fs=std::filesystem;using json=nlohmann::json;
std::wstring wide(const std::string&s){return {s.begin(),s.end()};}
std::string utf8(const std::wstring&s){return {s.begin(),s.end()};}
bool fileWrite(const fs::path&p,const std::string&s){std::ofstream f(p);f<<s;return bool(f);}
std::string fileRead(const fs::path&p){std::ifstream f(p);return {std::istreambuf_iterator<char>(f),{}};}
'''+s[s.index('constexpr int PanelCount'):s.index('Config cfg;')]+'''Config cfg;fs::path configPath,settingsPath;
'''+s[s.index('void saveSettings'):s.index('void closeInputs')]+r'''
int main(int argc,char**argv){settingsPath=fs::path(argv[1])/"settings.json";
 json old={{"schema",3},{"scale",1.1},{"backgroundOpacity",32},{"rows",12},{"panels",json::array({{{"x",25},{"y",90},{"visible",false}},{{"x",850},{"y",90},{"visible",true}},{{"x",850},{"y",700},{"visible",true}}})}};
 fileWrite(settingsPath,old.dump());loadSettings();assert(cfg.autoShow&&cfg.deltaScale==1&&!cfg.keepAfterRace);assert(cfg.rows==8&&cfg.x[1]==650&&cfg.y[2]==470&&!cfg.visible[0]);assert(cfg.opacity==32&&cfg.visible[3]&&cfg.visible[4]&&cfg.visible[5]&&!cfg.visible[6]);
 old["panels"][1]["x"]=123;old["panels"][1]["y"]=456;fileWrite(settingsPath,old.dump());loadSettings();assert(cfg.x[1]==123&&cfg.y[1]==456);
 for(int i=0;i<PanelCount;i++){cfg.x[i]=i*70;cfg.y[i]=i*80;cfg.visible[i]=i%2;}cfg.autoShow=false;cfg.deltaScale=1.8f;cfg.keepAfterRace=true;saveSettings();cfg={};loadSettings();assert(!cfg.autoShow&&cfg.keepAfterRace&&cfg.deltaScale==1.8f);for(int i=0;i<PanelCount;i++){assert(cfg.x[i]==i*70&&cfg.y[i]==i*80&&cfg.visible[i]==bool(i%2));}
 assert(json::parse(fileRead(settingsPath))["panels"].size()==7);
 std::cout<<"PASS: 3-to-7 panel migration, default positions, custom positions, visibility, opacity, persistence\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(source)
 subprocess.run(['g++','-std=c++17','-I',str(root/'vendor'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test'),tmp],check=True)

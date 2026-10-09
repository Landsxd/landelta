"""Run production shortcut writing with COM/MoveFile fakes and real temp files.
This tests command arguments and failure handling, not the Windows logon shell.
"""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
shim=r'''
#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <cwchar>
using HRESULT=long;using DWORD=unsigned long;
#define SUCCEEDED(x) ((x)>=0)
#define FAILED(x) ((x)<0)
const int CLSID_ShellLink=1,CLSCTX_INPROC_SERVER=1,IID_IShellLinkW=2,IID_IPersistFile=3,STGM_READ=0,SLGP_RAWPATH=4,SW_SHOWNORMAL=1,TRUE=1,MOVEFILE_REPLACE_EXISTING=1,MOVEFILE_WRITE_THROUGH=2;
int failAt=0,step=0,released=0;bool failMove=false;
std::string savedTarget,savedArgs,savedWorking;
long checked(){return ++step==failAt?-1:0;}
struct IPersistFile{
 long Load(const char* path,int){std::ifstream f(path);std::getline(f,savedTarget);std::getline(f,savedArgs);return f?0:-1;}
 long Save(const char* path,int){auto hr=checked();if(hr<0)return hr;std::ofstream f(path);f<<savedTarget<<'\n'<<savedArgs<<'\n';return f?0:-1;}
 void Release(){released++;delete this;}
};
struct IShellLinkW{
 long QueryInterface(int,void** out){auto hr=checked();if(hr>=0)*out=new IPersistFile;return hr;}
 long SetPath(const char* s){savedTarget=s;return checked();}
 long SetArguments(const wchar_t* s){std::wstring w=s;savedArgs={w.begin(),w.end()};return checked();}
 long SetWorkingDirectory(const char* s){savedWorking=s;return checked();}
 long SetDescription(const wchar_t*){return checked();}
 long SetIconLocation(const char*,int){return checked();}
 long SetShowCmd(int){return checked();}
 long GetPath(wchar_t* out,int size,void*,int){std::wstring w(savedTarget.begin(),savedTarget.end());wcsncpy(out,w.c_str(),size);return 0;}
 long GetArguments(wchar_t* out,int size){std::wstring w(savedArgs.begin(),savedArgs.end());wcsncpy(out,w.c_str(),size);return 0;}
 void Release(){released++;delete this;}
};
long CoCreateInstance(int,void*,int,int,void** out){auto hr=checked();if(hr>=0)*out=new IShellLinkW;return hr;}
bool MoveFileExW(const char* from,const char* to,int){if(failMove)return false;std::error_code ec;std::filesystem::rename(from,to,ec);return !ec;}
'''
cases=r'''
#include "startup.hpp"
#include <cassert>
#include <iostream>
std::string readFile(const std::filesystem::path& p){std::ifstream f(p);return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char** argv){namespace fs=std::filesystem;fs::path base=argv[1],exe=base/"folder with spaces"/"VortexACC.exe",link=base/"Startup"/"VortexACC-Auto.lnk";fs::create_directories(exe.parent_path());std::ofstream(exe)<<"test";std::wstring error;
 assert(acc::writeStartup(link,exe,true,error));assert(error.empty()&&savedArgs=="--background"&&savedTarget==exe.string()&&savedWorking==exe.parent_path().string());auto original=readFile(link);assert(acc::inspectStartup(link,exe).current);
 auto other=base/"New.exe";std::ofstream(other)<<"test";assert(acc::inspectStartup(link,other).exists&&!acc::inspectStartup(link,other).current);
 for(int fail=1;fail<=9;fail++){step=0;failAt=fail;assert(!acc::writeStartup(link,other,true,error));assert(!error.empty()&&readFile(link)==original);assert(!fs::exists(fs::path(link.string()+".vortex.tmp")));}
 failAt=0;failMove=true;assert(!acc::writeStartup(link,other,true,error));assert(readFile(link)==original);failMove=false;step=0;assert(acc::writeStartup(link,other,true,error));assert(acc::inspectStartup(link,other).current);
 assert(acc::writeStartup(link,other,false,error));assert(!fs::exists(link));assert(acc::writeStartup(link,other,false,error));
 fs::create_directory(link);assert(!acc::writeStartup(link,other,false,error));assert(fs::is_directory(link));
 assert(!acc::writeStartup({},exe,true,error));assert(!acc::writeStartup(base/"bad.lnk",base/"missing.exe",true,error));
 std::cout<<"PASS: shortcut target/working directory with spaces, background args, activate/deactivate/update, COM/save/rename failures preserve previous shortcut\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'shobjidl.h').write_text(shim);(p/'test.cpp').write_text(cases)
 subprocess.run(['g++','-std=c++17','-I',tmp,'-I',str(root/'src'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test'),tmp],check=True)

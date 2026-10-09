#pragma once
#include <shobjidl.h>
#include <filesystem>
#include <string>
namespace acc {
namespace startupfs=std::filesystem;
template<class T> struct ComOwner {T* p=nullptr;~ComOwner(){if(p)p->Release();}ComOwner()=default;ComOwner(const ComOwner&)=delete;ComOwner& operator=(const ComOwner&)=delete;};
struct StartupInfo {bool exists=false,current=false;std::wstring error;startupfs::path target;};
inline StartupInfo inspectStartup(const startupfs::path& linkPath,const startupfs::path& exe){
 StartupInfo info;std::error_code ec;info.exists=startupfs::is_regular_file(linkPath,ec);if(!info.exists)return info;
 ComOwner<IShellLinkW> link;ComOwner<IPersistFile> file;
 HRESULT hr=CoCreateInstance(CLSID_ShellLink,nullptr,CLSCTX_INPROC_SERVER,IID_IShellLinkW,reinterpret_cast<void**>(&link.p));
 if(SUCCEEDED(hr))hr=link.p->QueryInterface(IID_IPersistFile,reinterpret_cast<void**>(&file.p));
 if(SUCCEEDED(hr))hr=file.p->Load(linkPath.c_str(),STGM_READ);
 wchar_t target[32768]{},args[128]{};
 if(SUCCEEDED(hr))hr=link.p->GetPath(target,32768,nullptr,SLGP_RAWPATH);
 if(SUCCEEDED(hr))hr=link.p->GetArguments(args,128);
 if(FAILED(hr)){info.error=L"No se pudo leer el acceso de inicio.";return info;}
 info.target=target;info.current=startupfs::equivalent(info.target,exe,ec)&&std::wstring(args)==L"--background";return info;
}
inline bool writeStartup(const startupfs::path& linkPath,const startupfs::path& exe,bool enable,std::wstring& error){
 error.clear();std::error_code ec;
 if(linkPath.empty()||exe.empty()){error=L"No se encontró la carpeta de inicio o el ejecutable.";return false;}
 if(!enable){if(startupfs::exists(linkPath,ec)&&!startupfs::is_regular_file(linkPath,ec)){error=L"La ruta de inicio está ocupada por una carpeta.";return false;}startupfs::remove(linkPath,ec);if(ec){error=L"No se pudo desactivar el inicio con Windows.";return false;}return true;}
 if(!startupfs::is_regular_file(exe,ec)){error=L"El ejecutable no está disponible en esta ubicación.";return false;}
 startupfs::create_directories(linkPath.parent_path(),ec);if(ec){error=L"No se pudo acceder a la carpeta de inicio.";return false;}
 ComOwner<IShellLinkW> link;ComOwner<IPersistFile> file;
 HRESULT hr=CoCreateInstance(CLSID_ShellLink,nullptr,CLSCTX_INPROC_SERVER,IID_IShellLinkW,reinterpret_cast<void**>(&link.p));
 if(SUCCEEDED(hr))hr=link.p->SetPath(exe.c_str());
 if(SUCCEEDED(hr))hr=link.p->SetArguments(L"--background");
 if(SUCCEEDED(hr))hr=link.p->SetWorkingDirectory(exe.parent_path().c_str());
 if(SUCCEEDED(hr))hr=link.p->SetDescription(L"LANDELTA: esperar al juego en segundo plano");
 if(SUCCEEDED(hr))hr=link.p->SetIconLocation(exe.c_str(),0);
 // --background controls visibility. SW_HIDE is not consistently honored by Shell.
 if(SUCCEEDED(hr))hr=link.p->SetShowCmd(SW_SHOWNORMAL);
 if(SUCCEEDED(hr))hr=link.p->QueryInterface(IID_IPersistFile,reinterpret_cast<void**>(&file.p));
 auto tmp=linkPath;tmp+=L".vortex.tmp";
 if(SUCCEEDED(hr))hr=file.p->Save(tmp.c_str(),TRUE);
 if(FAILED(hr)){startupfs::remove(tmp,ec);error=L"No se pudo crear el acceso de inicio ("+std::to_wstring(hr)+L").";return false;}
 if(!MoveFileExW(tmp.c_str(),linkPath.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){startupfs::remove(tmp,ec);error=L"No se pudo guardar el acceso de inicio.";return false;}
 return true;
}
}

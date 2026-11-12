#pragma once
#include "iracing.hpp"
#include <thread>
#include <mutex>
namespace iracing {
// Native telemetry has its own worker. Resizing/opening dialogs cannot stop CSV capture.
struct Snapshot {
 acc::State state;race::Sample sample;Quality quality;
 bool connected=false,onTrack=false;uint64_t lastNew=0;unsigned generation=0;
 std::string identity,car,error;std::filesystem::path path;
};
struct Feed {
 std::thread worker;std::atomic<bool> running{false};std::mutex mutex;Snapshot snapshot;
 Snapshot read(){std::lock_guard<std::mutex> lock(mutex);return snapshot;}
 void stop(){running=false;if(worker.joinable())worker.join();}
 ~Feed(){stop();}
 void start(std::filesystem::path folder,std::wstring mapping=L"Local\\IRSDKMemMapFileName",std::wstring event=L"Local\\IRSDKDataValidEvent"){if(running)return;stop();running=true;worker=std::thread([this,folder,mapping,event]{
  Client client;client.mappingName=mapping;race::Recorder recorder;recorder.retainSamples=false;HANDLE ready=nullptr;uint64_t published=0;race::Sample previous;bool havePrevious=false;
  try{while(running){uint64_t now=GetTickCount64();bool fresh=client.poll(now);
    if(fresh&&client.onTrack){auto& p=client.sample;
     auto identity=client.identity+"/"+std::to_string(client.generation);
     if(havePrevious&&recorder.key==identity&&race::restarted(previous,p))recorder.close();
     auto wall=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
     recorder.add(p,identity,"iRacing",client.state.track,client.car,folder,wall);
     recorder.session.length=client.state.meters;previous=p;havePrevious=true;
    }else if(!client.onTrack&&recorder.online)recorder.pause();
    if(!client.connected){recorder.close();havePrevious=false;if(ready){CloseHandle(ready);ready=nullptr;}}
    if(now-published>=50){Snapshot next;next.state=client.state;next.sample=client.sample;next.quality=client.quality;next.connected=client.connected;next.onTrack=client.onTrack;next.lastNew=client.lastNew;next.generation=client.generation;next.identity=client.identity;next.car=client.car;next.path=recorder.session.path;next.error=recorder.error.empty()?client.error:recorder.error;
     {std::lock_guard<std::mutex> lock(mutex);snapshot=std::move(next);}published=now;}
    if(client.connected&&!ready)ready=OpenEventW(SYNCHRONIZE,FALSE,event.c_str());
    if(ready)WaitForSingleObject(ready,20);else std::this_thread::sleep_for(std::chrono::milliseconds(client.connected?4:100));
   }recorder.close();
  }catch(...){std::lock_guard<std::mutex> lock(mutex);snapshot.error="No se pudo continuar la captura iRacing; revisa espacio y permisos";}
  if(ready)CloseHandle(ready);{std::lock_guard<std::mutex> lock(mutex);snapshot.connected=snapshot.onTrack=false;}running=false;
 });}
};
}

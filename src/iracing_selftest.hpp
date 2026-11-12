// Isolated synthetic SDK mapping. Never opens or modifies the actual sim mapping.
bool iracingFeedSelfTest(){
 auto token=std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());
 auto name=L"Local\\LANDELTA-Test-"+token,event=name+L"-event";
 HANDLE mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,16384,name.c_str());if(!mapping)return false;
 auto bytes=static_cast<uint8_t*>(MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,16384));if(!bytes){CloseHandle(mapping);return false;}
 HANDLE ready=CreateEventW(nullptr,FALSE,FALSE,event.c_str());if(!ready){UnmapViewOfFile(bytes);CloseHandle(mapping);return false;}
 const char* names[]={"SessionNum","Lap","LapCurrentLapTime","LapLastLapTime","LapDistPct","LapBestLapTime","LapDeltaToSessionBestLap_OK","LapDeltaToSessionBestLap","IsOnTrack","Throttle","Brake","Speed","SteeringWheelAngle"};
 iracing::Header h{};h.version=2;h.status=1;h.hz=60;h.numVars=13;h.varOffset=112;h.numBuffers=2;h.bufferLen=256;h.buffers[0].offset=8192;h.buffers[1].offset=8448;h.buffers[0].tick=h.buffers[1].tick=-1;
 std::string yaml="WeekendInfo:\n TrackName: SDK-test\n TrackDisplayName: SDK-test\n SessionID: 10\n SubSessionID: 20\n TrackLength: 5.0 km\n";h.sessionOffset=4096;h.sessionLen=int(yaml.size());memcpy(bytes+4096,yaml.data(),yaml.size());
 for(int i=0;i<13;++i){iracing::Variable v{};v.type=5;v.offset=i*8;v.count=1;strcpy(v.name,names[i]);memcpy(bytes+112+i*sizeof(v),&v,sizeof(v));}memcpy(bytes,&h,sizeof(h));
 auto folder=std::filesystem::temp_directory_path()/(L"LANDELTA-SDK-test-"+token);iracing::Feed feed;feed.start(folder,name,event);
 for(int i=0;i<90;++i){int b=i%2;double row[]={0,2,1+i/60.,100.,.1+i*.001,100.,1,-.125,1,.7,.1,30.,.25};memcpy(bytes+h.buffers[b].offset,row,sizeof(row));MemoryBarrier();reinterpret_cast<iracing::Header*>(bytes)->buffers[b].tick=i;SetEvent(ready);Sleep(17);}
 Sleep(100);auto snapshot=feed.read();feed.stop();bool ok=false;
 try{auto session=race::loadCSV(snapshot.path);ok=session.samples.size()>=50&&session.game=="iRacing"&&snapshot.quality.expected==60&&session.samples.back().delta==-125&&session.samples.back().sourceHz==60;std::filesystem::remove(snapshot.path);std::filesystem::remove(folder);}catch(...){}
 CloseHandle(ready);UnmapViewOfFile(bytes);CloseHandle(mapping);return ok;
}

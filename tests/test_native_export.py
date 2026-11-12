"""Verify the C++ in-app exporter with Python's independent ZIP implementation."""
from pathlib import Path
import subprocess, tempfile, zipfile, json
root=Path(__file__).resolve().parents[1]
source=r'''
#include "session_zip.hpp"
int main(int argc,char** argv){race::Session s;s.game="ACC";s.track="Monza";s.steerUnit="native";s.path=std::filesystem::path(argv[1])/"source.csv";race::Lap l;l.number=1;l.ms=109000;l.complete=true;l.valid=-1;l.sectors={35000,36000,38000};s.laps.push_back(l);race::exportZIP(s,std::filesystem::path(argv[1])/"out.zip");}
'''
with tempfile.TemporaryDirectory(prefix='landelta-zip-') as tmp:
    p=Path(tmp);(p/'test.cpp').write_text(source)
    original=b'capture_ms,lap,gas\r\n100,1,0.7\r\n'
    (p/'source.csv').write_bytes(original)
    subprocess.run(['g++','-std=c++17','-I',str(root/'src'),'-I',str(root/'vendor'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test'),tmp],check=True)
    with zipfile.ZipFile(p/'out.zip') as z:
        assert z.testzip() is None
        assert set(z.namelist())=={'samples.csv','laps.csv','session.json'}
        assert z.read('samples.csv')==original
        meta=json.loads(z.read('session.json'));assert meta['track']=='Monza' and meta['car_version'] is None
        assert b'109000,1,1,0,-1,35000,36000,38000' in z.read('laps.csv')
print('PASS: native session ZIP, original bytes, lap summary, unknown metadata and independent CRC verification')

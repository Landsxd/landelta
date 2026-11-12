from pathlib import Path
import subprocess, tempfile, zipfile, json
root=Path(__file__).resolve().parents[1]
html=(root/'tools/ac-analyzer.html').read_text().split('<script>')[1].split('</script>')[0]
js=(root/'tools/session-analysis.js').read_text()
setup="const assert=require('assert'),fs=require('fs');const nodes={};global.document={getElementById:id=>nodes[id]||(nodes[id]={value:'50'})};global.addEventListener=()=>{};\n"
checks=r'''
const row=(lap,t,p,c,id,extra={})=>({lap,stint:1,lap_ms:t,progress:p,capture_ms:c,physics_packet:id,graphics_packet:id,in_pit:0,tyres_out:0,sector:0,last_sector_ms:1234,...extra});
const a=[row(1,99900,.999,1000,1)],b=[row(2,50,.0005,1150,2,{last_lap_ms:100000}),row(2,100,.001,1200,3,{last_lap_ms:100000})];
assert(finishEvidence(a,b));assert.strictEqual(finishEvidence(a,[{...b[0],capture_ms:1400}]),null);
assert.strictEqual(finishEvidence(a,[{...b[0],stint:2}]),null);
const data=new Map([[1,a],[2,b]]);assert.strictEqual(boundaryRows(data,1).at(-1).progress,1);assert.strictEqual(boundaryRows(data,2)[0].progress,0);
const sectorRows=[row(1,100,.1,100,1),row(1,150,.2,150,2,{sector:1}),row(1,200,.21,200,3,{sector:1,last_sector_ms:30000})];
assert.strictEqual(sessionLaps(new Map([[1,sectorRows]]))[0].official_sectors_ms[0],30000);
assert.strictEqual(sessionLaps(new Map([[1,sectorRows.slice(0,2)]]))[0].official_sectors_ms[0],null);
const q=packetQuality([row(1,0,0,0,1),row(1,50,.01,50,1),row(1,100,.02,100,2)]);
assert.strictEqual(q.physics_new,2);assert.strictEqual(q.physics_hz,10);
assert.strictEqual(sessionLaps(new Map([[1,[row(1,100,.1,100,1,{tyres_out:4})]]]))[0].validity,'unknown');
const pit=sessionLaps(new Map([[1,[row(1,0,0,0,1,{in_pit:1}),row(1,50,.01,50,2),row(1,100,.02,100,3,{in_pit:1})]]]))[0];
assert(pit.pit_entry&&pit.pit_exit);assert.strictEqual(pit.completeness,'incomplete');
const bytes=new Uint8Array([239,187,191,65,13,10]);
sessionZip([['muestras-originales.csv',bytes],['sesion.json',JSON.stringify(sessionMetadata(data))],['resumen-vueltas.csv',summaryCSV(data,50)]]).arrayBuffer().then(b=>fs.writeFileSync(process.argv[2],Buffer.from(b)));
'''
with tempfile.TemporaryDirectory() as tmp:
    script=Path(tmp)/'test.js';archive=Path(tmp)/'session.zip'
    script.write_text(setup+js+html+checks)
    subprocess.run(['node',str(script),str(archive)],check=True)
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None
        assert z.read('muestras-originales.csv')==bytes([239,187,191,65,13,10])
        assert json.loads(z.read('sesion.json'))['scope']=='entire_loaded_session'
        assert len(z.read('resumen-vueltas.csv').splitlines())==3
print('PASS: boundary evidence, carried sector values, pit states, unknown validity, unique packets and original-byte ZIP integrity')

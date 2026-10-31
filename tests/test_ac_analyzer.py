"""Exercise the actual embedded analyzer code in Node, without a browser/network."""
from pathlib import Path
import subprocess
root = Path(__file__).resolve().parents[1]
script = (root / 'tools/ac-analyzer.html').read_text().split('<script>')[1].split('</script>')[0]
setup = "const assert=require('assert'); const nodes={}; global.document={getElementById:id=>nodes[id]||(nodes[id]={})};global.addEventListener=()=>{};\n"
checks = r'''
const rows=Array.from({length:1001},(_,i)=>({progress:i/1000,lap_ms:i*100,capture_ms:i*100,graphics_packet:i,gap_ms:100,in_pit:0,tyres_out:0}));
assert(sectors(rows,50).every(x=>Math.abs(x-2000)<.001));
const broken=rows.filter((_,i)=>i<200||i>240);assert.strictEqual(sectors(broken,50)[10],null);
assert(sectors([],50).every(x=>x===null));
assert.throws(()=>readCSV('wrong\n1,2'));
assert.throws(()=>readCSV(columns.join(',')+'\n1,2'));
const values=columns.map(()=>0);values[3]=1;values[6]=.5;
assert.strictEqual(readCSV(columns.join(',')+'\n'+values.join(',')).size,1);
const duplicate=rows.flatMap(r=>[r,r]);assert.deepStrictEqual(sectors(duplicate,50),sectors(rows,50));
assert.strictEqual(quality(rows).observed_hz,10);
assert.strictEqual(time(94340),'1:34.340');
const summary=lapSummary(new Map([[1,[{...rows[0],sector:1,last_sector_ms:30000},{...rows[1],sector:2,last_sector_ms:31000}]],[2,[{...rows[2],last_lap_ms:94000}]]]));
assert.strictEqual(summary[0].official_lap_ms,94000);assert.deepStrictEqual(summary[0].official_sectors_ms,[30000,31000,33000]);assert.strictEqual(summary[1].official_lap_ms,null);
console.log('PASS: production analyzer schema, missing values, interpolation, duplicate graphics, gaps, measured Hz');
'''
subprocess.run(['node'], input=setup+script+checks, text=True, check=True)

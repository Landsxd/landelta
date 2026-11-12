/* Local-only analysis. Unknown is not valid; interpolated is not measured. */
function packetQuality(rows) {
  const result={samples:rows.length,physics_new:0,graphics_new:0,physics_hz:null,graphics_hz:null,max_gap_ms:0,gaps_over_250_ms:0};
  for(const channel of ['physics','graphics']){
    let previous=null,first=null,last=null,count=0;
    for(const r of rows){const id=r[channel+'_packet'];if(!Number.isFinite(id)||id===previous)continue;
      previous=id;count++;if(first===null)first=r.capture_ms;last=r.capture_ms;
    }
    result[channel+'_new']=count;
    result[channel+'_hz']=count>1&&last>first?+(1000*(count-1)/(last-first)).toFixed(2):null;
  }
  for(let i=1;i<rows.length;i++){const gap=rows[i].capture_ms-rows[i-1].capture_ms;result.max_gap_ms=Math.max(result.max_gap_ms,gap);if(gap>250)result.gaps_over_250_ms++;}
  return result;
}
function finishEvidence(before,after){
  if(!before?.length||!after?.length)return null;
  const a=before.at(-1),b=after[0],dt=b.capture_ms-a.capture_ms,total=b.last_lap_ms;
  if((a.stint||1)!==(b.stint||1)||b.lap!==a.lap+1||dt<=0||dt>250||a.progress<.98||b.progress>.02||a.in_pit||b.in_pit||
    !(total>0&&total<2147483647)||!(a.lap_ms<=total)||b.lap_ms>250||total-a.lap_ms>250)return null;
  // Official clock supplies boundary time; both sides must support a continuous crossing.
  const elapsed=total-a.lap_ms+b.lap_ms;
  if(elapsed<=0||Math.abs(elapsed-dt)>100)return null;
  const end=a.capture_ms+dt*(total-a.lap_ms)/elapsed;
  return {total,at:end,method:'official_clock_with_adjacent_samples'};
}
function boundaryRows(data,key){
  const rows=data.get(key)||[],start=finishEvidence(data.get(key-1),rows),end=finishEvidence(rows,data.get(key+1));
  const out=rows.slice();
  if(start)out.unshift({...rows[0],progress:0,lap_ms:0,capture_ms:start.at,graphics_packet:-90000001});
  if(end)out.push({...rows.at(-1),progress:1,lap_ms:end.total,capture_ms:end.at,graphics_packet:-90000002});
  return out;
}
function sessionLaps(data){
  return [...data].map(([key,rows])=>{
    const start=finishEvidence(data.get(key-1),rows),end=finishEvidence(rows,data.get(key+1));
    const splits=[null,null,null];let previous=null,pending=null;
    for(const r of rows){
      if(previous&&r.graphics_packet===previous.graphics_packet)continue;
      if(previous&&r.sector===previous.sector+1&&previous.sector>=0&&previous.sector<2&&r.capture_ms-previous.capture_ms<=250)
        pending={index:previous.sector,old:previous.last_sector_ms,at:r.capture_ms};
      // Never consume the carried value just because we are inside the next sector.
      if(pending&&r.capture_ms-pending.at<=250&&r.last_sector_ms!==pending.old&&r.last_sector_ms>0&&r.last_sector_ms<2147483647){
        splits[pending.index]=r.last_sector_ms;pending=null;
      }
      if(pending&&r.capture_ms-pending.at>250)pending=null;
      previous=r;
    }
    const total=end?.total??null;
    if(total!==null&&splits[0]!==null&&splits[1]!==null&&total>splits[0]+splits[1])splits[2]=total-splits[0]-splits[1];
    let pitOut=false,pitIn=false;
    for(let i=1;i<rows.length;i++){if(rows[i-1].in_pit&&!rows[i].in_pit)pitOut=true;if(!rows[i-1].in_pit&&rows[i].in_pit)pitIn=true;}
    const complete=!!start&&!!end;
    return {lap:key,stint:rows[0]?.stint||1,number:rows[0]?.lap,official_lap_ms:total,official_sectors_ms:splits,s3_derived:splits[2]!==null,
      completeness:complete?'complete':'incomplete',pit_exit:pitOut,pit_entry:pitIn,
      state:pitOut&&pitIn?'out_and_in_lap':pitOut?'out_lap':pitIn?'in_lap':complete?'complete':'incomplete',
      validity:'unknown',validity_source:'AC shared-memory prefix does not provide certified lap validity',
      boundaries:{start:!!start,end:!!end},quality:quality(rows)};
  });
}
function sessionMetadata(data){
  const rows=[...data.values()].flat(),r=rows[0]||{};
  return {schema:2,source,scope:'entire_loaded_session',microsectors:Number($('bins').value),
    game:'Assetto Corsa (2014)',track:r.track_id??null,layout:r.layout_id??null,car:r.car_id??null,
    game_version:r.game_version??null,car_version:null,track_length_m:r.track_length_m>0?r.track_length_m:null,
    ambient_temperature_c:null,track_temperature_c:null,compound:r.compound??null,
    assists:{abs:r.abs_setting??null,traction_control:r.tc_setting??null,auto_clutch:r.auto_clutch??null,auto_blip:r.auto_blip??null,stability:r.stability??null},
    unknown_policy:'null means unavailable, not zero or disabled; car/mod version is not inferred from car ID',
    units:{capture_ms:'monotonic milliseconds since recording segment',lap_ms:'game lap clock, milliseconds',progress:'normalized track position 0..1',
      gas:'normalized input 0..1, not force',brake:'normalized input 0..1, not force',steer_native:'raw shared-memory steerAngle; physical steering-wheel angle and ratio unverified; no conversion to degrees',
      speed_kmh:'km/h',fuel_l:'litres',acceleration:'g',gear_native:'0 reverse, 1 neutral, 2 first gear',rpm:'revolutions/minute'},
    quality:packetQuality(rows),sector_policy:'sequential graphics transitions; changed last-sector value within 250 ms; ambiguous unchanged values stay unknown',
    boundary_policy:'adjacent laps, same stint, <=250 ms gap, near finish, compatible official clock; otherwise null',
    validity_policy:'tyres_out is a review warning only; not official invalidity'};
}
function summaryCSV(data,n){
  const rows=[['stint','lap','state','completeness','validity','pit_exit','pit_entry','lap_ms','s1_ms','s2_ms','s3_derived_ms',...Array.from({length:n},(_,i)=>'microsector_'+(i+1)+'_ms')]];
  for(const r of sessionLaps(data))rows.push([r.stint,r.number,r.state,r.completeness,r.validity,r.pit_exit,r.pit_entry,r.official_lap_ms,...r.official_sectors_ms,...sectors(boundaryRows(data,r.lap),n)]);
  return rows.map(row=>row.map(v=>v===null?'':v).join(',')).join('\r\n');
}
// ZIP STORE, UTF-8 names, CRC32, no remote libraries. Original bytes are untouched.
function sessionZip(files){
  const enc=new TextEncoder(),parts=[],central=[];let offset=0;
  function crc(bytes){let c=0xffffffff;for(const b of bytes){c^=b;for(let i=0;i<8;i++)c=(c>>>1)^((c&1)?0xedb88320:0);}return (c^0xffffffff)>>>0;}
  for(const [name,input] of files){const bytes=typeof input==='string'?enc.encode(input):new Uint8Array(input),nm=enc.encode(name),sum=crc(bytes);
    const h=new Uint8Array(30+nm.length),v=new DataView(h.buffer);v.setUint32(0,0x04034b50,true);v.setUint16(4,20,true);v.setUint16(6,0x800,true);v.setUint32(14,sum,true);v.setUint32(18,bytes.length,true);v.setUint32(22,bytes.length,true);v.setUint16(26,nm.length,true);h.set(nm,30);
    const c=new Uint8Array(46+nm.length),w=new DataView(c.buffer);w.setUint32(0,0x02014b50,true);w.setUint16(4,20,true);w.setUint16(6,20,true);w.setUint16(8,0x800,true);w.setUint32(16,sum,true);w.setUint32(20,bytes.length,true);w.setUint32(24,bytes.length,true);w.setUint16(28,nm.length,true);w.setUint32(42,offset,true);c.set(nm,46);
    parts.push(h,bytes);central.push(c);offset+=h.length+bytes.length;
  }
  const end=new Uint8Array(22),v=new DataView(end.buffer);v.setUint32(0,0x06054b50,true);v.setUint16(8,files.length,true);v.setUint16(10,files.length,true);v.setUint32(12,central.reduce((n,c)=>n+c.length,0),true);v.setUint32(16,offset,true);
  return new Blob([...parts,...central,end],{type:'application/zip'});
}

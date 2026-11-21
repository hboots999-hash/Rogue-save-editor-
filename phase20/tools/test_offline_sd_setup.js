// SPDX-License-Identifier: GPL-3.0-or-later
// Synthetic-only unit test for the browser-based local catalog/setup tool.
const fs=require('fs'),vm=require('vm');const {webcrypto}=require('crypto');
global.crypto=webcrypto;global.document={getElementById(){return {onclick:null}}};
const html=fs.readFileSync(require('path').join(__dirname,'PKSM_Rogue_Offline_Setup_and_Verifier.html'),'utf8');
vm.runInThisContext(html.split('<script>')[1].split('</script>')[0],{filename:'pksm.js'});
const u16w=(b,i,v)=>{b[i]=v&255;b[i+1]=(v>>>8)&255};
const u32w=(b,i,v)=>{u16w(b,i,v);u16w(b,i+2,v>>>16)};
function encodeMon(b,at,plain,pid,ot){const order=PERMS[pid%24],key=pid^ot;u32w(b,at,pid);u32w(b,at+4,ot);let sum=0;for(let i=0;i<48;i+=2)sum=(sum+u16(plain,i))&0xffff;u16w(b,at+28,sum);for(let phys=0;phys<4;phys++){const off={G:0,A:12,E:24,M:36}[order[phys]];for(let i=0;i<12;i+=4)u32w(b,at+32+phys*12+i,u32(plain,off+i)^key)}}
function setSecChecks(b){for(let i=0;i<28;i++)u16w(b,i*4096+0xff6,sectorChecksum(b,i*4096))}
let old=new Uint8Array(SAVE_SIZE);let at1;
for(let bank=0;bank<2;bank++){let rotation=bank?5:3;for(let phys=0;phys<14;phys++){const start=(bank*14+phys)*4096,id=(phys+14-rotation)%14;u16w(old,start+0xff4,id);u32w(old,start+0xff8,0x08012025);u32w(old,start+0xffc,40+bank);} const s1=(bank*14+(1+rotation)%14)*4096;u32w(old,s1+0x234,1);const at=s1+0x238;at1=at;const p=new Uint8Array(48);u16w(p,0,149);p[8]=0xff;u16w(p,12,33);u16w(p,14,45);u16w(p,16,53);u16w(p,18,73);p[20]=35;p[21]=25;p[22]=15;p[23]=20;encodeMon(old,at,p,1,0x12345678)}
setSecChecks(old);
const info=saveInfo(old);if(info.active!==1)throw new Error('active bank wrong');
let edited=old.slice();let mon=plainPokemon(edited,at1),p=mon.p;p[8]=0xfc;u16w(p,12,89);p[20]=10;encodeMon(edited,at1,p,1,0x12345678);setSecChecks(edited);
const cat=new Uint8Array(COUNT*15);cat[89*15+13]=10;cat[89*15+14]=10;
const result=compareSaveDraft(old,edited,cat);
if(result.newId!==89||result.species!==149||result.changed>8)throw new Error('wrong result '+JSON.stringify(result));
try{const broken=edited.slice();broken[0]++;compareSaveDraft(old,broken,cat);throw Error('failed to reject damage')}catch(e){if(e.message==='failed to reject damage')throw e}
// Read-only extraction geometry for simple synthetic CIA, without matching approved digest.
const content=align64(align64(align64(0x2020+64)+64)+64),exefs=content+512,romAt=exefs+512;
let cia=new Uint8Array(romAt+ROM_SIZE);u32w(cia,0,0x2020);u32w(cia,8,64);u32w(cia,12,64);u32w(cia,16,64);u32w(cia,24,ROM_SIZE+2048);cia.set(Buffer.from('NCCH'),content+256);u32w(cia,content+0x1a0,1);cia.set(Buffer.from('.code'),exefs);u32w(cia,exefs+8,0);u32w(cia,exefs+12,ROM_SIZE);cia[romAt+0xa0]=80;
// make contents satisfy CIA content size length (ciSize+content<=file.len)
let ciaLarger=new Uint8Array(content+ROM_SIZE+2048);ciaLarger.set(cia);
const rom=ciaRom(ciaLarger);if(rom.length!==ROM_SIZE||rom[0xa0]!==80)throw new Error('CIA extraction failed');
console.log('PASS: exact 128KiB dual-bank synthetic save, encrypted species 149 move edit 33→89 PP10, corruption rejection, CIA binary geometry');

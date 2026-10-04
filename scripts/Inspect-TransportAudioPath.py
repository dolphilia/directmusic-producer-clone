from pathlib import Path
import json,hashlib,struct,sys
p=Path(sys.argv[1]);r=json.loads((p/'run.json').read_text(encoding='utf-8-sig'))
h=lambda f:hashlib.sha256(Path(f).read_bytes()).hexdigest()
assert r['passed'] and r['exitCode']==0 and h(r['executable'])==r['exeSha256']
assert h(r['buildSummary'])==r['buildSummarySha256']
result=json.loads((p/'stdout.txt').read_text(encoding='utf-8-sig'));assert result['passed'] and result['checks']==11
def chunks(b):
 assert b[:4]==b'RIFF' and struct.unpack_from('<I',b,4)[0]==len(b)-8
 out=[];pos=12
 while pos<len(b):
  size=struct.unpack_from('<I',b,pos+4)[0];end=pos+8+size+(size&1);assert end<=len(b)
  out.append(b[pos:end]);pos=end
 assert pos==len(b);return out
source=(p/'core/source.sgp').read_bytes();selected=(p/'core/selected.sgp').read_bytes();config=(p/'core/default.aup').read_bytes();carrier=(p/'core/carrier.sgp').read_bytes()
assert source[8:12]==selected[8:12]==carrier[8:12]==b'DMSG' and config[8:12]==b'DMAP'
assert chunks(selected)==chunks(source)+[config]
assert [x for x in chunks(carrier) if x[:4]==b'RIFF' and x[8:12]==b'DMAP']==[config]
assert (p/'core/different.aup').read_bytes()!=config
files=[dict(path=str(f.resolve()),sha256=h(f)) for f in sorted((p/'core').iterdir()) if f.is_file()]
proof=dict(passed=True,nativeChecks=11,runSha256=h(p/'run.json'),outputs=files,scope='Independent RIFF slice comparison; source chunks unchanged plus one whole-byte default DMAP; Motif carrier DMAP exact. Embedded priority validated by native assertion, runtime/GUI priority not claimed.',fullAcceptance=False)
(p/'transport-proof.json').write_text(json.dumps(proof,indent=2)+'\n',encoding='utf-8');print(json.dumps(proof))

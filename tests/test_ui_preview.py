"""Verify readable compact layout and per-pixel alpha in the exported production UI."""
from pathlib import Path
from PIL import Image
import xml.etree.ElementTree as ET
root=Path(__file__).resolve().parents[1]
im=Image.open(root/'Vista-demo.png').convert('RGBA')
assert im.size==(1280,900)
assert im.getpixel((640,850))[3]==0  # unused space
assert im.getpixel((32,530))[3]==0   # gap between vertical timing cards
assert im.getpixel((30,479))[3]==64  # panel background at 25%
assert im.getpixel((800,411))[3]==255 # green delta fill stays opaque
svg=ET.parse(root/'Vista-demo.svg')
texts=[e for e in svg.iter() if e.tag.endswith('text')]
labels=[e.text or '' for e in texts]
assert 'CLASI' in labels and 'REL' in labels
assert 'CLASIFICACIÓN' not in labels and 'RELATIVOS' not in labels
pedal_labels=[e.text or '' for e in texts if 292<=float(e.attrib['x'])<=592 and 477<=float(e.attrib['y'])<=565]
assert not any('%' in l for l in pedal_labels);assert 'GAS' in labels and 'FRENO' in labels and '-0.327 s' in labels
assert 'SECT · vs mejor previa' in labels and '+0.008' in labels and '-0.034' in labels
names=[e for e in texts if e.text in ('L. Landeros','A. Torres','D. Moreno')]
assert names and all(float(e.attrib['font-size'])>=24 for e in names)
relative=[e for e in texts if float(e.attrib['x'])>=668 and 131<float(e.attrib['y'])<331]
assert any(e.text=='A. Torres' and e.attrib['fill']=='#00c3ff' for e in relative)
assert any(e.text=='D. Moreno' and e.attrib['fill']=='#ffcf00' for e in relative)
assert [e.text for e in sorted(relative,key=lambda e:float(e.attrib['y'])) if e.text in ('C. Fuentes','A. Torres','L. Landeros','D. Moreno','S. Martínez')]==['C. Fuentes','A. Torres','L. Landeros','D. Moreno','S. Martínez']
sectors=[e for e in texts if e.text in ('S1','S2','S3') and float(e.attrib['x'])<268]
assert [e.text for e in sorted(sectors,key=lambda e:float(e.attrib['y']))]==['S1','S2','S3']
assert len({e.attrib['x'] for e in sectors})==1
assert '0:35.400' in labels and '0:36.400' in labels
assert any('Lo alcanzas en ~' in l for l in labels)
for file,expected in [('Vista-historial.svg','MONZA'),('Vista-sesion.svg','INVÁLIDA'),('Vista-control.svg','Pedales')]:
 words=[e.text or '' for e in ET.parse(root/file).iter() if e.tag.endswith('text')]
 assert expected in words
control_words=[e.text or '' for e in ET.parse(root/'Vista-control.svg').iter() if e.tag.endswith('text')]
assert 'LANDELTA' in control_words
start_words=[e.text or '' for e in ET.parse(root/'Vista-inicio.svg').iter() if e.tag.endswith('text')]
assert 'Mostrar overlays al entrar en sesión' in start_words
print('PASS: compact headers, vertical sectors, 24px names, relative order/colors, pedals, delta, sector comparison, transparency and history/control views')

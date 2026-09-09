"""Create a distant, uneven three-knock cue from the retained CC0 Kenney pack."""
from pathlib import Path
import sys, json, wave
import numpy as np
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'Saved/ShowcaseExpansion/AudioTools'))
import soundfile as sf
rate=44100; mix=np.zeros(rate*4)
source,sr=sf.read(root/'Saved/ShowcaseExpansion/AudioSource/KenneyImpact/Audio/impactWood_heavy_001.ogg')
if source.ndim>1: source=source.mean(axis=1)
source=np.interp(np.arange(round(len(source)*rate/sr*1.8))*sr/rate/1.8,np.arange(len(source)),source)
for time,gain in [(0.1,.72),(.78,.5),(1.94,1)]:
    start=int(time*rate); n=min(len(source),len(mix)-start)
    mix[start:start+n]+=source[:n]*gain
    for delay,echo in [(.13,.17),(.31,.09),(.55,.045)]:
        i=start+int(delay*rate); n=min(len(source),len(mix)-i)
        mix[i:i+n]+=source[:n]*gain*echo
mix-=mix.mean(); mix*=.55/max(.001,abs(mix).max())
mix[:220]*=np.linspace(0,1,220); mix[-220:]*=np.linspace(1,0,220)
out=root/'SourceArt/ShowcaseHorror/Audio'
with wave.open(str(out/'SW_Loop_DistantKnock.wav'),'wb') as wav:
    wav.setnchannels(1); wav.setsampwidth(2); wav.setframerate(rate)
    wav.writeframes((mix*32767).astype('<i2').tobytes())
metrics=json.loads((out/'audio_metrics.json').read_text())
metrics['SW_Loop_DistantKnock']={'seconds':4,'peak':float(abs(mix).max()),'rms':float(np.sqrt(np.mean(mix*mix))),'loop':False}
(out/'audio_metrics.json').write_text(json.dumps(metrics,indent=2))
print(metrics['SW_Loop_DistantKnock'])

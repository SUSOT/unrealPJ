"""Small CC0 plate/utensil cue; reuses the verified Kenney download."""
from pathlib import Path
import sys, json, wave
import numpy as np
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'Saved/ShowcaseExpansion/AudioTools'))
import soundfile as sf
src=root/'Saved/ShowcaseExpansion/AudioSource/KenneyImpact/Audio'
out=root/'SourceArt/ShowcaseHorror/Audio'
rate=44100
mix=np.zeros(int(rate*1.5))
for filename,start,gain in [('impactPlate_light_001.ogg',.02,.7),('impactPlate_light_003.ogg',.31,.45),('impactMetal_light_002.ogg',.86,.12)]:
    a,sr=sf.read(src/filename)
    if a.ndim>1: a=a.mean(axis=1)
    if sr!=rate: a=np.interp(np.arange(round(len(a)*rate/sr))*sr/rate,np.arange(len(a)),a)
    i=int(start*rate); count=min(len(a),len(mix)-i)
    mix[i:i+count]+=a[:count]*gain
mix-=mix.mean()
mix[:220]*=np.linspace(0,1,220); mix[-220:]*=np.linspace(1,0,220)
mix*=.35/max(.001,abs(mix).max())
with wave.open(str(out/'SW_Loop_Cutlery.wav'),'wb') as wav:
    wav.setnchannels(1); wav.setsampwidth(2); wav.setframerate(rate); wav.writeframes((mix*32767).astype('<i2').tobytes())
metrics=json.loads((out/'audio_metrics.json').read_text(encoding='utf-8'))
metrics['SW_Loop_Cutlery']={'seconds':1.5,'peak':float(abs(mix).max()),'rms':float(np.sqrt(np.mean(mix*mix))),'loop':False}
(out/'audio_metrics.json').write_text(json.dumps(metrics,indent=2),encoding='utf-8')
print(metrics['SW_Loop_Cutlery'])

"""Convert CC0 Kenney foley and build quiet original electrical room tone.

Run with bundled Python + numpy; soundfile is installed under Saved/.../AudioTools.
No speech, music, ultrasonic tones, or high-volume shock sounds.
"""
from pathlib import Path
import json
import sys
import wave
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'Saved/ShowcaseExpansion/AudioTools'))
import soundfile as sf

SRC = ROOT/'Saved/ShowcaseExpansion/AudioSource/KenneyImpact/Audio'
OUT = ROOT/'SourceArt/ShowcaseHorror/Audio'
OUT.mkdir(parents=True,exist_ok=True)
RATE = 44100
metrics = {}

def read(name):
    data,rate = sf.read(SRC/name)
    if data.ndim > 1: data=data.mean(axis=1)
    if rate != RATE: data=np.interp(np.arange(round(len(data)*RATE/rate))*rate/RATE,np.arange(len(data)),data)
    return data

def write(name,data,peak=None,loop=False):
    data=np.asarray(data,dtype=float)
    data-=data.mean()
    if not loop:
        attack=min(22,len(data)//4)
        release=min(220,len(data)//4)
        data[:attack]*=np.linspace(0,1,attack)
        data[-release:]*=np.linspace(1,0,release)
    if peak is not None: data*=peak/max(.001,abs(data).max())
    assert abs(data).max()<.95,'Clipping risk'
    pcm=(data*32767).astype('<i2')
    with wave.open(str(OUT/(name+'.wav')),'wb') as wav:
        wav.setnchannels(1); wav.setsampwidth(2); wav.setframerate(RATE); wav.writeframes(pcm.tobytes())
    metrics[name]={'seconds':round(len(data)/RATE,3),'peak':round(float(abs(data).max()),4),
                   'rms':round(float(np.sqrt(np.mean(data*data))),4),'loop':loop}

for index in range(4):
    write('SW_Loop_Step_'+str(index),read('footstep_concrete_%03d.ogg'%index),peak=.5)

rng=np.random.default_rng(2947)
n=int(RATE*1.45)
drag=np.zeros(n)
for i in range(15):
    grain=read('footstep_concrete_%03d.ogg'%(i%4))
    grain=np.interp(np.arange(round(len(grain)*1.2))/1.2,np.arange(len(grain)),grain)
    start=int((.045+i*.07)*RATE)
    count=min(len(grain),n-start)
    drag[start:start+count]+=grain[:count]*(.4+.2*np.sin(i*2))
wood=read('impactWood_light_001.ogg')
start=int(.98*RATE)
drag[start:start+min(len(wood),n-start)]+=wood[:min(len(wood),n-start)]*.7
# Granular friction with a dark wooden resonance, not a cinematic impact.
t=np.arange(n)/RATE
drag+=.02*np.sin(2*np.pi*157*t)*np.sin(np.pi*np.clip(t/1.2,0,1))**2
write('SW_Loop_ChairDrag',drag,peak=.4)
write('SW_Loop_Relay',read('impactMetal_light_001.ogg'),peak=.27)

# Exactly periodic 12-second bed: mains hum, faint ventilation, slight beating.
n=RATE*12
t=np.arange(n)/RATE
noise=rng.normal(0,1,n)
freq=np.fft.rfftfreq(n,1/RATE)
filtered=np.fft.rfft(noise)*np.exp(-freq/800)*(1-np.exp(-freq/100))
air=np.fft.irfft(filtered,n=n)
air*=.004/max(.001,np.std(air))
hum=(.037*np.sin(2*np.pi*60*t)+.014*np.sin(2*np.pi*120*t)+.008*np.sin(2*np.pi*180*t))
hum*=1+.06*np.cos(2*np.pi*t/12)
write('SW_Loop_RoomTone',hum+air,loop=True)
(OUT/'audio_metrics.json').write_text(json.dumps(metrics,indent=2),encoding='utf-8')
print(json.dumps(metrics,indent=2))

"""PIE contract: hard local flicker then sustained outage, no all-room blackout."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

u.EditorPythonScripting.set_keep_python_script_alive(True)
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
out=Path(u.Paths.project_saved_dir())/'ShowcaseExpansion'
assert levels.load_level('/Game/Developers/MOON/Level/Showcase1')
levels.editor_request_begin_play()
state={'since':time.monotonic(),'world':None,'samples':[],'bulb_samples':[],'busy':False}
report={}

def emissive(mat):
    if isinstance(mat,u.MaterialInstanceDynamic): return mat.get_scalar_parameter_value('EmissiveStrenght')
    return u.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(mat,'EmissiveStrenght')
def tick(delta):
    if state['busy']: return
    state['busy']=True
    try:
        if state['world'] is None:
            world=editor.get_game_world()
            if world is None:
                assert time.monotonic()-state['since']<30,'PIE did not start'
                return
            pawn=u.GameplayStatics.get_player_pawn(world,0)
            if pawn is None: return
            director=u.GameplayStatics.get_all_actors_of_class(world,u.ShowcaseLoopDirector)[0]
            # Use actual pawn movement and public pacing settings, not private state.
            director.set_editor_property('first_change_distance',100)
            director.set_editor_property('second_change_distance',200)
            director.set_editor_property('turn_back_unlock_distance',300)
            lamps=list(director.get_editor_property('lamps'))
            lights=[a.get_components_by_class(u.LightComponent) for a in lamps]
            bases=[[c.get_editor_property('intensity') for c in parts] for parts in lights]
            bulbs=[]
            for lamp in lamps:
                for mesh in lamp.get_components_by_class(u.MeshComponent):
                    for slot in range(mesh.get_num_materials()):
                        mat=mesh.get_material(slot)
                        if mat and isinstance(mat,u.MaterialInstance):
                            value=emissive(mat)
                            if value>0: bulbs.append((mat,value))
            state.update(world=world,pawn=pawn,director=director,lights=lights,bases=bases,bulbs=bulbs,bulb_min=1.0,since=time.monotonic(),step=0)
            return
        elapsed=time.monotonic()-state['since']
        if elapsed<1.0: return
        if state['step']<10:
            if elapsed<1+state['step']*.12: return
            pawn=state['pawn']
            pawn.set_actor_location(pawn.get_actor_location()+u.Vector(40,3,0),False,True)
            state['step']+=1
            return
        ratios=[]
        for parts,base in zip(state['lights'],state['bases']):
            ratios.append(max(c.get_editor_property('intensity')/b for c,b in zip(parts,base) if b>0))
        state['samples'].append(ratios)
        bulb_ratios=[]
        for mat,base in state['bulbs']:
            value=emissive(mat)
            state['bulb_min']=min(state['bulb_min'],value/base)
            bulb_ratios.append(value/base)
        state['bulb_samples'].append((u.GameplayStatics.get_time_seconds(state['world']),bulb_ratios))
        if elapsed<18: return
        samples=state['samples']
        report.update(samples=len(samples),lowest_ratio=min(min(s) for s in samples),
                      minimum_lit_lamps=min(sum(v>.2 for v in s) for s in samples),stage=state['director'].get_editor_property('current_stage'),bulb_minimum=state['bulb_min'])
        assert report['stage']==3,'Did not reach distortion stage'
        assert report['lowest_ratio']<.12,'No distinct local flicker; light never dips below 12%'
        minimum=1 if state['director'].get_editor_property('straight_corridor') else 12
        assert report['minimum_lit_lamps']>=minimum,'No readable light remains beside or behind the player'
        assert state['bulbs'] and state['bulb_min']<.12,'Lamp surface still glows while its light is off'
        # Inspect real rendered-material inputs, not the director's private clock.
        # Require two hard off/on blinks followed by a sustained fully-off interval.
        sharp_counts=[]
        longest_outage=0.0
        completed_patterns=0
        for bulb in range(len(state['bulbs'])):
            previous=None
            sharp=0
            off_since=None
            edges=[]
            for stamp,values in state['bulb_samples']:
                value=values[bulb]
                if previous:
                    old_time,old_value=previous
                    if stamp-old_time<=.10 and abs(value-old_value)>.85:
                        sharp+=1
                        edges.append('off' if value<.05 else 'on')
                if value<.01:
                    if off_since is None: off_since=stamp
                    longest_outage=max(longest_outage,stamp-off_since)
                    if stamp-off_since>=2.0 and edges[-5:]==['off','on','off','on','off']:
                        completed_patterns+=1
                        edges=[]
                else: off_since=None
                previous=(stamp,value)
            sharp_counts.append(sharp)
        report.update(max_sharp_edges=max(sharp_counts,default=0),longest_full_outage=longest_outage,
                      blink_then_off_patterns=completed_patterns)
        assert report['max_sharp_edges']>=5,'Light is fading smoothly instead of snapping off/on'
        assert longest_outage>=2.0,'No sustained fully-off interval after flickering'
        assert completed_patterns>0,'Missing off/on/off/on/off sequence followed by darkness'
        report['passed']=True
        finish()
    except Exception:
        report['passed']=False
        report['error']=traceback.format_exc()
        u.log_error(report['error'])
        finish()
    finally: state['busy']=False
def finish():
    (out/'flicker_validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    u.log_warning('SHOWCASE_FLICKER_TEST '+json.dumps(report))
    u.unregister_slate_post_tick_callback(handle)
    levels.editor_request_end_play()
    u.SystemLibrary.quit_editor()
handle=u.register_slate_post_tick_callback(tick)

"""Convert the saved circular Showcase1 into a recycled straight diner. Backup first.
Uses the recorded original furniture transforms and the project's existing assets.
Only Showcase1 and the new knock sound are saved. No shared material edits.
"""
import json, math
from pathlib import Path
import unreal as u
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
api=u.get_editor_subsystem(u.EditorActorSubsystem)
assert levels.load_level('/Game/Developers/MOON/Level/Showcase1')
actors=list(api.get_all_level_actors())
d=next(a for a in actors if isinstance(a,u.ShowcaseLoopDirector))
ext=next(a for a in actors if isinstance(a,u.ShowcaseRepeatExtension))
assert not d.get_editor_property('straight_corridor'),'Already converted; refuse to overwrite newer edits.'
out=Path(u.Paths.project_saved_dir())/'ShowcaseExpansion'
records=json.loads((out/'inspection.json').read_text())['actors']
template=[a for a in records if a['folder']=='3']
assert len([a for a in template if a['class']=='/Script/Engine.StaticMeshActor'])==170
COUNT=24; FIRST=-12; PITCH=1200
def transform(record,cell=0):
    p=record['translation']; q=record['rotation']; s=record['scale']
    t=u.Transform(location=u.Vector(p[0]-1980+cell*PITCH,p[1],p[2]),scale=u.Vector(*s))
    t.rotation=u.Quat(*q); return t
groups={}
def add(mesh,materials,t,collision=True,shadow=True,cull=0):
    key=(mesh.get_path_name(),tuple(m.get_path_name() if m else '' for m in materials),collision,shadow,cull)
    groups.setdefault(key,{'mesh':mesh,'materials':materials,'transforms':[],'collision':collision,'shadow':shadow,'cull':cull})['transforms'].append(t)
floor=u.load_asset('/Game/RestaurantScene/Meshes/SM_Floor')
wall=u.load_asset('/Game/RestaurantScene/Meshes/SM_Wall01')
cube=u.load_asset('/Engine/BasicShapes/Cube')
def mats(mesh): return [x.material_interface for x in mesh.get_editor_property('static_materials')]
for x in (0,300,600,900):
    for y in (-400,-100,200): add(floor,mats(floor),u.Transform(location=u.Vector(x,y,20)))
    add(wall,mats(wall),u.Transform(location=u.Vector(x,-400,20)))
    add(wall,mats(wall),u.Transform(location=u.Vector(x+300,500,20),rotation=u.Rotator(yaw=180)))
for y,z,w,h in ((50,-1,960,40),(50,410,960,25),(-425,210,30,450),(525,210,30,450)):
    add(cube,mats(cube),u.Transform(location=u.Vector(600,y,z),scale=u.Vector(12,w/100,h/100)))
unique=set(); movable={}
for row in template:
    if row['class']!='/Script/Engine.StaticMeshActor': continue
    c=row['meshes'][0]; name=c['mesh'].rsplit('.',1)[-1]
    if name in ('SM_Floor','SM_Wall01','Cube'): continue
    t=transform(c['transform']); p=t.translation; q=t.rotation; scale=t.scale3d
    key=(c['mesh'],tuple(c['materials']),tuple(round(v,2) for v in (p.x,p.y,p.z)),tuple(round(v,4) for v in (q.x,q.y,q.z,q.w,scale.x,scale.y,scale.z)))
    if key in unique: continue
    unique.add(key)
    mesh=u.load_asset(c['mesh']); materials=[u.load_asset(m) if m else None for m in c['materials']]
    if name in ('SM_Chair','SM_WallPainting_1') and name not in movable:
        movable[name]=(mesh,materials,t); continue
    tiny=name.startswith('SM_Shaker') or name=='SM_NapkinDispenser'
    detail=tiny or name=='SM_Decals' or 'Cables' in name or 'Pipes_Support' in name
    add(mesh,materials,t,not detail and 'NO_COLLISION' not in c['collision'],c['cast_shadow'] and not tiny,3000 if tiny else 6000 if detail else 8000 if 'Painting' in name else 0)
mesh_groups=[]
for data in groups.values():
    g=u.ShowcaseRepeatMeshGroup()
    for k,v in [('mesh',data['mesh']),('materials',data['materials']),('source_transforms',data['transforms']),('collision_enabled',data['collision']),('cast_shadow',data['shadow']),('detail_cull_distance',data['cull'])]: g.set_editor_property(k,v)
    mesh_groups.append(g)
lamp_source=next(a for a in actors if a.get_actor_label()=='Loop_Lamp_01')
cable_source=next(a for a in actors if a.get_actor_label()=='Loop_Cable_01')
lamp_record=next(a for a in template if 'PF_Ceilinglamp' in a['class'])
cable_record=next(a for a in template if 'BP_Cablespline' in a['class'])
fixtures=[]; lamps=[]; signs=[]; changes=[]; new_chairs={}
for cell in range(FIRST,FIRST+COUNT):
    for kind,source,record in [('Lamp',lamp_source,lamp_record),('Cable',cable_source,cable_record)]:
        a=ext.duplicate_fixture(source,u.Vector())
        assert a
        a.root_component.set_mobility(u.ComponentMobility.MOVABLE)
        a.set_actor_transform(transform(record['transform'],cell),False,True)
        a.set_actor_label('Straight_%s_%+03d'%(kind,cell)); a.set_folder_path('StraightShowcase/Fixtures')
        fixtures.append(a)
        if kind=='Lamp':
            lamps.append(a)
            for light in a.get_components_by_class(u.LightComponent):
                light.set_mobility(u.ComponentMobility.MOVABLE)
                light.set_intensity(160 if light.get_editor_property('intensity')>20 else 1.6)
                light.set_light_color(u.LinearColor(.67,.78,.86,1))
    for name,(mesh,materials,base) in movable.items():
        t=u.Transform(location=base.translation+u.Vector(cell*PITCH,0,0),scale=base.scale3d); t.rotation=base.rotation
        a=api.spawn_actor_from_class(u.StaticMeshActor,t.translation)
        c=a.static_mesh_component; c.set_mobility(u.ComponentMobility.MOVABLE); c.set_static_mesh(mesh)
        for slot,m in enumerate(materials): c.set_material(slot,m)
        c.set_collision_profile_name('NoCollision'); c.set_editor_property('generate_overlap_events',False)
        a.set_actor_transform(t,False,True); a.set_actor_label('Straight_Change_%s_%+03d'%(name,cell))
        a.set_folder_path('StraightShowcase/UnseenChanges'); fixtures.append(a)
        if name=='SM_Chair': new_chairs[cell]=a
        if name=='SM_Chair' and cell==-1:
            d.set_editor_property('reverse_chair',a); d.set_editor_property('reverse_chair_tucked_pose',t)
            a.set_actor_location(t.translation-u.Vector(65,0,0),False,True)
            a.set_folder_path('StraightShowcase/ReverseEncounter'); continue
        changed=u.Transform(location=t.translation,scale=t.scale3d)
        changed.rotation=t.rotation*u.Rotator(yaw=65 if name=='SM_Chair' else 180).quaternion()
        if name=='SM_Chair': changed.translation+=u.Vector(0,28,0)
        change=u.ShowcaseSpatialChange()
        for k,v in [('target',a),('changed_transform',changed),('stage',1 if name=='SM_Chair' else 2)]: change.set_editor_property(k,v)
        changes.append(change)
    if cell%2==0:
        sign=api.spawn_actor_from_class(u.TextRenderActor,u.Vector(cell*PITCH+900,-365,315),u.Rotator(yaw=90))
        sign.set_actor_label('Straight_ExitDirection_%+03d'%cell); sign.set_folder_path('StraightShowcase/Wayfinding')
        c=sign.get_component_by_class(u.TextRenderComponent); c.set_mobility(u.ComponentMobility.MOVABLE)
        c.set_text('EXIT  >'); c.set_world_size(20); c.set_horizontal_alignment(u.HorizTextAligment.EHTA_CENTER)
        c.set_text_render_color(u.Color(135,170,155,255)); signs.append(sign); fixtures.append(sign)
ext.set_actor_transform(u.Transform(),False,True)
for k,v in [('infinite_straight',True),('first_cell_index',FIRST),('repeat_count',COUNT),('repeat_spacing',PITCH),('mesh_groups',mesh_groups),('repeated_fixtures',fixtures)]: ext.set_editor_property(k,v)
ext.set_actor_label('Showcase1_InfiniteStraight'); ext.set_folder_path('StraightShowcase/Architecture')
ext.rebuild_extension(); ext.apply_fixture_distance_limits()
for k,v in [('straight_corridor',True),('straight_repeat_span',COUNT*PITCH),('loop_center',u.Vector()),('spatial_changes',changes),('lamps',lamps),('direction_signs',signs),('first_change_distance',1800.0),('second_change_distance',3800.0),('turn_back_unlock_distance',6000.0),('door_distance_ahead',550.0),('flicker_interval',10.0)]: d.set_editor_property(k,v)
d.set_folder_path('StraightShowcase/System')
# Transform the retained optional reverse-table vignette from the circle into the straight cell.
r=3055.7749073643904
def unbend(t):
    p=t.translation; a=math.atan2(p.x,r+50-p.y)
    loc=u.Vector(a*r,50+r-math.hypot(p.x,p.y-r-50),p.z)
    result=u.Transform(location=loc,scale=t.scale3d); result.rotation=u.Rotator(yaw=-math.degrees(a)).quaternion()*t.rotation
    return result
for a in actors:
    if a.get_actor_label().startswith('Loop_Reverse_'):
        a.set_actor_transform(unbend(a.get_actor_transform()),False,True); a.set_folder_path('StraightShowcase/ReverseEncounter')
        if isinstance(a,u.SpotLight): a.get_component_by_class(u.LightComponent).set_intensity(20)
anchor=d.get_editor_property('reverse_return_seat_location')
d.set_editor_property('reverse_return_seat_location',unbend(u.Transform(location=anchor)).translation)
start=next(a for a in actors if isinstance(a,u.PlayerStart))
start.set_actor_location(u.Vector(220,165,112),False,True); start.set_actor_rotation(u.Rotator(),True)
# Only remove the task-owned circular fixtures, changes, and signs now replaced.
removed=[]
for a in actors:
    if a.get_actor_label().startswith(('Loop_Lamp_','Loop_Cable_','Loop_Change_','Loop_ExitDirection_')):
        removed.append(a.get_actor_label()); assert api.destroy_actor(a)
    elif 'CircularLoop/Escape/Destination' in str(a.get_folder_path()): a.set_folder_path('StraightShowcase/EscapeDestination')
door=d.get_editor_property('escape_door'); door.set_folder_path('StraightShowcase/System')
for a in actors:
    if isinstance(a,u.PostProcessVolume):
        pp=a.get_editor_property('settings')
        settings={'auto_exposure_min_brightness':-3.0,'auto_exposure_max_brightness':-3.0,'auto_exposure_bias':.6,'color_saturation':u.Vector4(.72,.76,.8,1),'color_contrast':u.Vector4(1.05,1.05,1.05,1),'color_gamma':u.Vector4(1,1,1,1),'color_gain':u.Vector4(.92,.97,1,1),'white_temp':6500.0,'vignette_intensity':.38,'film_grain_intensity':.12,'bloom_intensity':.22,'motion_blur_amount':0.0}
        for k,v in settings.items(): pp.set_editor_property('override_'+k,True); pp.set_editor_property(k,v)
        a.set_editor_property('settings',pp)
fog=api.spawn_actor_from_class(u.ExponentialHeightFog,u.Vector(0,0,150))
fog.set_actor_label('Straight_DistantDarkness'); fog.set_folder_path('StraightShowcase/Atmosphere')
c=fog.get_component_by_class(u.ExponentialHeightFogComponent)
for k,v in [('fog_density',.024),('fog_height_falloff',.001),('fog_inscattering_luminance',u.LinearColor(.00025,.0004,.00055,1)),('fog_max_opacity',1.0),('start_distance',1400.0)]: c.set_editor_property(k,v)
task=u.AssetImportTask()
for k,v in [('filename',str(Path(u.Paths.project_dir())/'SourceArt/ShowcaseHorror/Audio/SW_Loop_DistantKnock.wav')),('destination_path','/Game/Developers/MOON/Audio/Horror'),('automated',True),('save',True)]: task.set_editor_property(k,v)
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
sound=u.load_asset('/Game/Developers/MOON/Audio/Horror/SW_Loop_DistantKnock'); assert isinstance(sound,u.SoundWave)
sound.set_sound_asset_compression_type(u.SoundAssetCompressionType.PCM); u.EditorAssetLibrary.save_loaded_asset(sound)
d.set_editor_property('distant_knock_sound',sound)
assert levels.save_current_level()
(out/'straight_install.json').write_text(json.dumps({'cells':COUNT,'span_m':COUNT*12,'instances':ext.get_repeated_instance_count(),'fixtures':len(fixtures),'spatial_changes':len(changes),'removed_circular_actors':removed},indent=2))
u.log_warning('SHOWCASE_STRAIGHT_SAVED')
u.SystemLibrary.quit_editor()

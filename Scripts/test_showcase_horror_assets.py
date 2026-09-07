"""Saved-map contract for grounded, localized horror; commandlet, read-only."""
import unreal as u

levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
assert levels.load_level('/Game/Developers/MOON/Level/Showcase1')
actors = u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
d = next(a for a in actors if isinstance(a, u.ShowcaseLoopDirector))
for change in d.get_editor_property('spatial_changes'):
    actor = change.get_editor_property('target')
    before = actor.get_actor_transform()
    after = change.get_editor_property('changed_transform')
    assert abs(before.translation.z-after.translation.z) < 1, 'Horror props must remain grounded: '+actor.get_actor_label()
    assert (before.scale3d-after.scale3d).length() < .01, 'No stretched furniture: '+actor.get_actor_label()
for name in ('SW_Loop_Step_0', 'SW_Loop_Step_1', 'SW_Loop_Step_2', 'SW_Loop_Step_3',
             'SW_Loop_ChairDrag', 'SW_Loop_Relay', 'SW_Loop_RoomTone', 'SW_Loop_Cutlery'):
    sound = u.load_asset('/Game/Developers/MOON/Audio/Horror/'+name)
    assert isinstance(sound, u.SoundWave), 'Missing real audio asset: '+name
    assert sound.get_editor_property('duration') > .05, 'Empty sound: '+name
chair=d.get_editor_property('reverse_chair')
pose=d.get_editor_property('reverse_chair_tucked_pose')
assert chair and d.get_editor_property('reverse_focus_light') and d.get_editor_property('reverse_cutlery_sound')
assert abs(chair.get_actor_location().z-pose.translation.z)<1
assert 60<(chair.get_actor_location()-pose.translation).length()<70
assert all(c.get_editor_property('target')!=chair for c in d.get_editor_property('spatial_changes'))
assert len([a for a in actors if a.get_actor_label().startswith('Loop_Reverse_')])==5
u.log_warning('SHOWCASE_HORROR_ASSETS_PASS')
u.SystemLibrary.quit_editor()

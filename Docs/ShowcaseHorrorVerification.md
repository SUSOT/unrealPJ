# Showcase1 psychological-horror revision — verification (2026-09-04)

## Scope and review baseline

User request: replace the unconvincing exaggerated spatial effects with an atmosphere
that creates believable psychological unease. Retain the one-way progression and
turn-back escape already approved by the user.

Task-start baseline: `Saved/ShowcaseExpansion/Backup/Horror_20260904_105401/`.
Most feature files were already untracked from earlier work, so a HEAD-only diff
would not isolate this revision. No commits, pushes, character/input changes, or
other level edits were performed. Per the user's explicit preference, the two
review axes were checked sequentially by the main agent, without sub-agents.

## Standards axis

- The director owns all level-local cues; no global audio or game-mode dependency was added.
- Eight reusable spatial audio components plus one room-tone component; no tick-time asset loading/spawning.
- Lamp emissive changes use per-instance dynamic materials; shared source materials remain unchanged.
- Cue positions are fixed world positions, not camera attachments. Audio stops on escape/EndPlay.
- 32 exaggerated booth actors were merged back into existing HISM groups, not discarded as visible furniture.
- Small PCM sound assets retain their source WAVs and CC0 provenance.
- Editor and game Development builds succeeded. Git whitespace check passed for tracked changes.

## Request / behavior axis

- Removed floating/scaled furniture. Chairs stay grounded, with small lateral movement and rotation.
- Only nearby, previously visible props change when outside the conservative view guard, with a nine-second gap.
- Distance-driven own steps establish a rhythm; intermittent delayed footsteps and a single post-stop step imply an unseen presence.
- Turning to look back suppresses following footsteps. Existing sounds remain at their original world positions.
- Local light-failure sequences use the same timing for lamp intensity and visible bulb emissive, with a room-tone dip.
- Retained 40/70/90m progression and deliberate 5m return requirement. Door traversal, destination clearance, and restart behavior passed.
- Saved map exposure is -3 EV100, verified by reloading/rendering. This corrects an almost-black saved view without changing other maps.

## Evidence

- `ShowcaseHorrorRhythmRed.log`: expected failures before presence-rhythm implementation.
- `ShowcaseHorrorRhythmGreen.log`: all three native tests passed.
- `ShowcaseHorrorAssetsRed.log`: expected failure on old scaled props.
- `ShowcaseHorrorAssetsGreen.log`: grounded transforms and all seven sound assets passed.
- `ShowcaseHorrorEscape.log`: 2048 floor and 4096 bidirectional capsule checks, actual PIE progression/door crossing/restart passed.
- `ShowcaseHorrorBulbRed.log`: caught the visible bulb remaining on despite light dimming.
- `ShowcaseHorrorBulbFinal.log`: 750 samples; lowest light ratio 0.01377, bulb ratio 0.02, at least 13/16 fixtures remain lit.
- `ShowcaseHorrorAudioFinal.log`: real audio-device PIE test passed movement, pause, camera turn, localized cue and furniture-change checks.
- `Saved/ShowcaseExpansion/showcase_horror_runtime.wav`: 50.47s stereo engine-output capture; peak 0.04977, RMS 0.002878, nonzero stereo difference. No clipped samples.
- `ShowcaseHorrorFinalSave.log`: final map/sound save succeeded without the earlier commandlet-only BINKA decoder ensure.
- `horror_saved_exposure.png`: final saved-exposure render, no exposure override needed.

The first unattended audio recording was silent because Unreal mutes an unfocused
window by default. The final test used a process-local command-line override;
project audio settings were not changed. Early script API-name/type errors were
corrected and the affected tests re-run.

These checks verify implementation behavior and output, not a guarantee that every
player finds it frightening. No blind human playtest, headphone listening review,
packaged build playthrough, or 60FPS benchmark was performed in this revision.

## Follow-up: hard flicker instead of fades (2026-09-04 12:26 KST)

- User requested abrupt blinking followed by darkness. Replaced only the director's
  light-switch waveform; two short interruptions precede a 0.65/1.6/3-second outage.
  Direct light and bulb emissive consume the same switch value. No brightness overshoot.
- Baseline backup: `Saved/ShowcaseExpansion/Backup/HardFlicker_20260904_122123/`.
- Red test detected zero abrupt transitions in the previous smooth waveform.
- Green PIE test: 857 samples, light and emissive minima both 0, longest fully-off
  interval 2.984 seconds, six complete blink/blink/outage patterns, at least 13 of
  16 fixtures still lit. Log: `ShowcaseHardFlickerGreen.log`.
- Editor/game builds and all three native Showcase regression tests passed.
- Direct standards review: bounded existing timing function, retained strength/off
  controls, no new tick work or asset allocations. Direct request review: hard
  transitions and sustained outage confirmed; no unrelated gameplay changes.
- Map SHA256 before/after unchanged:
  `154AE5E913528DFA160F4879B6367BB6F4C959C0575287B1B2AD4EB0DCC4AF7D`.

## Straight corridor revision (2026-09-07)

- Scope: Showcase1 changed from a circular diner to 24 straight 12m cells around
  the player. Distant cells/fixtures are recycled without teleporting the player.
  New pacing is 18/38/60m. A doorway is placed about 5.5m behind outside the view;
  the first stationary turn reveals it, and its position locks once observed.
- Existing reverse-table encounter converted to straight coordinates. Corrected
  its test harness to obtain saved light settings before PIE starts.
- Horror presentation: uneven spatial knocks from retained CC0 source audio,
  late footsteps, witness-dependent furniture, hard electrical failures, a dark
  vanishing point, and readable nearby pools of light. Research and tuning values:
  `Docs/ShowcaseStraightResearch.md`.
- Backup: `Saved/ShowcaseExpansion/Backup/Straight_20260907/`.
- `ShowcaseStraightPIE.log`: 721 floor samples and 700 capsule sweeps passed;
  immediate turn-to-door, fixed observed door, physical crossing, safe destination
  and restart passed. Audio output peak 0.076355, non-silent and unclipped.
- Cell recycling tested at nine positions from -450m to +460m: at least 110m
  of floor ahead/behind at every position, no player relocation, 122 recycled
  cells, constant 4008 HISM instances. This is functional coverage, not an FPS claim.
- `ShowcaseStraightReverse.log`: all 15 reverse encounter assertions passed,
  including seeing the backrest, hidden tuck, cutlery, one-shot replay protection,
  ambience restore and normal-route return. Actual audio peak 0.127014.
- `ShowcaseStraightFlicker.log`: 742 samples, direct light and bulb emissive
  minima 0; 11 abrupt transitions, ten detected blink/blink/outage patterns,
  longest full outage 2.986s, at least five fixtures above 20% intensity.
- Editor and game Development builds passed. `ShowcaseStraightNative.log`:
  all four existing Showcase state regression tests passed.
- Direct scope review: preserves character/input and destination. Destruction
  limited to the 72 named circular fixtures/changes/signs replaced by straight
  versions. Backup retains them. Shared RestaurantScene materials were not edited;
  19 local copies provide serialized HISM usage flags, plus local exit materials.
- Direct code review: streaming bounded to a fixed window; geometry updates occur
  at cell crossings; audio reuses the existing eight components; no new per-frame
  asset loading. Door collision/traversal verified through actual pawn movement.
- `git diff --check` passed. Validation and preview scripts do not save changes.
- First renderer exposed overly dark nearby surfaces; corrected light radii,
  spot/fill intensities and inherited shadow grading. Saved-level renders:
  `straight_start.png`, `straight_dread.png`, `straight_turn_door.png`.
- Final exit polish gives the door/frame low emissive visibility. A blocking
  overlap check selects a furniture-free location 4.5–9m behind (prefers 5.5m),
  addressing a table clipping the first preview's doorframe. Follow-up runs:
  `ShowcaseStraightPIEFinal.log`, `ShowcaseStraightClearDoorRender.log`.
- Final saved map SHA256:
  `130C8C6ABCDEEBF067BAF15ED0DDE665EFA4AC1182BFA283D988ECFFC40DBBC5`.

No packaged-playthrough, blind human fear study, or 60FPS benchmark is claimed.

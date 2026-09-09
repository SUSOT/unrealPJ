# Showcase1: 뒤돌아 탈출하기

> 2026-09-07: 현재 Showcase1은 **직선 무한 복도 / 18·38·60m 진행 / 뒤돌면 바로 보이는 문**으로 변경했다.
> 최신 동작과 조절 값은 [ShowcaseStraightResearch.md](ShowcaseStraightResearch.md)를 참고한다.
> 아래 원형 배치·40/70/90m·5m 뒤로 걷기 내용은 이전 구현 기록이다. 원형용 설치/검증 스크립트를 현재 맵에 재적용하지 않는다.

대상 맵: `/Game/Developers/MOON/Level/Showcase1`.
캐릭터, 입력, GameMode, 다른 레벨은 변경하지 않는다. 기존 원형 통로를 유지한다.

## 플레이 흐름

- PlayerStart가 바라보는 방향으로 걷는다. 초반에 돌아서도 탈출문은 생기지 않는다.
- 초반에는 평범한 자기 발소리와 낮은 실내 환기·전기음으로 기준을 만든다.
- 진행 거리 40m: 간헐적으로 뒤에서 0.26초 늦은 발소리가 따라온다. 멈추면 약 0.55초 뒤에 한 걸음이 더 들린다. 뒤를 보면 추적 발소리가 멎는다.
- 한 번 본 가까운 의자는 시선 밖으로 나간 뒤 28cm 당겨지고 65도 돌아간다. 그 자리에서 가구 마찰음이 난다. 소품 변화는 9초 이상 간격을 두며, 11m 이내에서만 일어난다.
- 70m: 방금 보았던 액자가 벽 쪽으로 뒤집힐 수 있다. 앞쪽 전등 두 개가 순서대로 꺼지고 실내 기계음도 잠깐 잦아든다.
- 90m: 앞쪽 전등 세 개가 순서대로 꺼져 몇 초간 어두운 구간을 만든다. 뒤쪽 길과 탈출문의 조명은 남고, 출구 표지는 보지 않는 동안 돌아가는 방향으로 바뀐다.
- 공중 부양, 가구 높이 늘리기, 맵 전체 동시 변형은 제거했다. 어둠 속에서도 방금 본 배치를 비교할 수 있도록 맵 전용 고정 노출을 EV100 -3으로 조정했다.
- 변화 사이 간격은 40→30→20m로 줄어든다. 걷기 속도 3m/s라면 세 번째 단계까지 약 30초가 걸린다(실제 관찰 시점은 시선과 경로에 따라 달라짐).
- 이때 실제로 뒤돌아 5m 되짚어 걸으면, 돌아가는 길의 코너 너머 약 18–36m 앞에 문이 생긴다.
- 카메라만 돌리거나 앞을 보며 뒷걸음질하는 행동은 탈출 조건이 아니다.
- 문에 가까이 가면 1.25초에 걸쳐 자동으로 열린다. 열린 문을 정면에서 통과하면 짧은 페이드 뒤 별도의 탈출 공간으로 이동한다.
- 탈출 후 입력은 유지된다. 다음 맵이 지정되지 않았으므로 임의의 맵을 로드하거나 게임을 종료하지 않는다.
- 탈출문이 나타나면 추적 발소리와 새 조명 고장은 멎는다. 문 통과 시 이 연출의 소리도 정리된다.

거리는 원형 중심선 기준이다. 왕복으로 걸음 수를 늘려도 진행을 부풀릴 수 없다.
한 번 열린 문은 다시 앞으로 가도 사라지지 않는다. PIE를 다시 시작하면 모든 진행이 초기화된다.

## 처음부터 반대로 걸었을 때

- 시작점에서 반대쪽으로 약 10여 m 떨어진 기존 테이블에 별도 장면을 배치했다. 주변 전등과 실내 기계음이 잦아들고, 식기와 컵이 놓인 테이블에 따뜻한 조명이 남는다.
- 당겨진 의자를 가까이서 0.45초 이상 보면 뒤쪽 약 3.6m 지점에서 가구 마찰음이 한 번 난다.
- 소리가 난 방향으로 돌아보아 의자의 현재 위치와 옮겨질 위치가 모두 시선 밖이 된 경우에만 의자가 65cm 들어간다. 보고 있는 의자는 움직이지 않는다.
- 다시 의자를 확인하고 출발점 방향으로 2.5m 이상 돌아오면, 시선 밖의 빈 맞은편 자리에서 식기가 달그락거린다.
- 이 장면은 한 플레이 동안 한 번만 발생한다. 강제 사망, 이동 제한, 순간이동, 새 탈출문은 추가하지 않았다. 무시하고 지나갈 수도 있다.
- 출발점으로 돌아오면 주변 밝기와 룸톤이 복구된다. 이후 정방향으로 진행하면 기존 40/70/90m 공포와 뒤돌기 탈출이 그대로 작동한다.

Outliner의 `CircularLoop/Escape/ReverseEncounter`에 소품과 조명을 모았다.
Director의 `Loop > Reverse Encounter`에서 의자, 들어간 뒤의 Transform, 조명, 식기 소리와 발생 위치를 확인할 수 있다.
전용 의자는 일반 `Spatial Changes`에서 제외되어 두 연출이 같은 의자를 움직이지 않는다.

## 레벨에서 조절

Outliner의 `CircularLoop/Escape/System/Showcase1_TurnBackEscape`를 선택한다.

| 속성 | 기본값 (cm) | 의미 |
|---|---:|---|
| First Change Distance | 4000 | 첫 변화 |
| Second Change Distance | 7000 | 두 번째 변화 |
| Turn Back Unlock Distance | 9000 | 뒤돌기 탈출 조건 활성화 |
| Required Backtrack Distance | 500 | 뒤돌아 걸어야 하는 거리 |
| Door Distance Ahead | 1800 | 돌아가는 방향의 문 배치 거리 |

거리 단계는 작은 값부터 순서대로 유지한다. 변화를 보지 않는 동안 적용하므로 거리 임계값과 실제 목격 시점은 다를 수 있다.
`Spatial Changes`에서 대상 소품, 변경 Transform, 발생 단계를 수정할 수 있다.
`Showcase1_ReversalExitDoor`의 `Destination`은 `Showcase1_EscapeDestination`을 참조한다.
추후 다음 레벨 연결은 Director의 `OnEscapeCompleted` 이벤트에 연결하면 된다.

`Loop > Flicker`에서 `Enable Light Flicker`를 끄면 깜빡임만 제거된다.
`Flicker Strength`(0–1)로 강도, `Flicker Interval`(맵 값 12초, 실행 시 최소 8초)로 발생 간격을 조절한다.
일정한 박자가 되지 않도록 간격에 0.4–4.1초의 변화를 더한다.
전등은 페이드 없이 약 0.12초/0.15초의 짧은 꺼짐을 두 번 반복하고, 그 뒤 완전히 꺼진 상태를 유지한다.
소등 유지 시간은 단계별 0.65초/1.6초/3초이며, 다시 켜질 때도 즉시 켜진다. 3단계 순차 연출은 약 5초 안에 복구된다.
전구 머티리얼의 발광도 실제 조명과 함께 변하며, 원본 공유 머티리얼은 수정하지 않는다.

`Loop > Sound > Horror Volume`은 전체 연출 볼륨(맵 값 0.8), `Enable Presence Audio`는 음향 켜기/끄기다.
`OnHorrorCue(Cue, Location)` 이벤트를 이용해 추후 방향성 자막을 연결할 수 있다. 현재 별도 자막 UI는 없다.
소리 출처/라이선스와 원본 WAV는 `SourceArt/ShowcaseHorror/`에 보관한다. Kenney CC0 효과음과 직접 합성한 룸톤이며 음악·음성은 없다.
짧고 선명한 점멸이 있으므로 빛에 민감한 플레이어를 위한 깜빡임 해제 옵션은 유지한다.
단발 연출 뒤 충분한 간격을 두고, 맵 전체를 동시에 점멸시키지는 않는다.

## 성능과 검증

2336개 반복 인스턴스는 HISM 유지. 변화용 소품 32개 중 31개는 정방향, 1개는 역방향 전용이다.
역방향 테이블에 작은 식기 소품 4개와 그림자를 계산하지 않는 로컬 스포트라이트 1개를 추가했다.
Director는 초당 20회 갱신, 문은 등장한 뒤에만 Tick한다. 반복적인 에셋 로드나 통로 재생성은 없다.
음향은 룸톤 1개와 재사용하는 공간 효과음 컴포넌트 8개로 제한한다. 효과음은 작은 mono PCM 에셋이다.
이 작업에서 실기기 60FPS를 보장하는 성능 벤치마크는 수행하지 않았다.

- 빌드: UnrealTeamPJEditor Win64 Development
- 네이티브 테스트: `Automation RunTests UnrealTeamPJ.Showcase`
- 실제 전등 밝기 검사: `Scripts/test_showcase_flicker.py`
- 실제 PIE와 충돌 검증: `Scripts/validate_showcase_escape.py`
- 실제 음향·정지 반응·시선·가구 변화: `Scripts/validate_showcase_horror.py`
- 역방향 시야·조명·공간 음향·재방문·정방향 복귀: `Scripts/validate_showcase_reverse.py`
- 역방향 테이블 전후 렌더: `Scripts/render_showcase_reverse.py`
- 저장 에셋 및 지면 유지 검사: `Scripts/test_showcase_horror_assets.py`
- 밝기 비교 렌더: `Scripts/render_showcase_horror.py`
- 임시 렌더 검증: `Scripts/render_showcase_escape.py`
- 설치 후 표지 방향·발광과 탈출 공간 밝기 마감: `Scripts/finalize_showcase_escape_signs.py`
- 초기 심리 공포 설치: `Scripts/build_showcase_horror.py` (역방향 전용 의자를 다시 일반 변화 대상으로 만들 수 있으므로 현재 맵에 재실행하지 않는다)
- 역방향 설치 기록: `Scripts/build_showcase_reverse.py` (설치된 맵에서는 실행 거부; 실제 렌더링·오디오가 활성화된 full editor에서 사용)
- `tune_showcase_escape.py`는 이전 과장된 변형 버전의 기록이므로 현재 맵에 다시 적용하지 않는다.
- 검증 결과: `Saved/ShowcaseExpansion/escape_validation.json`
- 작업 전 맵 백업: `Saved/ShowcaseExpansion/Backup/Showcase1_before_escape_*.umap`
- 이번 조정 전 백업: `Saved/ShowcaseExpansion/Backup/Horror_20260904_105401/`
- 역방향 장면 추가 전 백업: `Saved/ShowcaseExpansion/Backup/ReverseEncounter_20260904_123909/`

자동 음향 테스트를 숨겨진 에디터로 실행하면 기본 비활성 창 음소거 때문에 녹음이 무음일 수 있다.
테스트 프로세스에만 `-ini:Engine:[Audio]:UnfocusedVolumeMultiplier=1.0`을 붙인다. 프로젝트 설정은 바꾸지 않는다.
네이티브 상태 검사와 실제 PIE 검사는 기능 검증이지, 플레이어가 반드시 공포를 느낀다는 보장은 아니다.

검증/렌더 스크립트는 테스트용 플레이어 이동·시점·조명 변경을 맵에 저장하지 않는다.
설치 스크립트 `build_showcase_escape.py`는 중복 배치를 막기 위해 이미 설치된 맵에서는 실행을 거부한다.

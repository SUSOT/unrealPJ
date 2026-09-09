# Showcase1: 끝없는 직선 식당과 뒤돌기 탈출

2026-09-07. 적용 대상은 MOON/Level/Showcase1. 작업 전 백업은
`Saved/ShowcaseExpansion/Backup/Straight_20260907/`.

## 조사에서 확인한 내용

- Amnesia/SOMA 개발사 Frictional은 놀라는 순간 사이의 기다림, 불확실성,
  플레이어가 스스로 상황을 해석할 여지가 공포 경험에 중요하다고 설명한다.
  [Thomas Grip, 9 Years, 9 Lessons on Horror](https://frictionalgames.com/2019-10-9-years-9-lessons-on-horror/).
- Alien: Isolation의 크리에이티브 리드 Alistair Hope는 소리를 실재감 있는
  환경의 일부로 다루고, 화면 사건에 따라 음악을 동적으로 변화시킨다고 설명한다.
  [개발자 인터뷰, PlayStation Blog](https://blog.playstation.com/?p=127046).

## 이 맵에 대한 설계 판단

위 자료에서 원칙을 참고했으며 게임의 연출이나 음원을 복제하지 않았다.
기존 식당의 생활감 있는 가구를 반복해 익숙한 공간을 만든 뒤, 이동과 시선에
반응하는 소리·가구·조명 변화가 그 익숙함을 깨도록 설계한다.

- 12m 길이의 직선 구간 24개를 플레이어 앞뒤에 유지한다. 멀리 있는 구간만
  재배치해 양방향으로 계속 걸을 수 있고, 플레이어 위치는 순간이동시키지 않는다.
- 가까운 테이블과 바닥은 읽히고 먼 소실점은 어둠으로 사라지게 한다.
  좁은 조명 범위, 약한 차가운 색보정, 낮은 필름 그레인을 사용한다.
- 18m: 뒤쪽 벽에서 불규칙한 두드림, 멈춘 뒤 추가 발소리, 시선 밖 의자 변화.
- 38m: 전방 조명이 짧게 깜빡인 뒤 소등되고 주변 기계음도 일시적으로 잦아든다.
- 60m: 출구가 약 5.5m 뒤 시야 밖에 준비된다. 돌아보면 바로 발견할 수 있다.
  이전의 '5m를 뒤로 걸어야 문 생성' 조건은 직선 모드에서 사용하지 않는다.
- 문틀이 테이블과 겹치지 않도록 4.5–9m 범위의 빈 위치를 검사한다. 5.5m를 우선한다.
- 계속 전진하는 동안 문은 시야 밖에서 가까운 뒤쪽을 유지한다. 발견 후에는
  위치를 고정한다. 접근해서 문을 통과하면 기존 탈출 공간에 도착한다.
- 기존 반대 방향 테이블 장면도 직선 좌표로 옮긴다.

두드림 음원은 프로젝트에 보관된 Kenney Impact Sounds CC0 목재 충격음을
시간차와 잔향으로 조합한 새 에셋이다. 출처는 `SourceArt/ShowcaseHorror/README.md`.

## 조절 위치

`Showcase1_TurnBackEscape`의 First Change Distance / Second Change Distance /
Turn Back Unlock Distance는 각각 1800 / 3800 / 6000cm.
Door Distance Ahead는 550cm. Straight Corridor가 직선 동작을 활성화한다.
`Showcase1_InfiniteStraight`의 Repeat Count는 24, Repeat Spacing은 1200cm.
이 값과 Director의 Straight Repeat Span(28800cm)은 함께 변경해야 한다.

실제 공포감은 개인차와 플레이 환경의 영향을 받는다. 자동 검증은 이동, 시야,
충돌, 문 통과, 음향 출력과 렌더 확인을 대상으로 하며 사용자 플레이 피드백으로 조율한다.

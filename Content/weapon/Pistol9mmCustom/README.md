# Pistol 9mm Custom — 기본 리깅 작업

작업일: 2026-09-14

## 열 파일

- `Pistol9mm_Rigged.blend`: 편집용 작업 파일. `Pistol9mm_Rig_Work` 장면이 작업 결과입니다.
- `Pistol9mm_BeforeRig_20260914.blend`: 작업 전 Blender 장면 백업.
- 원래 331개 오브젝트는 작업 파일 안의 `Scene` 장면에도 보존했습니다.
- `textures` 폴더는 작업 파일 옆에 유지하세요. 원본 다운로드 ZIP의 텍스처를 상대 경로로 연결했습니다.

## 구성

| 오브젝트 | 본 / 역할 |
|---|---|
| Pistol_Frame | Root / 몸통과 고정 부품 |
| Pistol_Slide | Slide / 슬라이드, 가늠쇠, 가늠자와 동반 부품 |
| Pistol_Magazine | Mag / 탄창 전체 |
| Pistol_Barrel | Barrel / 총열 |

`Slide`, `Mag`, `Barrel`은 `Root`의 자식입니다. 모든 정점은 해당 본 하나에 웨이트 1.0으로 연결되어 있습니다. 탄창 본 이름 `Mag`는 현재 프로젝트의 `M1911WeaponView.cpp`에서 사용하는 이름과 일치합니다. 본 이름의 일치만으로 기존 게임 코드와 호환성이 검증되는 것은 아닙니다.

`02_Attachments` 컬렉션에는 소음기, 조준기, 레일 마운트, 라이트, 레이저가 별도 메시로 있습니다. 소음기는 Barrel, 나머지는 Root를 따릅니다. 부착 위치와 피벗은 `attachment_sockets.json`에 작업 리그 좌표 기준으로 기록했습니다. UE 임포트 후 축 변환을 확인해서 소켓에 적용해야 합니다.

Blender 작업 좌표는 미터 단위, +X 전방, +Z 위쪽입니다. 부착물을 제외한 권총의 바운딩 박스는 약 18.69 × 2.72 × 12.69cm입니다.

## 확인용 애니메이션

- `Fire_Preview`: 30fps, 1–10프레임. 3프레임에서 슬라이드가 뒤로 3.42cm 이동하고 원위치로 돌아옵니다.
- `Reload_Preview`: 30fps, 1–60프레임. 탄창을 아래/뒤로 빼고 다시 넣는 기초 동작입니다.
- 현재 파일은 `Fire_Preview`, 1프레임으로 저장되어 있습니다.
- 손 동작, 탄창 교체 연출, 탄피 배출, 총열의 잠금 해제 회전, 게임 입력 연결은 포함하지 않습니다.

Blender Python Console에서 다음 코드로 탄창 확인 동작을 선택할 수 있습니다.

```python
bpy.data.objects['Pistol9mm_Rig'].animation_data.action = bpy.data.actions['Reload_Preview']; bpy.context.scene.frame_start = 1; bpy.context.scene.frame_end = 60; bpy.context.scene.frame_set(1)
```

발사 확인 동작으로 돌아가기:

```python
bpy.data.objects['Pistol9mm_Rig'].animation_data.action = bpy.data.actions['Fire_Preview']; bpy.context.scene.frame_start = 1; bpy.context.scene.frame_end = 10; bpy.context.scene.frame_set(1)
```

본을 직접 조작하려면 선택된 애니메이션을 해제하고 Pose Mode에서 조작하세요. 뷰포트의 오버레이는 최종 미리보기를 위해 꺼 두었습니다. 본 표시가 필요하면 오버레이를 켜세요.

## FBX 파일

- `SK_Pistol9mm_Custom.fbx`: 본체, 슬라이드, 탄창, 총열과 본 4개. 애니메이션 없이 내보냈습니다.
- `A_Pistol9mm_Fire.fbx`, `A_Pistol9mm_Reload.fbx`: 각 애니메이션과 동일 메시/리그 포함.
- `SM_Pistol_*.fbx`: 각 부착물의 독립 메시. 피벗이 부착 위치에 있고 내보내기 위치는 원점입니다.

UE에서는 본체 파일로 새 Skeleton을 만든 다음 애니메이션 파일을 그 Skeleton에 연결하는 순서로 진행하세요. 애니메이션 파일에도 메시가 포함되어 있으므로 애니메이션만 가져올 때 메시 임포트는 끕니다. 이 작업에서는 UE 임포트, 머티리얼 복원, 소켓 생성, BP 교체, 게임 실행을 수행하지 않았습니다.

## 검증 결과

- 전체 6,284개 정점의 단일 본 웨이트 확인.
- Slide/Mag/Barrel 조작 시 해당 부품만 움직이고, Root 조작 시 네 부품 모두 움직이는지 실제 평가된 정점 좌표로 검사.
- 본체 FBX를 Blender에 다시 임포트하여 본 이름/부모 구조, 정점 수, 웨이트 합, 크기 확인.
- 애니메이션 FBX 두 개를 다시 임포트하여 최대 이동량과 마지막 프레임 원위치 복귀 확인.
- 기본 자세, 슬라이드 후퇴, 탄창 분리 렌더를 각각 생성하고 육안 확인.
- 상세 수치는 `rig_validation.json` 참고.

렌더: `Pistol9mm_Ready.png`, `Pistol9mm_Fire.png`, `Pistol9mm_Reload.png`.

원본 에셋: 사용자가 다운로드한 `pistol-9mm-custom.zip`의 Pistol 9mm Custom. 이 폴더의 원본 모델/텍스처에는 원 에셋의 라이선스가 적용됩니다.

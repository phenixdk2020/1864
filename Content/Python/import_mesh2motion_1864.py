# Importerer de 18 Mesh2Motion-klip (SourceAssets/Mesh2Motion, se notes.md dér) som AnimSequences.
#   Hest:     SourceAssets/Mesh2Motion/Horse/A_Horse_*.fbx  -> /Game/Units/Horse/            skelet SK_Horse_1864, roll 0 (HORSE_CLIP_ROLL)
#   Menneske: SourceAssets/Mesh2Motion/Human/A_M2M_*.fbx    -> /Game/Units/Human/Mesh2Motion/ skelet SK_Human_1864, roll 90, samme import som A_Gun_*
#             (ingen ekstra hofte-tillæg: Hips er allerede løftet 98,079 cm i filerne).
# Kør kun når hverken UnrealEditor, Build.bat eller spillet kører (fra PowerShell):
#   UnrealEditor-Cmd.exe Game1864.uproject -run=pythonscript -script=<denne fil> -unattended -nosplash -stdout
# Eksisterende asset med samme navn kopieres først til Backup_<dato> i målmappen (én gang pr. dato) og slettes, så klippet importeres frisk.
# Env: M2M_ONLY=A_Horse_Run,A_M2M_Push (kun disse), M2M_HORSE_ROLL (0), M2M_HUMAN_ROLL (90).
# Kontrol pr. klip (log-linje 'M2M-IMPORT CHECK'): længde, frames, skelet, bækken-/hoftehøjde (komponentrum) ved frame 0, orientering OK/FEJL.
import os, datetime
import unreal

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(ROOT, 'SourceAssets', 'Mesh2Motion')
HORSE_DEST = '/Game/Units/Horse'
HUMAN_DEST = '/Game/Units/Human/Mesh2Motion'
HORSE_SKELETON_PATHS = ['/Game/Units/Horse/SK_Horse_1864_Skeleton']
HORSE_MESH = '/Game/Units/Horse/SK_Horse_1864'
HUMAN_SKELETON = '/Game/Units/Human/Skeletons/SK_Human_1864'
HORSE_CLIPS = ['A_Horse_Idle2', 'A_Horse_Eat', 'A_Horse_TurnLeft', 'A_Horse_TurnRight', 'A_Horse_Death', 'A_Horse_Run',
               'A_Horse_M2M_Walk', 'A_Horse_M2M_Trot', 'A_Horse_M2M_Rear', 'A_Horse_M2M_Kick']
HUMAN_CLIPS = ['A_M2M_' + n for n in ('Push', 'Walk_Carry', 'Driving', 'Sitting_Idle', 'Fixing_Kneeling', 'Kneeling_Tired',
                                      'Crouch_Idle', 'Death_A', 'Death_B', 'Death_C', 'Death_D', 'Idle_Sword')]
HORSE_ROLL = float(os.environ.get('M2M_HORSE_ROLL', '0'))
HUMAN_ROLL = float(os.environ.get('M2M_HUMAN_ROLL', '90'))
TODAY = datetime.date.today().strftime('%Y-%m-%d')
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
LOG = 'M2M-IMPORT '
failures = []
rows = []


def say(msg):
    unreal.log_warning(LOG + msg)


def fail(msg):
    failures.append(msg)
    unreal.log_error(LOG + 'PROBLEM ' + msg)


def remove_with_backup(dest, name):
    path = dest + '/' + name
    if EAL.does_asset_exist(path):
        backup = dest + '/Backup_' + TODAY
        if not EAL.does_asset_exist(backup + '/' + name):
            if not EAL.duplicate_asset(path, backup + '/' + name):
                raise RuntimeError('Backup fejlede, sletter ikke: ' + name)
        EAL.delete_asset(path)


def import_clip(src_dir, dest, name, skeleton, roll):
    remove_with_backup(dest, name)
    ui = unreal.FbxImportUI()
    for k, v in [('import_mesh', False), ('import_as_skeletal', True), ('import_animations', True), ('import_materials', False),
                 ('import_textures', False), ('skeleton', skeleton), ('automated_import_should_detect_type', False),
                 ('mesh_type_to_import', unreal.FBXImportType.FBXIT_ANIMATION), ('override_animation_name', name)]:
        ui.set_editor_property(k, v)
    ui.anim_sequence_import_data.set_editor_property('import_rotation', unreal.Rotator(roll=roll, pitch=0, yaw=0))
    t = unreal.AssetImportTask()
    for k, v in [('filename', os.path.join(src_dir, name + '.fbx')), ('destination_path', dest), ('destination_name', name),
                 ('automated', True), ('replace_existing', False), ('save', True), ('options', ui), ('factory', unreal.FbxFactory())]:
        t.set_editor_property(k, v)
    TOOLS.import_asset_tasks([t])
    seq = unreal.load_asset(dest + '/' + name)
    return seq if isinstance(seq, unreal.AnimSequence) else None


def find_bone(seq, suffix):
    names = [str(n) for n in unreal.AnimationLibrary.get_animation_track_names(seq)]
    return next((n for n in names if n.lower().endswith(suffix)), None)


def bone_z0(seq, bone):
    """Knoglens højde ved frame 0 i komponentrum (AnimPoseExtensions); falder tilbage til lokal translation."""
    try:
        pose = unreal.AnimPoseExtensions.get_anim_pose_at_frame(seq, 0, unreal.AnimPoseEvaluationOptions())
        t = unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD)
        return t.translation, 'komponent'
    except Exception as e:  # noqa
        t = unreal.AnimationLibrary.get_bone_pose_for_frame(seq, bone, 0, False)
        return t.translation, 'lokal(' + str(e)[:40] + ')'


def check(seq, kind, skeleton):
    name = seq.get_name()
    n = int(unreal.AnimationLibrary.get_num_frames(seq))
    bone = find_bone(seq, 'pelvis' if kind == 'horse' else 'hips')
    sk_ok = seq.get_editor_property('skeleton') == skeleton
    if not bone:
        fail(name + ' ingen pelvis/hips-track')
        rows.append((name, seq.get_play_length(), n, skeleton.get_name(), None, 'FEJL'))
        return
    loc, space = bone_z0(seq, bone)
    z = loc.z
    # Orientering: kroppen skal staa med Z op. Hest bækken ca. 105 cm; menneske ca. 94 cm staaende (sidde/knæle/hug lavere, jf. notes.md).
    # Ligger klippet ned, er Z lille og en anden akse stor.
    if kind == 'horse':
        # The horse's pelvis stands about 60 cm ahead of its root in the rest pose (A_Horse_Stand too).
        ok = 95.0 <= z <= 115.0 and abs(loc.y) < 40 and abs(loc.x) < 80 if 'Death' not in name else (95.0 <= z <= 115.0)
    else:
        ok = 40.0 <= z <= 110.0 and z > abs(loc.y) * 0.6 and z > abs(loc.x) * 0.6
    ok = bool(ok) and sk_ok
    say('CHECK %s length=%.3fs frames=%d skeleton=%s(%s) %s=(%.1f %.1f %.1f)[%s] %s' % (
        name, seq.get_play_length(), n, seq.get_editor_property('skeleton').get_name(), 'OK' if sk_ok else 'FORKERT', bone,
        loc.x, loc.y, loc.z, space, 'OK' if ok else 'FEJL'))
    if not ok:
        fail(name + ' orientering/skelet mistænkelig (z=%.1f)' % z)
    rows.append((name, seq.get_play_length(), n, seq.get_editor_property('skeleton').get_name(), z, 'OK' if ok else 'FEJL'))


def main():
    only = [c.strip() for c in os.environ.get('M2M_ONLY', '').split(',') if c.strip()]
    mesh = unreal.load_asset(HORSE_MESH)
    horse_skel = mesh.get_editor_property('skeleton') if isinstance(mesh, unreal.SkeletalMesh) else None
    human_skel = unreal.load_asset(HUMAN_SKELETON)
    if not isinstance(horse_skel, unreal.Skeleton):
        raise RuntimeError('SK_Horse_1864 mangler eller har intet skelet')
    if not isinstance(human_skel, unreal.Skeleton):
        raise RuntimeError('SK_Human_1864 mangler')
    jobs = [('horse', os.path.join(SRC, 'Horse'), HORSE_DEST, horse_skel, HORSE_ROLL, [c for c in HORSE_CLIPS if not only or c in only]),
            ('human', os.path.join(SRC, 'Human'), HUMAN_DEST, human_skel, HUMAN_ROLL, [c for c in HUMAN_CLIPS if not only or c in only])]
    for kind, src, dest, skel, roll, clips in jobs:
        say('%s roll=%.1f skelet=%s dest=%s clips=%s' % (kind, roll, skel.get_path_name(), dest, ','.join(clips)))
        for name in clips:
            if not os.path.isfile(os.path.join(src, name + '.fbx')):
                fail('fil mangler ' + name)
                continue
            seq = import_clip(src, dest, name, skel, roll)
            if not seq:
                fail('import fejlede ' + name)
                continue
            check(seq, kind, skel)
            EAL.save_loaded_asset(seq)
    for kind, src, dest, skel, roll, clips in jobs:
        for name in clips:
            if not EAL.does_asset_exist(dest + '/' + name):
                fail('asset mangler ' + dest + '/' + name)
    say('TABEL navn | laengde s | frames | skelet | hofte/baekken z f0 cm | status')
    for r in rows:
        say('TABEL %s | %.3f | %d | %s | %s | %s' % (r[0], r[1], r[2], r[3], '-' if r[4] is None else '%.1f' % r[4], r[5]))
    say('DONE problems=%d %s' % (len(failures), '; '.join(failures)))


main()

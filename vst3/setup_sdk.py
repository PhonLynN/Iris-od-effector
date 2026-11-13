import zipfile, pathlib, shutil

root = pathlib.Path(r'C:\Users\PhonLynN\dsh_Workspace\效果器\vst3')
sdk = root / 'vst3sdk'
pairs = {
    'vst3_base': 'base',
    'vst3_cmake': 'cmake',
    'vst3_pluginterfaces': 'pluginterfaces',
    'vst3_public_sdk': 'public.sdk',
}
for z, dest in pairs.items():
    zp = root / 'deps' / f'{z}.zip'
    with zipfile.ZipFile(zp) as zf:
        top = zf.namelist()[0].split('/')[0]
        tmp = root / 'deps' / f'_x_{z}'
        if tmp.exists():
            shutil.rmtree(tmp)
        zf.extractall(tmp)
        src = tmp / top
        dst = sdk / dest
        if dst.exists():
            shutil.rmtree(dst)
        shutil.move(str(src), str(dst))
        shutil.rmtree(tmp)
    print(dest, 'ok', sum(1 for _ in (sdk / dest).rglob('*')))

for p in ['cmake/modules/SMTG_AddVST3Library.cmake',
          'public.sdk/source/vst/vstsinglecomponenteffect.h',
          'pluginterfaces/vst/ivstaudioprocessor.h',
          'public.sdk/source/main/pluginfactory.h']:
    print(p, (sdk / p).exists())

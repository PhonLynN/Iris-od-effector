# Iris R 安装脚本（Cubase 必须已退出）
$ErrorActionPreference = "Stop"
$dst = Join-Path $env:LOCALAPPDATA "Programs\Common\VST3\Iris.vst3"
$src = Join-Path $PSScriptRoot "build\VST3\Release\Iris.vst3"
if (Get-Process Cubase* -ErrorAction SilentlyContinue) { Write-Host "请先退出 Cubase"; exit 1 }
Remove-Item -Recurse -Force $dst -ErrorAction SilentlyContinue
Copy-Item -Recurse -Force $src $dst
$mit = (Get-ChildItem -Recurse -Filter moduleinfotool.exe (Join-Path $PSScriptRoot "build\bin\Release") | Select-Object -First 1).FullName
& $mit -create -version 1.8.0 -path "$dst\Contents\x86_64-win\Iris.vst3" 2>&1 | Out-File -Encoding utf8 "$dst\Contents\Resources\moduleinfo.json"
$val = (Get-ChildItem -Recurse -Filter validator.exe (Join-Path $PSScriptRoot "build\bin\Release") | Select-Object -First 1).FullName
$tmp = Join-Path $env:TEMP "irisv18chk"
Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $tmp | Out-Null
Copy-Item -Recurse -Force $dst (Join-Path $tmp "Iris.vst3")
& $val (Join-Path $tmp "Iris.vst3")
Write-Host "Installed: $dst"

# Snowflake 3D — README/docs 用スクリーンショットを生成する。
# アプリの --shot キャプチャモードを使い、決まった形状を再現可能に書き出す。
param(
    [string]$QtTools = "C:/Qt/Tools",
    [string]$QtDir = "C:/Qt/6.11.1/mingw_64"
)

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
$env:Path = "$QtDir/bin;$QtTools/mingw1310_64/bin;" + $env:Path
$exe = "$repo/build/Snowflake3D.exe"
$docs = "$repo/docs"
New-Item -ItemType Directory -Force $docs | Out-Null

# --shot <out> --mode 2d|3d --model 0|1 --preset N --steps K --tilt deg --cam dist
& $exe --shot "$docs/01_stellar_2d.png"  --mode 2d --model 1 --preset 2 --steps 700 --cam 175
& $exe --shot "$docs/02_fern_2d.png"     --mode 2d --model 1 --preset 3 --steps 850 --cam 250
& $exe --shot "$docs/03_dendrite_3d.png" --mode 3d --model 1 --preset 0 --steps 400 --tilt 30 --cam 300
& $exe --shot "$docs/04_thick_3d.png"    --mode 3d --model 1 --preset 3 --steps 360 --tilt 48 --cam 300

Get-ChildItem $docs -Filter *.png | Select-Object Name, Length

# Snowflake 3D — ビルド補助スクリプト (Windows / MinGW)
# 使い方:  ./scripts/build.ps1            (Release ビルド)
#         ./scripts/build.ps1 -Run       (ビルド後に実行)
# Qt のパスは環境に合わせて -QtDir / -QtTools で上書きできます。
param(
    [string]$QtDir   = "C:/Qt/6.11.1/mingw_64",
    [string]$QtTools = "C:/Qt/Tools",
    [string]$BuildType = "Release",
    [switch]$Run
)

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot

$env:Path = "$QtTools/CMake_64/bin;$QtTools/Ninja;$QtTools/mingw1310_64/bin;$QtDir/bin;" + $env:Path

cmake -S $repo -B "$repo/build" -G Ninja `
    -DCMAKE_BUILD_TYPE=$BuildType `
    -DCMAKE_PREFIX_PATH="$QtDir" `
    -DCMAKE_CXX_COMPILER="$QtTools/mingw1310_64/bin/g++.exe"

cmake --build "$repo/build"

if ($Run) { & "$repo/build/Snowflake3D.exe" }

# Snowflake 3D — Qt 非依存コアのヘッドレステストをビルド・実行する。
# OpenMP(libgomp)のため MinGW の bin を PATH に通す。
param(
    [string]$QtTools = "C:/Qt/Tools"
)

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
$mingw = "$QtTools/mingw1310_64/bin"
$env:Path = "$mingw;" + $env:Path

$core = "$repo/src/core"
$out  = "$env:TEMP/snowflake_test_core.exe"

& "$mingw/g++.exe" -O2 -fopenmp -std=c++17 -I "$core" `
    "$repo/tests/test_core.cpp" `
    "$core/ReiterModel.cpp" `
    "$core/GravnerGriffeathModel.cpp" `
    -o $out

& $out
Remove-Item -Force $out -ErrorAction SilentlyContinue

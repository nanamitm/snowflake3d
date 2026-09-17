cmake_minimum_required(VERSION 3.21)

project(Snowflake3D VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Quick3D QuickControls2)
find_package(OpenMP)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(Snowflake3D
    src/main.cpp
    src/SimController.cpp
    src/SimController.h
    src/EnvProvider.h
    src/MeshBuilder.h
    src/InstanceColor.h
    src/HexPrismGeometry.cpp
    src/HexPrismGeometry.h
    src/SnowflakeInstancing.cpp
    src/SnowflakeInstancing.h
    src/Sim3DController.cpp
    src/Sim3DController.h
    src/Voxel3DInstancing.cpp
    src/Voxel3DInstancing.h
    src/core/CrystalModel.h
    src/core/ReiterModel.cpp
    src/core/ReiterModel.h
    src/core/GravnerGriffeathModel.cpp
    src/core/GravnerGriffeathModel.h
    src/core/Crystal3DModel.h
    src/core/Reiter3DModel.cpp
    src/core/Reiter3DModel.h
    src/core/GravnerGriffeath3DModel.cpp
    src/core/GravnerGriffeath3DModel.h
)

target_include_directories(Snowflake3D PRIVATE src)

qt_add_qml_module(Snowflake3D
    URI SnowflakeApp   # 実行ファイル名 Snowflake3D とモジュール用ディレクトリの衝突を避ける(Linux)
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

target_link_libraries(Snowflake3D PRIVATE
    Qt6::Quick
    Qt6::Quick3D
    Qt6::QuickControls2
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(Snowflake3D PRIVATE OpenMP::OpenMP_CXX)
endif()

# ===== WebAssembly 固有 =====
# シングルスレッド版 Qt でビルドするため OpenMP は無効(計算は逐次実行)。
# SharedArrayBuffer を使わないので COOP/COEP ヘッダも不要。
if(EMSCRIPTEN)
    # Qt wasm は DejaVu しか持たないので、日本語 UI 用のフォントを埋め込む。
    qt_add_resources(Snowflake3D "fonts"
        PREFIX "/fonts"
        BASE resources/fonts
        FILES resources/fonts/NotoSansJP-subset.ttf
    )

    set_target_properties(Snowflake3D PROPERTIES
        QT_WASM_INITIAL_MEMORY "128MB"   # Quick3D + 3D ボクセル格子の分
    )

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/wasm/index.html
                   ${CMAKE_CURRENT_BINARY_DIR}/index.html COPYONLY)
endif()

# Windows: GUI サブシステムにしてコンソールウィンドウを出さない
if(WIN32)
    set_target_properties(Snowflake3D PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

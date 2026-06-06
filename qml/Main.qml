import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick3D
import QtQuick3D.Helpers

ApplicationWindow {
    id: win
    width: 1320
    height: 860
    visible: true
    property bool mode3d: false
    title: mode3d ? "Snowflake 3D — full 3D crystal"
                  : "Snowflake 3D — " + sim.modelNames[sim.modelIndex]

    // ===== 3D ビュー =====
    View3D {
        id: view
        anchors.fill: parent

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.SkyBox
            lightProbe: Texture { source: "image://env/studio" }
            probeExposure: 1.1
            probeHorizon: 0.2
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        Node {
            id: originNode
            PerspectiveCamera { id: camera; z: 650; clipFar: 100000; clipNear: 1 }
        }

        DirectionalLight { eulerRotation.x: -35; eulerRotation.y: -45; brightness: 0.7 }

        // 氷マテリアル(2D/3D 共有)
        PrincipledMaterial {
            id: iceMaterial
            baseColor: "#eaf6ff"
            metalness: 0.0
            roughness: 0.10
            specularAmount: 1.0
            clearcoatAmount: 1.0
            clearcoatRoughnessAmount: 0.10
            transmissionFactor: 0.85
            thicknessFactor: 6.0
            attenuationColor: "#cfe8ff"
            attenuationDistance: 120.0
            indexOfRefraction: 1.31
        }

        Model {              // 2D (高さフィールド)
            visible: !win.mode3d
            geometry: sim.geometry
            materials: iceMaterial
        }
        Model {              // 完全 3D (ボクセル)
            visible: win.mode3d
            geometry: sim3d.geometry
            scale: Qt.vector3d(3, 3, 3)
            materials: iceMaterial
        }

        OrbitCameraController { anchors.fill: parent; origin: originNode; camera: camera }
    }

    // ===== 操作パネル =====
    Frame {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 12
        width: 340
        background: Rectangle { color: "#dd141a2b"; radius: 8 }

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            // 2D / 3D 切替
            Button {
                Layout.fillWidth: true
                text: win.mode3d ? "← 2D 表示へ戻る" : "完全 3D 表示へ →"
                onClicked: {
                    win.mode3d = !win.mode3d;
                    if (win.mode3d) sim.stop(); else sim3d.stop();
                }
            }

            // ---------- 2D パネル ----------
            ColumnLayout {
                visible: !win.mode3d
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8

                Label { text: "Step: " + sim.stepCount; color: "white"; font.pixelSize: 18; font.bold: true }

                Label { text: "Model"; color: "#9fb3d1"; font.pixelSize: 12 }
                ComboBox {
                    Layout.fillWidth: true
                    model: sim.modelNames
                    currentIndex: sim.modelIndex
                    onActivated: sim.modelIndex = currentIndex
                }

                RowLayout {
                    Layout.fillWidth: true
                    Button {
                        text: sim.running ? "⏸ Pause" : "▶ Play"
                        Layout.fillWidth: true
                        onClicked: sim.running ? sim.stop() : sim.start()
                    }
                    Button { text: "Step"; enabled: !sim.running; onClicked: sim.stepOnce() }
                    Button { text: "Reset"; onClicked: sim.reset() }
                }

                Label { text: "Presets"; color: "#9fb3d1"; font.pixelSize: 12 }
                Flow {
                    Layout.fillWidth: true
                    spacing: 6
                    Repeater {
                        model: sim.presetNames
                        Button { text: modelData; onClicked: sim.applyPreset(index) }
                    }
                }

                Label { text: "Parameters"; color: "#9fb3d1"; font.pixelSize: 12 }
                Repeater {
                    model: sim.params
                    ColumnLayout {
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: 0
                        Label {
                            text: modelData.name + ": " + Number(slider.value).toFixed(modelData.decimals)
                                  + (modelData.needsReset ? "  ※Reset" : "")
                            color: "white"; font.pixelSize: 12
                        }
                        Slider {
                            id: slider
                            Layout.fillWidth: true
                            from: modelData.min; to: modelData.max; value: modelData.value
                            onMoved: sim.setParam(modelData.index, value)
                        }
                    }
                }

                Label { text: "Thickness (誇張): " + Number(sim.thickness).toFixed(1); color: "white"; font.pixelSize: 12 }
                Slider { Layout.fillWidth: true; from: 1.0; to: 20.0; value: sim.thickness; onMoved: sim.thickness = value }
                Label { text: "Speed: " + sim.speed + " steps/tick"; color: "white"; font.pixelSize: 12 }
                Slider { Layout.fillWidth: true; from: 1; to: 20; value: sim.speed; onMoved: sim.speed = Math.round(value) }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.fillWidth: true
                    Button { text: "Export STL"; Layout.fillWidth: true; onClicked: stlDialog.open() }
                    Button { text: "Save PNG"; Layout.fillWidth: true; onClicked: pngDialog.open() }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Button { text: "Save Config"; Layout.fillWidth: true; onClicked: cfgSaveDialog.open() }
                    Button { text: "Load Config"; Layout.fillWidth: true; onClicked: cfgLoadDialog.open() }
                }
            }

            // ---------- 3D パネル ----------
            ColumnLayout {
                visible: win.mode3d
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8

                Label { text: "Step: " + sim3d.stepCount; color: "white"; font.pixelSize: 18; font.bold: true }
                Label {
                    Layout.fillWidth: true; wrapMode: Text.WordWrap
                    text: "六角プリズム格子の 3 次元モデル。GG を選ぶと樹枝状に枝分かれします。"
                    color: "#9fb3d1"; font.pixelSize: 11
                }

                Label { text: "3D Model"; color: "#9fb3d1"; font.pixelSize: 12 }
                ComboBox {
                    Layout.fillWidth: true
                    model: sim3d.modelNames
                    currentIndex: sim3d.modelIndex
                    onActivated: sim3d.modelIndex = currentIndex
                }

                RowLayout {
                    Layout.fillWidth: true
                    Button {
                        text: sim3d.running ? "⏸ Pause" : "▶ Play"
                        Layout.fillWidth: true
                        onClicked: sim3d.running ? sim3d.stop() : sim3d.start()
                    }
                    Button { text: "Step"; enabled: !sim3d.running; onClicked: sim3d.stepOnce() }
                    Button { text: "Reset"; onClicked: sim3d.reset() }
                }

                Label { text: "Presets"; color: "#9fb3d1"; font.pixelSize: 12 }
                Flow {
                    Layout.fillWidth: true; spacing: 6
                    Repeater {
                        model: sim3d.presetNames
                        Button { text: modelData; onClicked: sim3d.applyPreset(index) }
                    }
                }

                Label { text: "Parameters"; color: "#9fb3d1"; font.pixelSize: 12 }
                Repeater {
                    model: sim3d.params
                    ColumnLayout {
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: 0
                        Label {
                            text: modelData.name + ": " + Number(slider3.value).toFixed(modelData.decimals)
                                  + (modelData.needsReset ? "  ※Reset" : "")
                            color: "white"; font.pixelSize: 12
                        }
                        Slider {
                            id: slider3
                            Layout.fillWidth: true
                            from: modelData.min; to: modelData.max; value: modelData.value
                            onMoved: sim3d.setParam(modelData.index, value)
                        }
                    }
                }

                Label { text: "Speed: " + sim3d.speed + " steps/tick"; color: "white"; font.pixelSize: 12 }
                Slider { Layout.fillWidth: true; from: 1; to: 10; value: sim3d.speed; onMoved: sim3d.speed = Math.round(value) }

                Item { Layout.fillHeight: true }

                Button { text: "Save PNG"; Layout.fillWidth: true; onClicked: pngDialog.open() }
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: "#9fb3d1"; font.pixelSize: 11
                text: "ドラッグ=回転 / ホイール=ズーム。"
            }
        }
    }

    FileDialog {
        id: stlDialog
        fileMode: FileDialog.SaveFile
        nameFilters: ["STL files (*.stl)"]
        defaultSuffix: "stl"
        onAccepted: sim.exportStl(selectedFile)
    }

    FileDialog {
        id: pngDialog
        fileMode: FileDialog.SaveFile
        nameFilters: ["PNG files (*.png)"]
        defaultSuffix: "png"
        onAccepted: {
            var path = ("" + selectedFile).replace(/^file:\/\/\//, "");
            view.grabToImage(function(res) { res.saveToFile(path); });
        }
    }

    FileDialog {
        id: cfgSaveDialog
        fileMode: FileDialog.SaveFile
        nameFilters: ["JSON config (*.json)"]
        defaultSuffix: "json"
        onAccepted: sim.saveConfig(selectedFile)
    }

    FileDialog {
        id: cfgLoadDialog
        fileMode: FileDialog.OpenFile
        nameFilters: ["JSON config (*.json)"]
        onAccepted: sim.loadConfig(selectedFile)
    }
}

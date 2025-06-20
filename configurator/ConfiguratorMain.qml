import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Config 1.0

ApplicationWindow {
    visible: true
    width: 420
    height: 320
    title: qsTr("Section Configurator")

    ConfiguratorController {
        id: cfg
        onConfigSent: statusLabel.text = qsTr("Configuration sent")
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 10

        Repeater {
            id: rep
            model: 3
            delegate: RowLayout {
                spacing: 10
                property int sectionIdx: index + 1
                Label { text: "Section " + sectionIdx }
                CheckBox { id: hh; text: "HH" }
                CheckBox { id: ht; text: "HT" }
                CheckBox { id: th; text: "TH" }
                CheckBox { id: tt; text: "TT" }
                // expose values for controller
                property alias hhChecked: hh.checked
                property alias htChecked: ht.checked
                property alias thChecked: th.checked
                property alias ttChecked: tt.checked
            }
        }

        Button {
            text: qsTr("Send All")
            Layout.alignment: Qt.AlignHCenter
            onClicked: {
                for (let i = 0; i < rep.count; ++i) {
                    const row = rep.itemAt(i);
                    const delay = i*5; // 5 ms per index
                    Qt.createQmlObject('import QtQuick 2.0; Timer { interval: '+delay+'; repeat: false; running: true; onTriggered: cfg.sendConfig('+row.sectionIdx+','+row.hhChecked+','+row.htChecked+','+row.thChecked+','+row.ttChecked+') }', rep, "sendTimer"+i);
                }
            }
        }

        Label { id: statusLabel }
    }
} 

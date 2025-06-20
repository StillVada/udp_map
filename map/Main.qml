import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Section 1.0

ApplicationWindow {
    visible: true
    width: 400
    height: 300
    title: qsTr("Section Map")

    SectionController {
        id: controller
        sectionId: sectionSelect.currentIndex + 1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        ComboBox {
            id: sectionSelect
            model: ["1", "2", "3"]
            textRole: ""
            currentIndex: 0
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Image {
                id: headConn
                source: controller.headConnectionSource
                visible: source !== ""
                fillMode: Image.PreserveAspectFit
                Layout.preferredWidth: 120
                Layout.preferredHeight: 120
            }

            Image {
                id: sectionImage
                source: controller.imageSource
                fillMode: Image.PreserveAspectFit
                Layout.preferredWidth: 120
                Layout.preferredHeight: 120
            }

            Image {
                id: tailConn
                source: controller.tailConnectionSource
                visible: source !== ""
                fillMode: Image.PreserveAspectFit
                Layout.preferredWidth: 120
                Layout.preferredHeight: 120
            }
        }
    }
} 

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "QMessageBox"
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , udpSocket(new QUdpSocket(this))
{
    ui->setupUi(this);

    connect(udpSocket, &QUdpSocket::readyRead, this, &MainWindow::readPendingDatagrams);
    connect(ui->executeButton, &QPushButton::clicked, this, &MainWindow::ExecuteCommand);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::ExecuteCommand()
{

    QString host = ui->myIP->text();
    quint16 port = ui->myPort->text().toUShort();
    QString message = ui->myCommand->text();

    QByteArray datagram = message.toUtf8();
    qint64 bytesSent = udpSocket->writeDatagram(datagram, QHostAddress(host), port);

}
void MainWindow::readPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = udpSocket->receiveDatagram();
        QByteArray data = datagram.data();
        QString sender = datagram.senderAddress().toString();
        quint16 senderPort = datagram.senderPort();

    }
}

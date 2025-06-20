#include "SectionController.h"
#include <QDebug>
#include <cstring>

#define BROADCAST_PORT 12345

SectionController::SectionController(QObject* parent) : QObject(parent)
{
    m_socket.bind(BROADCAST_PORT, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    connect(&m_socket, &QUdpSocket::readyRead, this, &SectionController::processPendingDatagrams);
    updateImageSource();
}

int SectionController::sectionId() const { return m_sectionId; }

void SectionController::setSectionId(int id)
{
    if (m_sectionId == id) return;
    m_sectionId = id;
    m_sections.clear();
    m_myStatus.section_id = 0;
    m_headConnectionId = 0;
    m_tailConnectionId = 0;
    emit sectionIdChanged();
    
    m_imageSource = QString("qrc:/images/section_%1_head.png").arg(m_sectionId, 2, 10, QChar('0'));
    emit imageSourceChanged();
    m_headConnectionSource.clear();
    m_tailConnectionSource.clear();
    emit connectionChanged();
}

QString SectionController::imageSource() const { return m_imageSource; }
QString SectionController::headConnectionSource() const { return m_headConnectionSource; }
QString SectionController::tailConnectionSource() const { return m_tailConnectionSource; }

void SectionController::processPendingDatagrams()
{
    while (m_socket.hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(int(m_socket.pendingDatagramSize()));
        m_socket.readDatagram(datagram.data(), datagram.size());

        if (datagram.size() < sizeof(WrappedPacket)) continue;
        WrappedPacket pkt;
        std::memcpy(&pkt, datagram.constData(), sizeof(WrappedPacket));

        if (pkt.type != PacketType::Status) continue;
        
        ConnectionStatus status = pkt.payload.status;
        m_sections[status.section_id] = status;
        if (status.section_id == m_sectionId) {
            m_myStatus = status;
        }
    }

    if (m_myStatus.section_id == 0) return;
    int prevHeadId = m_headConnectionId;
    int prevTailId = m_tailConnectionId;
    int headPartnerId = 0;
    int tailPartnerId = 0;
    int head_partner_dist = 10000;
    int tail_partner_dist = 10000;

    for (const auto& [id, other] : m_sections) {
        if (id == m_sectionId) continue;
        int dist = std::abs(m_sectionId - id);

        if (dist < head_partner_dist && (m_myStatus.head_head && other.head_head || m_myStatus.head_tail && other.tail_head)) {
            head_partner_dist = dist;
            headPartnerId = id;
        }
        if (dist < tail_partner_dist && (m_myStatus.tail_head && other.head_tail || m_myStatus.tail_tail && other.tail_tail)) {
            tail_partner_dist = dist;
            tailPartnerId = id;
        }
    }
    m_headConnectionId = headPartnerId;
    m_tailConnectionId = tailPartnerId;


    const QString newImageSource = QString("qrc:/images/section_%1_head.png").arg(m_sectionId, 2, 10, QChar('0'));
    QString headPartnerSource = "";
    if (m_headConnectionId != 0) {
        bool useHeadImage = m_myStatus.head_tail;
        headPartnerSource = QString("qrc:/images/section_%1_%2.png")
            .arg(m_headConnectionId, 2, 10, QChar('0'))
            .arg(useHeadImage ? "head" : "tail");
    }

    QString tailPartnerSource = "";
    if (m_tailConnectionId != 0) {
        bool useHeadImage = m_myStatus.tail_head;
        tailPartnerSource = QString("qrc:/images/section_%1_%2.png")
            .arg(m_tailConnectionId, 2, 10, QChar('0'))
            .arg(useHeadImage ? "head" : "tail");
    }
    
    if (m_imageSource != newImageSource) {
        m_imageSource = newImageSource;
        emit imageSourceChanged();
    }
    
    if (m_headConnectionSource != headPartnerSource || m_tailConnectionSource != tailPartnerSource) {
        m_headConnectionSource = headPartnerSource;
        m_tailConnectionSource = tailPartnerSource;
        emit connectionChanged();
    }
}

void SectionController::updateImageSource()
{
    const QString newImageSource = QString("qrc:/images/section_%1_head.png")
                                    .arg(m_sectionId, 2, 10, QChar('0'));
    if (m_imageSource != newImageSource) {
        m_imageSource = newImageSource;
        emit imageSourceChanged();
    }
} 

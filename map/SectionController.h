#ifndef SECTIONCONTROLLER_H
#define SECTIONCONTROLLER_H

#include <QObject>
#include <QUdpSocket>
#include <map>

struct ConnectionStatus {
    int section_id;
    bool head_head;
    bool head_tail;
    bool tail_head;
    bool tail_tail;
};

enum class PacketType : uint8_t {
    Status = 1,
    Config = 2,
    NoConfig = 3
};

struct WrappedPacket {
    PacketType type;
    union {
        ConnectionStatus status;
        char dummy[sizeof(ConnectionStatus)];
    } payload;
};

class SectionController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int sectionId READ sectionId WRITE setSectionId NOTIFY sectionIdChanged)
    Q_PROPERTY(QString imageSource READ imageSource NOTIFY imageSourceChanged)
    Q_PROPERTY(QString headConnectionSource READ headConnectionSource NOTIFY connectionChanged)
    Q_PROPERTY(QString tailConnectionSource READ tailConnectionSource NOTIFY connectionChanged)

public:
    explicit SectionController(QObject* parent = nullptr);

    int sectionId() const;
    void setSectionId(int id);

    QString imageSource() const;
    QString headConnectionSource() const;
    QString tailConnectionSource() const;

signals:
    void sectionIdChanged();
    void imageSourceChanged();
    void connectionChanged();

private slots:
    void processPendingDatagrams();

private:
    void updateImageSource();

    int m_sectionId {1};
    bool m_forward {true};
    QString m_imageSource;
    QUdpSocket m_socket;

    QString m_headConnectionSource;
    QString m_tailConnectionSource;
    int m_headConnectionId {0};
    int m_tailConnectionId {0};
    bool m_headConnUseHead {true};
    bool m_tailConnUseHead {true};
    std::map<int, ConnectionStatus> m_sections;
    ConnectionStatus m_myStatus {0,false,false,false,false};
};

#endif // SECTIONCONTROLLER_H 

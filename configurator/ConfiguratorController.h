#ifndef CONFIGURATORCONTROLLER_H
#define CONFIGURATORCONTROLLER_H

#include <QObject>
#include <QUdpSocket>
#include <QAbstractSocket>
#include <cstdint>

struct ConfigPacket {
    int section_id;
    bool head_head;
    bool head_tail;
    bool tail_head;
    bool tail_tail;
};

enum class PacketType : uint8_t;
struct WrappedPacket;

class ConfiguratorController : public QObject
{
    Q_OBJECT
public:
    explicit ConfiguratorController(QObject* parent = nullptr);

    Q_INVOKABLE void sendConfig(int id, bool hh, bool ht, bool th, bool tt);

signals:
    void configSent();

private:
    QUdpSocket m_socket;
};

#endif // CONFIGURATORCONTROLLER_H 

#include "ConfiguratorController.h"
#include <cstdint>
#include <cstring>

#define BROADCAST_PORT 12345
#define BROADCAST_IP "255.255.255.255"

enum class PacketType : uint8_t {
    Status = 1,
    Config = 2,
    NoConfig = 3
};

struct WrappedPacket {
    PacketType type;
    union {
        ConfigPacket config;
        char dummy[sizeof(ConfigPacket)];
    } payload;
};

ConfiguratorController::ConfiguratorController(QObject* parent)
    : QObject(parent)
{
}

void ConfiguratorController::sendConfig(int id, bool hh, bool ht, bool th, bool tt)
{
    WrappedPacket pkt{};
    pkt.type = PacketType::Config;
    pkt.payload.config = {id, hh, ht, th, tt};

    QHostAddress addr(QStringLiteral(BROADCAST_IP));
    qint64 sent = m_socket.writeDatagram(reinterpret_cast<const char*>(&pkt), sizeof(pkt), addr, BROADCAST_PORT);
    if (sent == sizeof(pkt)) {
        emit configSent();
    }
} 

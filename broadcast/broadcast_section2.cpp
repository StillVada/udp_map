#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>
#include <chrono>
#include <thread>
#include <atomic>
#include <cstdint>
#include <cmath>

#define BROADCAST_PORT 12345
#define BROADCAST_IP "255.255.255.255"
#define STATUS_INTERVAL 5

struct ConnectionStatus {
    int section_id;
    bool head_head;
    bool head_tail;
    bool tail_head;
    bool tail_tail;
};

struct ConfigPacket {
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
        ConfigPacket config;
    } payload;
};

class SectionConnectionAnalyzer {
private:
    int my_id;
    std::map<int, ConnectionStatus> sections;
    int head_connected_to;
    int tail_connected_to;
    
public:
    SectionConnectionAnalyzer(int id) : my_id(id), head_connected_to(0), tail_connected_to(0) {}
    
    void updateSection(const ConnectionStatus& status) {
        if (status.section_id != my_id) {
            sections[status.section_id] = status;
        }
    }

    void logStatus() {
        std::cout << "Статус | Секция " << my_id 
                  << " | Голова: " << (head_connected_to ? std::to_string(head_connected_to) : "-")
                  << " | Хвост: " << (tail_connected_to ? std::to_string(tail_connected_to) : "-")
                  << std::endl;
    }
    
    bool analyze(const ConnectionStatus& my_inputs) {
        int prev_head = head_connected_to;
        int prev_tail = tail_connected_to;

        head_connected_to = 0;
        tail_connected_to = 0;
        int head_partner_dist = 10000;
        int tail_partner_dist = 10000;

        for (const auto& [id, other] : sections) {
            if ((my_inputs.head_head && other.head_head) ||
                (my_inputs.head_tail && other.tail_head)) {
                
                int dist = std::abs(my_id - id);
                if (dist < head_partner_dist) {
                    head_partner_dist = dist;
                    head_connected_to = id;
                }
            }

            if ((my_inputs.tail_head && other.head_tail) ||
                (my_inputs.tail_tail && other.tail_tail)) {

                int dist = std::abs(my_id - id);
                if (dist < tail_partner_dist) {
                    tail_partner_dist = dist;
                    tail_connected_to = id;
                }
            }
        }

        return (prev_head != head_connected_to) || (prev_tail != tail_connected_to);
    }

    int head() const { return head_connected_to; }
    int tail() const { return tail_connected_to; }
};

int main() {
    int section_id = std::atoi(std::getenv("SECTION_ID"));
    if (section_id == 0) {
        std::cerr << "Не указан SECTION_ID" << std::endl;
        return 1;
    }

    SectionConnectionAnalyzer analyzer(section_id);

    ConnectionStatus my_inputs = {section_id, 0,0,0,0};
    std::atomic<bool> configured{false};

    std::cout << "Секция " << section_id << " запущена. Ожидаю конфигурацию..." << std::endl;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#ifdef SO_REUSEPORT
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
#endif
    
    int broadcastEnable = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

    struct sockaddr_in broadcastAddr = {};
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(BROADCAST_PORT);
    inet_pton(AF_INET, BROADCAST_IP, &broadcastAddr.sin_addr);

    struct sockaddr_in listenAddr = {};
    listenAddr.sin_family = AF_INET;
    listenAddr.sin_port = htons(BROADCAST_PORT);
    listenAddr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (struct sockaddr*)&listenAddr, sizeof(listenAddr));

    auto last_send_time = std::chrono::steady_clock::now();
    
    while (true) {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_send_time).count() >= STATUS_INTERVAL) {
            WrappedPacket out{};
            if (configured) {
                out.type = PacketType::Status;
                out.payload.status = my_inputs;
            } else {
                out.type = PacketType::NoConfig;
                out.payload.status.section_id = section_id;
            }

            ssize_t rc = sendto(sock, &out, sizeof(out), 0,
                               (const sockaddr*)&broadcastAddr, sizeof(broadcastAddr));

            if (rc >= 0) {
                if (configured) {
                    std::cout << "Отправил статус | Секция " << section_id
                              << " | Голова: " << (analyzer.head() ? std::to_string(analyzer.head()) : "-")
                              << " | Хвост: " << (analyzer.tail() ? std::to_string(analyzer.tail()) : "-")
                              << std::endl;
                }
            }
            last_send_time = now;
        }

        WrappedPacket recvPkt;
        struct sockaddr_in senderAddr;
        socklen_t senderLen = sizeof(senderAddr);
        
        if (recvfrom(sock, &recvPkt, sizeof(recvPkt), MSG_DONTWAIT,
                    (struct sockaddr*)&senderAddr, &senderLen) > 0) {
            if (recvPkt.type == PacketType::Config && recvPkt.payload.config.section_id == section_id && !configured) {
                const auto& cfg = recvPkt.payload.config;
                my_inputs = {cfg.section_id, cfg.head_head, cfg.head_tail, cfg.tail_head, cfg.tail_tail};
                configured = true;
                std::cout << "Принял конфигурацию: HH=" << my_inputs.head_head << " HT=" << my_inputs.head_tail
                          << " TH=" << my_inputs.tail_head << " TT=" << my_inputs.tail_tail << std::endl;
            } else if (recvPkt.type == PacketType::Status) {
                const ConnectionStatus& received = recvPkt.payload.status;
                if (received.section_id != section_id) {
                    analyzer.updateSection(received);
                    if (analyzer.analyze(my_inputs)) {
                        analyzer.logStatus();
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

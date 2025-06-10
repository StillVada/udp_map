#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <vector>
#include <unordered_set>
#include <functional>

#define CONFIG_PORT (getenv("CONFIG_PORT") ? atoi(getenv("CONFIG_PORT")) : 5001)
#define HEAD_PORT   (getenv("HEAD_PORT")   ? atoi(getenv("HEAD_PORT"))   : 6000)
#define TAIL_PORT   (getenv("TAIL_PORT")   ? atoi(getenv("TAIL_PORT"))   : 6001)
#define BUFFER_SIZE 1024
#define LOCOMOTIVE_NUMBER (getenv("LOCOMOTIVE_NUMBER") ? atoi(getenv("LOCOMOTIVE_NUMBER")) : 1)

// Forward declarations
std::string detect_connection_type(int port);
std::vector<std::pair<std::string, std::string>> getAllConnections();

// Global connections container
std::vector<std::pair<std::string, std::string>> connections;

// Структура сокета
struct SocketInfo {
    int fd;
    int port;
    bool is_connected;  // true = клиент, false = сервер
    sockaddr_in remote_addr;
};

// Проверка типа устройства подключения
std::string detect_connection_type(int port) {
    return (port % 2 == 1) ? "1" : "0"; // Нечётные = TAIL (0), Чётные = HEAD (1)
}

// Функция для определения номера секции по порту
int get_section_number(int port) {
    return (port - 6000) / 2 + 1;  // +1 чтобы начинать с 1 вместо 0
}

// Функция для получения номера секции по порту и приведению её к двузначному числу
std::string format_section_id(int section_port) {
    int section_num = get_section_number(section_port);
    if (section_num < 10) {
        return "0" + std::to_string(section_num);
    }
    return std::to_string(section_num);
}

// Функция для получения всех соединений между сокетами
std::vector<std::pair<std::string, std::string>> getAllConnections() {
    std::vector<std::pair<std::string, std::string>> all_connections;
    std::unordered_set<std::string> visited;

    // Функция для рекурсивного обхода
    std::function<void(const std::string&)> traverse = [&](const std::string& current) {
        // Проверка обрабатывалась ли текущая секция
        if (visited.count(current)) 
            return;
        visited.insert(current);

        for (const auto& [from, to] : connections) {
            if (from == current) {
                all_connections.emplace_back(from, to);
                traverse(to); // Идём к следующей секции
            }
        }
    };

    // Находим все стартовые точки (секции без входящих соединений)
    std::unordered_set<std::string> has_incoming;
    for (const auto& [from, to] : connections) {
        has_incoming.insert(to);
    }

    for (const auto& [from, to] : connections) {
        if (!has_incoming.count(from)) {
            traverse(from); // Начинаем обход с корневых секций
        }
    }

    return all_connections;
}

// Инициализация сокета в режиме сервера
void initServerSocket(SocketInfo& sock, int port) {
    sock.fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock.fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sock.fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Socket bind failed");
        close(sock.fd);
        exit(EXIT_FAILURE);
    }

    sock.port = port;
    sock.is_connected = false;  // Режим сервера
    std::cout << "Сокет слушает порт " << port << std::endl;
}

// Переключение сокета в режим клиента
void switchToClientMode(SocketInfo& sock, const sockaddr_in& remote_addr) {
    sock.is_connected = true;
    sock.remote_addr = remote_addr;

    // Формируем сообщение о подключении
    std::string msg = "Подключен к порту " + std::to_string(ntohs(remote_addr.sin_port));
    
    // Отправляем подтверждение
    sendto(sock.fd, msg.c_str(), msg.size(), 0,
          (sockaddr*)&remote_addr, sizeof(remote_addr));
    
    std::cout << "Сокет переключён в режим клиента (" << msg << ")" << std::endl;
}

void handleSockets(SocketInfo& config_sock, SocketInfo& head_sock, SocketInfo& tail_sock) {
    char buffer[BUFFER_SIZE];
    fd_set read_fds;
    
    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(config_sock.fd, &read_fds);
        if (!head_sock.is_connected) FD_SET(head_sock.fd, &read_fds);
        if (!tail_sock.is_connected) FD_SET(tail_sock.fd, &read_fds);

        // Вычисление максимального файлового дискриптора
        int max_fd = config_sock.fd;
        if (!head_sock.is_connected && head_sock.fd > max_fd) max_fd = head_sock.fd;
        if (!tail_sock.is_connected && tail_sock.fd > max_fd) max_fd = tail_sock.fd;

        timeval timeout{1, 0};
        // Проверяется пришло ли что-нибудь в сокеты (функция select проверяет с 0 до максимального файлового дискриптора)
        int activity = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);

        if (activity < 0) {
            perror("Ошибка select");
            break;
        }

        // Обработка команды на config_sock
        if (FD_ISSET(config_sock.fd, &read_fds)) {
            sockaddr_in remote_addr{};
            socklen_t addr_len = sizeof(remote_addr);
            int bytes = recvfrom(config_sock.fd, buffer, BUFFER_SIZE, 0,
                               (sockaddr*)&remote_addr, &addr_len);

            if (bytes > 0) {
                buffer[bytes] = '\0';
                std::cout << "Получена команда: " << buffer << std::endl;

                // Обработка запроса соединений соседей
                if (strncmp(buffer, "GET_CONNECTIONS", 15) == 0) {
                    std::string response;
                    for (const auto& [from, to] : connections) {
                        response += from + "->" + to + "|";
                    }
                    sendto(config_sock.fd, response.c_str(), response.size(), 0,
                         (sockaddr*)&remote_addr, sizeof(remote_addr));
                    continue;
                }

                // Обработка запроса получения всех соединений
                else if (strncmp(buffer, "GET_ALL_CONNECTIONS", 18) == 0) {
                    auto all_conns = getAllConnections();
                    std::string response;
                    for (const auto& [from, to] : all_conns) {
                        response += from + "->" + to + "|";
                    }
                    char client_ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &remote_addr.sin_addr, client_ip, sizeof(client_ip));
                    std::cout << "Отправляю ответ клиенту " << client_ip << ":" << ntohs(remote_addr.sin_port) << std::endl;
                    sendto(config_sock.fd, response.c_str(), response.size(), 0,
                        (sockaddr*)&remote_addr, sizeof(remote_addr));
                    continue;
                }

                // Обработка запроса подключения головы
                else if (strncmp(buffer, "CONNECT_HEAD", 12) == 0 && !head_sock.is_connected) {
                    int port = atoi(buffer + 13);
                    if (port > 0 && port <= 65535) {
                        remote_addr.sin_port = htons(port);
                        switchToClientMode(head_sock, remote_addr);
                    }
                }

                // Обработка запроса подключения хвоста
                else if (strncmp(buffer, "CONNECT_TAIL", 12) == 0 && !tail_sock.is_connected) {
                    int port = atoi(buffer + 13);
                    if (port > 0 && port <= 65535) {
                        remote_addr.sin_port = htons(port);
                        switchToClientMode(tail_sock, remote_addr);
                    }
                }
            }
            else if (bytes == 0) {
                // UDP никогда не возвращает 0! Это может быть ошибка логики
                continue; // Просто продолжаем цикл
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // Таймаут - нормальная ситуация
                    continue;
                }
                perror("recvfrom error");
                continue; // Продолжаем работу после ошибки
            }
        }

        // Обработка данных на head_sock (входящие подключения к head)
        if (!head_sock.is_connected && FD_ISSET(head_sock.fd, &read_fds)) {
            sockaddr_in client_addr{};
            socklen_t addr_len = sizeof(client_addr);
            int bytes = recvfrom(head_sock.fd, buffer, BUFFER_SIZE, 0,
                                (sockaddr*)&client_addr, &addr_len);

            if (bytes > 0) {
                buffer[bytes] = '\0';
                
                // Формируется id формата XYZW, где X - номер локомотива; YZ - Номер секции; W - тип устройства 0 - голова, 1 - хвост
                int remote_port = ntohs(client_addr.sin_port);

                // id удаленного устройства
                std::string remote_id = std::to_string(LOCOMOTIVE_NUMBER) + 
                                       format_section_id(remote_port) + 
                                       detect_connection_type(remote_port);
                
                // id текущего устройства
                std::string local_id = std::to_string(LOCOMOTIVE_NUMBER) + 
                                       format_section_id(HEAD_PORT) +
                                       "0";
                
                connections.emplace_back(remote_id, local_id);
                
                std::cout << "HEAD: Добавлено соединение " 
                << remote_id << "->" << local_id << std::endl;
            }
        }
        
        // Обработка данных на tail_sock (входящие подключения к tail)
        if (!tail_sock.is_connected && FD_ISSET(tail_sock.fd, &read_fds)) {
            sockaddr_in client_addr{};
            socklen_t addr_len = sizeof(client_addr);
            int bytes = recvfrom(tail_sock.fd, buffer, BUFFER_SIZE, 0,
                (sockaddr*)&client_addr, &addr_len);
                
            if (bytes > 0) {
                buffer[bytes] = '\0';

                // Формируется id формата XYZW, где X - номер локомотива; YZ - Номер секции; W - тип устройства 0 - голова, 1 - хвост
                int remote_port = ntohs(client_addr.sin_port);

                // id удаленного устройства
                std::string remote_id = std::to_string(LOCOMOTIVE_NUMBER) + 
                                    format_section_id(remote_port) + 
                                    detect_connection_type(remote_port);
                
                // id текущего устройства
                std::string local_id = std::to_string(LOCOMOTIVE_NUMBER) + 
                                    format_section_id(TAIL_PORT) + 
                                    "1";
                
                connections.emplace_back(remote_id, local_id);
            
                std::cout << "TAIL: Добавлено соединение " 
                        << remote_id << "->" << local_id << std::endl;
            }
        }
    }
}

int main() {
    SocketInfo config_sock, head_sock, tail_sock;

    // Инициализация всех сокетов в режиме сервера
    initServerSocket(config_sock, CONFIG_PORT);
    initServerSocket(head_sock, HEAD_PORT);
    initServerSocket(tail_sock, TAIL_PORT);

    // Основной цикл обработки
    handleSockets(config_sock, head_sock, tail_sock);

    // Закрытие сокетов (необязательно для UDP, но полезно)
    close(config_sock.fd);
    close(head_sock.fd);
    close(tail_sock.fd);

    return 0;
}

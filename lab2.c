#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 8080
#define MAX_CLIENTS 1

volatile sig_atomic_t wasSigHup = 0;

void sigHupHandler(int sig) {
    wasSigHup = 1;
}

int main() {
    int server_fd, client_fd = -1;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    fd_set read_fds;
    sigset_t blocked_mask, orig_mask;
    struct sigaction sa;

    // Создание сокета
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Настройка адреса сервера
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Привязка сокета к адресу
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Начало прослушивания
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Регистрация обработчика сигнала
    sa.sa_handler = sigHupHandler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGHUP, &sa, NULL) == -1) {
        perror("sigaction failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Блокировка сигнала SIGHUP
    sigemptyset(&blocked_mask);
    sigaddset(&blocked_mask, SIGHUP);
    if (sigprocmask(SIG_BLOCK, &blocked_mask, &orig_mask) == -1) {
        perror("sigprocmask failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is running and listening on port %d\n", PORT);

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        int max_fd = server_fd;

        if (client_fd != -1) {
            FD_SET(client_fd, &read_fds);
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
        }

        // Вызов pselect()
        int activity = pselect(max_fd + 1, &read_fds, NULL, NULL, NULL, &orig_mask);
        if (activity == -1 && errno == EINTR) {
            if (wasSigHup) {
                printf("Received SIGHUP signal\n");
                wasSigHup = 0;
            }
            continue;
        } else if (activity == -1) {
            perror("pselect failed");
            break;
        }

        // Обработка новых соединений
        if (FD_ISSET(server_fd, &read_fds)) {
            int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            if (new_socket < 0) {
                perror("accept failed");
                continue;
            }

            if (client_fd == -1) {
                client_fd = new_socket;
                printf("New connection accepted, client_fd = %d\n", client_fd);
            } else {
                printf("New connection rejected, closing socket\n");
                close(new_socket);
            }
        }

        // Обработка данных от клиента
        if (client_fd != -1 && FD_ISSET(client_fd, &read_fds)) {
            char buffer[1024] = {0};
            int valread = read(client_fd, buffer, sizeof(buffer));
            if (valread == 0) {
                printf("Client disconnected, closing client_fd = %d\n", client_fd);
                close(client_fd);
                client_fd = -1;
            } else if (valread < 0) {
                perror("read failed");
                close(client_fd);
                client_fd = -1;
            } else {
                printf("Received %d bytes from client\n", valread);
            }
        }
    }

    // Закрытие сокета сервера
    close(server_fd);
    return 0;
}
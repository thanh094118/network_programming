#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX_CLIENTS  10
#define BUF_SIZE     512
#define NAME_SIZE    64
#define DB_FILE      "user.txt"
#define OUT_FILE     "out.txt"

#define STATE_WAIT_USER   0
#define STATE_WAIT_PASS   1
#define STATE_LOGGED_IN   2

struct Client {
    int  fd;
    int  state;
    char username[NAME_SIZE];
};

void send_str(int fd, const char *msg) {
    send(fd, msg, strlen(msg), 0);
}

int check_login(const char *username, const char *password) {
    FILE *fp = fopen(DB_FILE, "r");
    if (fp == NULL) return 0;

    char line[BUF_SIZE];
    char db_user[NAME_SIZE];
    char db_pass[NAME_SIZE];

    while (fgets(line, sizeof(line), fp) != NULL) {

        int len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        if (sscanf(line, "%63s %63s", db_user, db_pass) != 2) {
            continue;
        }

        if (strcmp(username, db_user) == 0 &&
            strcmp(password, db_pass) == 0) {
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

void exec_and_send(int fd, const char *cmd) {

    char full_cmd[BUF_SIZE * 2];
    snprintf(full_cmd, sizeof(full_cmd), "%s > %s 2>&1", cmd, OUT_FILE);

    int ret = system(full_cmd);

    FILE *fp = fopen(OUT_FILE, "r");
    if (fp == NULL) {
        send_str(fd, "ERROR\n");
        return;
    }

    char buf[BUF_SIZE];
    int  sent_anything = 0;

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        send_str(fd, buf);
        sent_anything = 1;
    }
    fclose(fp);

    if (!sent_anything) {
        if (ret == 0) {
            send_str(fd, "DONE\n");
        } else {
            send_str(fd, "ERROR\n");
        }
    }

    send_str(fd, "\n> ");
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);

    int listener = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons((uint16_t)port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(listener, 5);

    printf("Connected to port %d...\n", port);
    printf("Use database: %s\n", DB_FILE);

    struct Client clients[MAX_CLIENTS];
    int           num_clients = 0;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
    }

    while (1) {

        fd_set read_fds;
        FD_ZERO(&read_fds);

        FD_SET(listener, &read_fds);
        int max_fd = listener;

        for (int i = 0; i < num_clients; i++) {
            FD_SET(clients[i].fd, &read_fds);
            if (clients[i].fd > max_fd) {
                max_fd = clients[i].fd;
            }
        }

        int ret = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (ret < 0) break;

        if (FD_ISSET(listener, &read_fds)) {

            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            int client_fd = accept(listener,
                                   (struct sockaddr *)&client_addr,
                                   &client_len);

            if (client_fd >= 0 && num_clients < MAX_CLIENTS) {

                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr,
                          client_ip, sizeof(client_ip));
                printf("New client: %s (fd=%d)\n",
                       client_ip, client_fd);

                clients[num_clients].fd    = client_fd;
                clients[num_clients].state = STATE_WAIT_USER;
                memset(clients[num_clients].username, 0, NAME_SIZE);
                num_clients++;

                send_str(client_fd,"Username: ");
            }
        }

        for (int i = 0; i < num_clients; i++) {

            if (!FD_ISSET(clients[i].fd, &read_fds)) {
                continue;
            }

            char buf[BUF_SIZE];
            int  n = recv(clients[i].fd, buf, BUF_SIZE - 1, 0);

            if (n <= 0) {
                printf("Pick fd=%d (%s).\n",
                       clients[i].fd,
                       strlen(clients[i].username) > 0
                           ? clients[i].username : "unSigned");
                close(clients[i].fd);
                clients[i] = clients[--num_clients];
                i--;
                continue;
            }

            buf[n] = '\0';

            while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r')) {
                buf[--n] = '\0';
            }

            if (clients[i].state == STATE_WAIT_USER) {

                strncpy(clients[i].username, buf, NAME_SIZE - 1);
                clients[i].state = STATE_WAIT_PASS;
                send_str(clients[i].fd, "Password: ");

            } else if (clients[i].state == STATE_WAIT_PASS) {

                if (check_login(clients[i].username, buf)) {

                    printf("DONE: %s (fd=%d)\n",
                           clients[i].username, clients[i].fd);
                    clients[i].state = STATE_LOGGED_IN;

                    char welcome[BUF_SIZE];
                    snprintf(welcome, sizeof(welcome),
                             "DONE %s\n"
                             "Nhap lenh:\n> ",
                             clients[i].username);
                    send_str(clients[i].fd, welcome);

                } else {

                    printf("ERROR: %s (fd=%d)\n",
                           clients[i].username, clients[i].fd);
                    memset(clients[i].username, 0, NAME_SIZE);
                    clients[i].state = STATE_WAIT_USER;
                    send_str(clients[i].fd,
                             "ERROR\n"
                             "Username: ");
                }

            } else if (clients[i].state == STATE_LOGGED_IN) {

                if (strcmp(buf, "exit") == 0 || strcmp(buf, "quit") == 0) {
                    send_str(clients[i].fd, "Tam biet!\n");
                    printf(" %s Out (fd=%d)\n",
                           clients[i].username, clients[i].fd);
                    close(clients[i].fd);
                    clients[i] = clients[--num_clients];
                    i--;
                    continue;
                }

                printf(" %s running: %s\n",
                       clients[i].username, buf);

                exec_and_send(clients[i].fd, buf);
            }
        }
    }

    close(listener);
    return 0;
}

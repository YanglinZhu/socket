#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sqlite3.h>

#define PORT 6666
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100

int client_sockets[MAX_CLIENTS];
sqlite3 *db;

void *handle_client(void *arg);
int authenticate_user(const char *username, const char *password);

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    char buffer[BUFFER_SIZE];
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(client_socket, buffer, BUFFER_SIZE);
        if (valread <= 0) {
            break;
        }

        if (strncmp(buffer, "login ", 6) == 0) {
            char username[50], password[50];
            sscanf(buffer + 6, "%s %s", username, password);
            if (authenticate_user(username, password)) {
                send(client_socket, "Login successful", strlen("Login successful"), 0);
            } else {
                send(client_socket, "Login failed", strlen("Login failed"), 0);
            }
        } else {
            // 广播消息给所有客户端
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] != 0) {
                    send(client_sockets[i], buffer, strlen(buffer), 0);
                }
            }
        }
    }

    close(client_socket);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == client_socket) {
            client_sockets[i] = 0;
        }
    }
    return NULL;
}

int authenticate_user(const char *username, const char *password) {
    char *err_msg = 0;
    sqlite3_stmt *res;
    const char *sql = "SELECT * FROM users WHERE username = ? AND password = ?";
    
    if (sqlite3_prepare_v2(db, sql, -1, &res, 0) != SQLITE_OK) {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    
    sqlite3_bind_text(res, 1, username, -1, 0);
    sqlite3_bind_text(res, 2, password, -1, 0);
    
    int step = sqlite3_step(res);
    if (step == SQLITE_ROW) {
        sqlite3_finalize(res);
        return 1;
    } else {
        sqlite3_finalize(res);
        return 0;
    }
}

int main(int argc, char *argv[]) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // 初始化客户端socket数组
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }

    // 创建服务器socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 绑定socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 监听连接
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 打开SQLite数据库
    if (sqlite3_open("chat.db", &db)) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return(0);
    }

    // 创建用户表
    char *sql = "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, username TEXT, password TEXT);";
    char *err_msg = 0;
    if (sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 0;
    }

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept failed");
            continue;
        }

        // 将新连接加入到客户端socket数组中
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] == 0) {
                client_sockets[i] = new_socket;
                pthread_t thread_id;
                pthread_create(&thread_id, NULL, handle_client, &new_socket);
                break;
            }
        }
    }

    sqlite3_close(db);
    close(server_fd);

    return 0;
}

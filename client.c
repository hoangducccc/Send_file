#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define PORT 8080
#define SERVER_IP "192.168.237.145"

void error(char *message) {
    perror(message);
    exit(1);
}

typedef struct {
    int num_files;
    char filenames[100][256];
} SendFileList;

void receiveFile(int socket_fd, char *filename) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s", filename);

    FILE *file = fopen(filepath, "wb");
    if (file == NULL) {
        error("Không thể tạo tệp");
    }

    char buffer[1024];
    ssize_t bytesRead;

    while (1) {
        bytesRead = recv(socket_fd, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            break;
        }
        fwrite(buffer, 1, bytesRead, file);
    }

    fclose(file);
    printf("Đã nhận tệp %s\n", filename);
}

void listFiles(int socket_fd) {
    char request[] = "list";
    send(socket_fd, request, strlen(request) + 1, 0);

    SendFileList receiveList;
    if (recv(socket_fd, &receiveList, sizeof(receiveList), 0) == -1) {
        error("Lỗi khi nhận danh sách tệp");
    }

    printf("Danh sách tệp trong thư mục 'files' của server:\n");
    for (int i = 0; i < receiveList.num_files; i++) {
        printf("%s\n", receiveList.filenames[i]);
    }
}

void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int main() {
    int socket_fd;
    struct sockaddr_in server_addr;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);


    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        error("Không thể kết nối đến server");
    }


    while (1) {
        // Nhập lựa chọn
        printf("Lựa chọn:\n");
        printf("1. Xem danh sách tên tệp trong thư mục 'files' của server\n");
        printf("2. Tải tệp từ server\n");
        printf("3. Thoát\n");

        int choice;
        scanf("Vui lòng nhập lựa chọn của bạn: %d", &choice);
        clearInputBuffer();

        switch (choice) {
            case 1: {
                socket_fd = socket(AF_INET, SOCK_STREAM, 0);
                connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
                listFiles(socket_fd);
                break;
                }
            case 2: {
                int t = 0;
                char request[] = "list";
                send(socket_fd, request, strlen(request) + 1, 0);
                SendFileList receiveList;
                recv(socket_fd, &receiveList, sizeof(receiveList), 0);
                socket_fd = socket(AF_INET, SOCK_STREAM, 0);
                connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
                while (t == 0) {
                    char filename[256];
                    printf("Nhập tên file cần tải hoặc 'quit' để thoát: ");
                    fgets(filename, sizeof(filename), stdin);

                    size_t len = strlen(filename);
                    if (len > 0 && filename[len - 1] == '\n') {
                        filename[len - 1] = '\0';
                    }

                    if (strcmp(filename, "quit") == 0) {
                        break;
                    }

                    int find = 0;
                    for (int i = 0; i < receiveList.num_files; i++) {
                        if (strcmp(receiveList.filenames[i], filename) == 0) {
                            find = 1;
                            break;
                        }
                    }

                    if (find) {
                        send(socket_fd, filename, strlen(filename) + 1, 0);
                        receiveFile(socket_fd, filename);
                        t = 1;
                    } else {
                        printf("Tệp không tồn tại. Vui lòng thử lại.\n");
                    }
                }
                break;
            }

            case 3:
                close(socket_fd);
                exit(0);
                break;
            default:
                printf("Lựa chọn không hợp lệ. Vui lòng thử lại.\n");
                break;
        }
    }

    close(socket_fd);

    return 0;
}

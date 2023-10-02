#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"

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
    snprintf(filepath, sizeof(filepath), "received_%s", filename);

    FILE *file = fopen(filepath, "wb"); // Thay đổi chế độ mở tệp thành "wb"
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

    // Tạo socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        error("Không thể tạo socket");
    }

    // Khởi tạo thông tin của server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // Kết nối đến server
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
        scanf("%d", &choice);
        clearInputBuffer(); // Consume newline characters

        switch (choice) {
            case 1:
                listFiles(socket_fd);
                break;
            case 2: {
                // Nhập tên tệp cần tải
                char filename[256];
                printf("Nhập tên tệp cần tải hoặc 'quit' để thoát: ");
                fgets(filename, sizeof(filename), stdin);

                // Loại bỏ ký tự newline nếu có
                size_t len = strlen(filename);
                if (len > 0 && filename[len - 1] == '\n') {
                    filename[len - 1] = '\0';
                }

                // Kiểm tra nếu người dùng muốn thoát
                if (strcmp(filename, "quit") == 0) {
                    break;
                }

                // Gửi tên tệp đến server
                send(socket_fd, filename, strlen(filename) + 1, 0);

                // Nhận tệp từ server
                receiveFile(socket_fd, filename);
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

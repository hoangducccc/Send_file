#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>


#define PORT 8080
#define FILE_PATH "files/"

void error(char *message) {
    perror(message);
    exit(1);
}

typedef struct {
    int num_files;
    char filenames[100][256];
} SendFileList;

void sendFile(int client_socket, char *filename) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s%s", FILE_PATH, filename);

    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        error("Không thể mở tệp");
    }

    char buffer[1024];
    ssize_t bytesRead;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(client_socket, buffer, bytesRead, 0) == -1) {
            error("Lỗi khi gửi dữ liệu");
        }
    }

    fclose(file);
}

void listFiles(int client_socket) {
    struct dirent *de;
    DIR *dr = opendir(FILE_PATH);

    if (dr == NULL) {
        error("Không thể mở thư mục");
    }

    SendFileList fileList;
    fileList.num_files = 0;

    while ((de = readdir(dr)) != NULL) {
        if (de->d_type == DT_REG) {
            strncpy(fileList.filenames[fileList.num_files], de->d_name, sizeof(fileList.filenames[fileList.num_files]));
            fileList.num_files++;
        }
    }

    closedir(dr);

    if (send(client_socket, &fileList, sizeof(fileList), 0) == -1) {
        error("Lỗi khi gửi danh sách tệp");
    }
}

void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void *client_handler(void *arg) {
    int client_socket = *((int *)arg);
    char buffer[1024];
    ssize_t bytesRead;

    // Nhận yêu cầu từ client
    bytesRead = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytesRead <= 0) {
        close(client_socket);
        pthread_exit(NULL);
    }

    // Loại bỏ ký tự newline (nếu có)
    if (buffer[bytesRead - 1] == '\n') {
        buffer[bytesRead - 1] = '\0';
    }

    if (strcmp(buffer, "list") == 0) {
        listFiles(client_socket);
    } else {
        // Gửi tệp cho client
        sendFile(client_socket, buffer);
    }

    close(client_socket);
    pthread_exit(NULL);
}

int main() {
    int socket_fd, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Tạo thư mục "files" để lưu trữ các tệp
    // mkdir(FILE_PATH, 0777);

    // Tạo socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        error("Không thể tạo socket");
    }

    // Khởi tạo thông tin của server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Ràng buộc socket đến địa chỉ và cổng
    if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        error("Không thể ràng buộc socket");
    }

    // Lắng nghe kết nối
    if (listen(socket_fd, 5) == -1) {
        error("Không thể lắng nghe kết nối");
    }

    printf("Đang chờ kết nối...\n");

    // Chấp nhận và xử lý kết nối từ client
    while (1) {
        client_socket = accept(socket_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            error("Không thể chấp nhận kết nối");
        }

        // Tạo một luồng mới để xử lý client
        pthread_t thread;
        if (pthread_create(&thread, NULL, client_handler, &client_socket) != 0) {
            error("Không thể tạo luồng mới");
        }

        // Giải phóng tài nguyên luồng sau khi hoàn thành
        pthread_detach(thread);
    }

    close(socket_fd);

    return 0;
}

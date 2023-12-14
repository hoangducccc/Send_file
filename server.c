#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>

#define PORT 8081
#define FILE_PATH "files/"
#define PASSWORD "bungbu"

void error(char *message)
{
    perror(message);
    exit(1);
}

typedef struct
{
    int num_files;
    char filenames[100][256];
} SendFileList;

void sendFile(int client_socket, char *filename)
{
    char filepath[256];
    size_t bytesRead;
    // printf("%s", filename);
    snprintf(filepath, sizeof(filepath), "%s%s", FILE_PATH, filename);

    FILE *file = fopen(filepath, "rb"); // Mở file để đọc dưới dạng binary
    if (file == NULL)
    {
        perror("Không thể mở tệp");
        exit(EXIT_FAILURE);
    }

    char buffer[1024];

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        ssize_t bytesSent = send(client_socket, buffer, bytesRead, 0);
        if (bytesSent == -1)
        {
            perror("Lỗi khi gửi dữ liệu");
            fclose(file);
            exit(EXIT_FAILURE);
        }
        // Kiểm tra xem đã gửi đúng số byte hay không
        if ((size_t)bytesSent != bytesRead)
        {
            perror("Gửi không đủ dữ liệu");
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }

    fclose(file);
}

void listFiles(int client_socket)
{
    struct dirent *de;
    DIR *dr = opendir(FILE_PATH);

    if (dr == NULL)
    {
        error("Không thể mở thư mục");
    }

    SendFileList fileList;
    fileList.num_files = 0;

    while ((de = readdir(dr)) != NULL)
    {
        if (de->d_type == DT_REG)
        {
            strncpy(fileList.filenames[fileList.num_files], de->d_name, sizeof(fileList.filenames[fileList.num_files]));
            fileList.num_files++;
        }
    }

    closedir(dr);

    if (send(client_socket, &fileList, sizeof(fileList), 0) == -1)
    {
        error("Lỗi khi gửi danh sách tệp");
    }
}

void receiveFile(int client_socket, char *filename)
{
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s%s", FILE_PATH, filename);

    FILE *file = fopen(filepath, "wb"); // Mở file để ghi dưới dạng binary
    if (file == NULL)
    {
        perror("Không thể tạo tệp");
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    ssize_t bytesRead;

    while (1)
    {
        bytesRead = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0)
        {
            break;
        }
        size_t bytesWritten = fwrite(buffer, 1, bytesRead, file);
        if (bytesWritten != bytesRead)
        {
            perror("Write to file failed");
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }

    fclose(file);
    printf("Đã nhận %s", filename);
}

void clearInputBuffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

void upload_confirm(int client_socket)
{
    char request[1024] = "agree_to_upload";
    send(client_socket, request, strlen(request) + 1, 0);
}

void upload_refuse(int client_socket)
{
    char request[1024] = "refuse_to_upload";
    send(client_socket, request, strlen(request) + 1, 0);
}

void *client_handler(void *arg)
{
    int client_socket = *((int *)arg);
    char buffer[1024];
    char filename[1024];
    // char request[1024] = "upload";
    ssize_t bytesRead;
    size_t length_password = strlen(PASSWORD);

    // Khởi tạo buffer và filename
    memset(buffer, 0, sizeof(buffer));
    memset(filename, 0, sizeof(filename));

    bytesRead = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytesRead <= 0)
    {
        close(client_socket);
        pthread_exit(NULL);
    }

    // Loại bỏ ký tự newline (nếu có) và đảm bảo kết thúc chuỗi
    // buffer[bytesRead] = '\0';
    // if (buffer[bytesRead - 1] == '\n') {
    //     buffer[bytesRead - 1] = '\0';
    // }
    // printf("%s",buffer);
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    getpeername(client_socket, (struct sockaddr *)&client_addr, &addr_len);
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);

    printf("Kết nối từ: %s\n", client_ip);

    if (buffer[bytesRead - 1] == '\n')
    {
        buffer[bytesRead - 1] = '\0';
    }

    if (strcmp(buffer, "list") == 0)
    {
        listFiles(client_socket);
    }
    else if (strncmp(buffer, "send", 4) == 0)
    {
        strcpy(filename, buffer + 4);
        sendFile(client_socket, filename);
    }
    else if (strncmp(buffer, "upload", 6) == 0)
    {
        strcpy(buffer, buffer + 6);
        if (strncmp(buffer, PASSWORD, length_password) == 0)
        {
            // upload_confirm(client_socket);
            strcpy(buffer, buffer + length_password);
            if (strlen(buffer) == 0)
            {
                upload_confirm(client_socket);
            }
            // else {
            //     upload_refuse(client_socket);
            // }
        }
        else
        {
            upload_refuse(client_socket);
        }
    }
    else if (strncmp(buffer, PASSWORD, length_password) == 0)
    {
        strcpy(buffer, buffer + length_password);
        receiveFile(client_socket, buffer);
    }
    else
    {
        printf("%s", buffer);
    }

    close(client_socket);
    pthread_exit(NULL);
}

int main()
{
    int socket_fd, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        error("Không thể tạo socket");
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        error("Không thể ràng buộc socket");
    }

    if (listen(socket_fd, 5) == -1)
    {
        error("Không thể lắng nghe kết nối");
    }

    printf("Đang chờ kết nối...\n");

    while (1)
    {
        client_socket = accept(socket_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1)
        {
            error("Không thể chấp nhận kết nối");
        }

        pthread_t thread;
        if (pthread_create(&thread, NULL, client_handler, &client_socket) != 0)
        {
            error("Không thể tạo luồng mới");
        }

        pthread_detach(thread);
    }

    close(socket_fd);

    return 0;
}

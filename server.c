#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define PORT 8081
#define SERVER_IP "127.0.0.1"
#define FILE_PATH "files/"

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

void receiveFile(int socket_fd, char *filename)
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
        bytesRead = recv(socket_fd, buffer, sizeof(buffer), 0);
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
    printf("Đã nhận tệp %s\n", filename);
}

void upload_file(int socket_fd, char *filename)
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
        ssize_t bytesSent = send(socket_fd, buffer, bytesRead, 0);
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

void listFiles(int socket_fd)
{
    char request[] = "list";
    send(socket_fd, request, strlen(request) + 1, 0);

    SendFileList receiveList;
    if (recv(socket_fd, &receiveList, sizeof(receiveList), 0) == -1)
    {
        error("Lỗi khi nhận danh sách tệp");
    }

    printf("Danh sách tệp trong thư mục 'files' của server:\n");
    for (int i = 0; i < receiveList.num_files; i++)
    {
        printf("%s\n", receiveList.filenames[i]);
    }
}

int upload_confirm(int socket_fd)
{
    char buffer[1024];
    ssize_t bytesRead;
    int t = 1;

    bytesRead = recv(socket_fd, buffer, sizeof(buffer), 0);
    if (bytesRead <= 0)
    {
        close(socket_fd);
    }
    if (strcmp(buffer, "agree_to_upload") == 0)
    {
        printf("Done\n");
        return 1;
    }
    else if (strcmp(buffer, "refuse_to_upload") == 0)
    {
        printf("False\n");
        return 0;
    }
}

char **list_files_upload(int *num_files)
{
    struct dirent *de;
    DIR *dr = opendir(FILE_PATH);

    if (dr == NULL)
    {
        error("Không thể mở thư mục");
    }

    *num_files = 0;
    char **fileList = NULL;

    // Mở rộng hàm để in ra tên các file
    printf("======== Danh sách file ========\n");

    while ((de = readdir(dr)) != NULL)
    {
        if (de->d_type == DT_REG)
        {
            *num_files += 1;
            fileList = (char **)realloc(fileList, sizeof(char *) * (*num_files));
            fileList[*num_files - 1] = strdup(de->d_name);

            // In ra tên file
            printf("%s\n", de->d_name);
        }
    }

    closedir(dr);

    return fileList;
}

void clearInputBuffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

int main()
{
    int socket_fd;
    struct sockaddr_in server_addr;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
    //     error("Không thể kết nối đến server");
    // }

    while (1)
    {
        // Nhập lựa chọn
        printf("Lựa chọn:\n");
        printf("1. Xem danh sách tên tệp trong thư mục 'files' của server\n");
        printf("2. Tải tệp từ server\n");
        printf("3. Upload\n");
        printf("4. Exit\n");

        int choice;
        scanf("%d", &choice);
        clearInputBuffer();

        switch (choice)
        {
        case 1:
        {
            socket_fd = socket(AF_INET, SOCK_STREAM, 0);
            connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
            listFiles(socket_fd);
            close(socket_fd);
            break;
        }
        case 2:
        {
            int t = 0;
            char request[1024] = "list";
            socket_fd = socket(AF_INET, SOCK_STREAM, 0);
            connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
            send(socket_fd, request, strlen(request) + 1, 0);
            SendFileList receiveList;
            recv(socket_fd, &receiveList, sizeof(receiveList), 0);
            close(socket_fd);
            socket_fd = socket(AF_INET, SOCK_STREAM, 0);
            connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
            while (t == 0)
            {
                char filename[256];
                for (int i = 0; i < receiveList.num_files; i++)
                {
                    printf("%s", receiveList.filenames[i]);
                }

                printf("Nhập tên file cần tải hoặc 'quit' để thoát: ");
                fgets(filename, sizeof(filename), stdin);

                size_t len = strlen(filename);
                if (len > 0 && filename[len - 1] == '\n')
                {
                    filename[len - 1] = '\0';
                }

                if (strcmp(filename, "quit") == 0)
                {
                    break;
                }

                int find = 0;
                for (int i = 0; i < receiveList.num_files; i++)
                {
                    if (strcmp(receiveList.filenames[i], filename) == 0)
                    {
                        find = 1;
                        break;
                    }
                }

                if (find)
                {
                    snprintf(request, sizeof(request), "%s%s", "send", filename);
                    printf("%s", request);
                    send(socket_fd, request, strlen(request) + 1, 0);
                    // send(socket_fd, filename, strlen(filename) + 1, 0);
                    receiveFile(socket_fd, filename);
                    t = 1;
                }
                else
                {
                    printf("Tệp không tồn tại. Vui lòng thử lại.\n");
                }
            }
            close(socket_fd);
            break;
        }

        case 3:
        {
            char request[1024] = "upload";
            socket_fd = socket(AF_INET, SOCK_STREAM, 0);
            connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
            char password[256];
            printf("Nhập password de tai tai lieu hoặc 'quit' để thoát: ");
            fgets(password, sizeof(password), stdin);

            size_t len = strlen(password);
            if (len > 0 && password[len - 1] == '\n')
            {
                password[len - 1] = '\0';
            }

            if (strcmp(password, "quit") == 0)
            {
                break;
            }

            snprintf(request, sizeof(request), "%s%s", "upload", password);
            send(socket_fd, request, strlen(request) + 1, 0);
            if (upload_confirm(socket_fd) == 1)
            {
                close(socket_fd);
                socket_fd = socket(AF_INET, SOCK_STREAM, 0);
                connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
                int a = 1;
                int num_files;
                char **fileList = list_files_upload(&num_files);
                // char filename[256];
                while (a)
                {
                    // int num_files;
                    // char **fileList = list_files_upload(&num_files);
                    char filename[256];
                    printf("Nhập ten file hoặc 'quit' để thoát: ");
                    fgets(filename, sizeof(filename), stdin);
                    size_t len1 = strlen(filename);
                    if (len1 > 0 && filename[len1 - 1] == '\n')
                    {
                        filename[len1 - 1] = '\0';
                    }

                    if (strcmp(filename, "quit") == 0)
                    {
                        break;
                        a = 0;
                    }

                    for (int i = 0; i < num_files; i++)
                    {
                        if (strcmp(filename, fileList[i]) == 0)
                        {
                            a = 0;
                            snprintf(request, sizeof(request), "%s%s", password, fileList[i]);
                            send(socket_fd, request, strlen(request) + 1, 0);
                            // sleep(1);
                            // char buffer[1024];
                            // ssize_t bytesRead;
                            // memset(buffer, 0, sizeof(buffer));
                            // bytesRead = recv(socket_fd, buffer, sizeof(buffer), 0);
                            // if (buffer[bytesRead - 1] == '\n')
                            // {
                            //     buffer[bytesRead - 1] = '\0';
                            // }
                            // if (strcmp(buffer, "let's_upload") == 0)
                            // {
                            //     printf("%s", buffer);
                            //     upload_file(socket_fd, fileList[i]);
                            // }

                            upload_file(socket_fd, fileList[i]);
                            printf("Done\n");
                            close(socket_fd);
                            break;
                        }
                    }
                    if (a == 1)
                    {
                        printf("File khong ton tai, vui long nhap lai\n");
                    }
                }
                // close(socket_fd);

                // snprintf(request, sizeof(request), "%s%s", password, filename);
                // send(socket_fd, request, strlen(request) + 1, 0);

                // //printf("%s", request);
                // upload_file(socket_fd, filename);
                // close(socket_fd);
            }
            else
            {
                printf("Mat khau sai\n");
            }

            // char filename[256];
            // printf("Nhập ten file hoặc 'quit' để thoát: ");
            // fgets(filename, sizeof(filename), stdin);
            // size_t len = strlen(filename);
            // if (len > 0 && filename[len - 1] == '\n') {
            //     filename[len - 1] = '\0';
            // }

            // if (strcmp(filename, "quit") == 0) {
            //     break;
            // }

            // snprintf(request, sizeof(request), "%s%s%s", "upload", password, filename);
            // send(socket_fd, request, strlen(request) + 1, 0);

            close(socket_fd);
            break;
        }

        case 4:
            close(socket_fd);
            exit(0);
            break;
        default:
            printf("Lựa chọn không hợp lệ. Vui lòng thử lại.\n");
            break;
        }
    }

    // close(socket_fd);

    return 0;
}

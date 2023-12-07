# Send_file
## Bên server
Tải code chương trình về
```
git clone https://github.com/hoangducccc/Send_file
```
Có thể thay đổi password trong server.c tại 
```
#define PASSWORD "bungbu"
```
Các file muốn chia sẻ cho các client được lưu trữ tại thư mục "files"
Biên dich chương trình từ server.c
```
gcc -o server server.c -pthread
```
Chạy chương trình bên server
```
./server
```
## Bên client
Tải code chương trình về
```
git clone https://github.com/hoangducccc/Send_file
```
Các file muốn tải từ server và upload lên server được lưu trữ tại thư mục "files"
Biên dich chương trình từ client.c
```
gcc -o client client.c
```
Chạy chương trình bên server
```
./client
```
## Lưu ý
Có thể thử nghiệm trên một máy vật lý, tuy nhiên phải tạo hai thư mục client và server khác nhau để lưu chương trình, thay địa chỉ ip của server là 127.0.0.1 trong client.c
```
#define SERVER_IP "20.78.13.17"
```

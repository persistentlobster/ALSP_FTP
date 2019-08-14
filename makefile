all: client server

client: ftpClient.c ftpUtil.c
	gcc -g -Wall -o client ftpClient.c ftpUtil.c

server: ftpServer.c ftpUtil.c
	gcc -g -Wall -o server ftpServer.c ftpUtil.c

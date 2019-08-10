#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>

void perror_exit(char *msg);

int countArgsToken(const char *buf, char * delim);
void parseOnToken(char *src, char *result[], char *token);

int sndfile(int sd, int fd, char *filename);
int recvfile(int sd, const char *filename, int filesize);

void send_msg(char * msg, int sd);
int rec_msg(char ** msg, int sd);

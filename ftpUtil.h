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
#include <errno.h>
#include <glob.h>
#include <libgen.h>
#include <time.h>

extern int const BUF_MAX;

void perror_exit(char *msg);

int countArgsToken(const char *buf, char * delim);
void parseOnToken(char *src, char *result[], char *token);
int expandPath(char * path, glob_t *holder);

int sndfile(int sd, int fd, char *filename);
int recvfile(int sd, const char *filename);

void send_msg(char * msg, int sd);
int rec_msg(char ** msg, int sd);

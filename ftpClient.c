/**
 * ftpClient.c
 *
 * Joseph Venetucci and Micah Burnett, CS410P ALSP, 7/26/2019
 *
 * This is the CLIENT code for the myFTP program. The client
 * establishes a connection with the server and sends commands
 * until the user terminates the program.
*/

#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

int BUF_MAX = 500;

void error_exit(const char *msg) {
  fputs(msg, stderr);
  exit(EXIT_FAILURE);
}

void perror_exit(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

/************ MAIN PROGRAM **************/

int main(int argc, char **argv) {
  struct sockaddr_in srv_addr; // IPv4 domain type
  char buf[BUF_MAX];
  int port, sd, bytes_sent, bytes_recv;
  char *host;
  
  if (argc != 3) {
    fprintf(stderr, "Usage: %s hostname port", argv[0]);
    exit(EXIT_FAILURE);
  }

  host = argv[1];
  port = strtol(argv[2], NULL, 10);

  while (1) {
    memset(&srv_addr, 0, sizeof(srv_addr)); 
    srv_addr.sin_family = AF_INET;							// IPv4
    srv_addr.sin_addr.s_addr = inet_addr(host); // resolve hostname
    srv_addr.sin_port = htons(port);            // convert int to network formatted uint16_t

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
      perror_exit("error opening socket");

    if (connect(sd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0)
      perror_exit("error connecting to socket");

    memset(buf, 0, BUF_MAX);
    printf("> ");
    fgets(buf, BUF_MAX, stdin);

    if (strncmp(buf, "exit", 4) == 0) {
      printf("Exiting...\n");
      exit(EXIT_SUCCESS);
    }

    if ((bytes_sent = write(sd, buf, strlen(buf))) < 0)
      perror_exit("error sending message");

		memset(buf, 0, BUF_MAX);

    if ((bytes_recv = recv(sd, buf, BUF_MAX, MSG_WAITALL)) < 0)
      perror("error receiving message");

    printf("Client sent %d bytes. Server sent %d bytes\n", bytes_sent, bytes_recv);
    printf("Response from server: %s\n", buf);
  }
  close(sd);

  exit(EXIT_SUCCESS);
}

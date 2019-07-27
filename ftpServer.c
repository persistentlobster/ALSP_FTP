/**
 * ftpServer.c
 *
 * Joseph Venetucci and Micah Burnett, CS410P ALSP, 7/26/2019
 *
 * This is the SERVER code for the myFTP program. The server
 * listens over a connection with the client for incoming requests.
*/

/*#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>

*/

#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

int BUF_MAX = 500;

void perror_exit(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
	int sd, client_sc, port;
	char buf[BUF_MAX];
	char *response;
	struct sockaddr_in serv_addr;
	int num_bytes;
	time_t t;
	struct tm tm;
	
	if (argc < 2) {
		fprintf(stderr,"Usage: %s port\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	sd = socket(AF_INET, SOCK_STREAM, 0);
	
	if (sd < 0) 
		perror_exit("error opening socket\n");
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	port = strtol(argv[1], NULL, 10);
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	
	if (bind(sd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
		perror_exit("error binding to socket\n");
	
	listen(sd, 5);

	while (1) {
		if ((client_sc = accept(sd, (struct sockaddr *) NULL, NULL)) < 0) 
			perror_exit("error accepting request");
		
		memset(buf, 0, BUF_MAX);
		num_bytes = read(client_sc, buf, BUF_MAX-1);
		
		if (num_bytes < 0) 
			perror_exit("ERROR reading from socket");
		
		t = time(NULL);
		tm = *localtime(&t);

		printf("received message at %d:%d:%d : %s\n", 
					 tm.tm_hour, tm.tm_min, tm.tm_sec, buf);
	
		response = "message received";	
		if ((num_bytes = write(client_sc, response, strlen(response))) < 0)
			perror_exit("error writing to socket");
		
		close(client_sc);
	}

  exit(EXIT_SUCCESS);
}



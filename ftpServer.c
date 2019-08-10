/**
 * ftpServer.c
 *
 * Joseph Venetucci and Micah Burnett, CS410P ALSP, 7/26/2019
 *
 * This is the SERVER code for the myFTP program. The server
 * listens over a connection with the client for incoming requests.
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
#include <limits.h>
#include <fcntl.h>

int BUF_MAX = 4096;

/**
 * prints a message from the user followed by the msg associated
 * with errno and exits.
 */
void perror_exit(char *msg) {
    perror(msg);
    exit(1);
}

/**
 * Returns the number of tokens separated by the delimiter
 */
int countArgsToken(const char *buf, char * delim) {
  int count = 0;
  char * token;
  char bufCopy[strlen(buf)+ 1];
    
  if (!buf)
    return -1;

  strcpy(bufCopy, buf);
  token = strtok(bufCopy, delim); 
  while (token != NULL) {
    ++count;
    token = strtok(NULL, delim);
  }
  return count;
}

/**
 * Genereal purpose function that parses src on the specified token
 * using strtok. Result[] is filled with an array of pointers to the
 * beginning of each null-terminated token.
 */
void parseOnToken(char *src, char *result[], char *token) {
  int i=0;
  char *arg;

  arg = strtok(src, token);
  while (arg != NULL) {
    result[i++] = arg;
    arg = strtok(NULL, token);
  }
  result[i] = NULL;
}

/**
 * Creates a file (filename) with contents read from socket (sd).
 * Returns number of bytes written on success, or -1 on failure.
 */
int recvfile(int sd, const char *filename) {
  mode_t MODE = (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);  // Set perms to 644
  int fd_dst, bytes_recv, num_bytes = 0, total_read = 0;
  char *buf[BUF_MAX];
  unsigned long filesize;
  
  memset(buf, 0, BUF_MAX);

  // Read size of file that will be sent over socket
  if (read(sd, (char *) &filesize, sizeof(filesize)) < 0) {
    perror_exit("read error");
  }
  filesize = (unsigned long) ntohl(filesize);
  printf("About to receive %lu bytes\n", filesize);

  // 1. Create/overwrite file
  if ((fd_dst = open(filename, O_WRONLY | O_CREAT | O_TRUNC, MODE)) < 0) {
    return -1;
  }

  // 2. Read file contents from socket. NOTE: Do NOT use strlen as there could be a '\n' in buf
  while ((bytes_recv = read(sd, buf, BUF_MAX)) != 0) {
    total_read += bytes_recv;
    if (bytes_recv < 0) {
      return -1;
    }

    // 3. Write contents to file until all bytes that have been read have been written
    void *bufp = buf;
    while (bytes_recv > 0) {
      int bytes_sent = write(fd_dst, bufp, bytes_recv);
      if (bytes_sent <= 0) {
          perror_exit("write error");
      }
      bytes_recv -= bytes_sent;
      bufp += bytes_sent;
      num_bytes += bytes_sent;
    }
    if (num_bytes == filesize)    // Done
      break;
  }
  printf("Total bytes read: %d\n", total_read);
  printf("Total bytes written: %d\n", num_bytes);
  close(fd_dst);
  return num_bytes;
}

/************************/
/** BUILT-IN FUNCTIONS **/
/************************/
/* Built in functions have the form 'int builtin_NAME(char ** args)'
 * where NAME is the name of the builtin and args is a list of
 * arguments to that builtin.
 *
 * To add additional builtin functions:
 * (1) Create the function, following the form just described.
 * (2) Add the newly created function to getBuiltInFunc()
 */


/** STEP 1: Define builtin function here **/

/** STEP 2: Add builtin function here **/
int (*getBuiltInFunc(char * cmd))(char **) {
    return NULL;
}

/****************************/
/** END BUILT-IN FUNCTIONS **/
/****************************/

int main(int argc, char *argv[]) {
	int sd, client_sc, port;
	char buf[BUF_MAX];
	struct sockaddr_in serv_addr;
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
		
    // Read command from client
		memset(buf, 0, BUF_MAX);
    dup2(client_sc, 0);
    fgets(buf, BUF_MAX, stdin);

    // Chomp newline
    buf[strlen(buf)-1] = '\0';

    /****************** BEGIN PROCESSING PUT CMD *******************/

    if (strncmp(buf, "put", 3) == 0) {
      printf("received \"put\" command from client\n");
      printf("buf is: %s\n", buf);

      int arg_count = countArgsToken(buf, " ");
      if (arg_count != 2) {
        fprintf(stderr, "Error: expected 2 tokens but received %d\n", arg_count);
        continue;
      }
      char *args[arg_count + 1];
      parseOnToken(buf, args, " ");
      char *file = args[1];

      // Receive the file
      if (recvfile(client_sc, file) < 0)
        perror_exit("recvfile error");

      /****************** END PROCESSING PUT CMD *******************/
    }

		t = time(NULL);
		tm = *localtime(&t);

		// printf("received message at %d:%d:%d : %s\n", 
		// 			 tm.tm_hour, tm.tm_min, tm.tm_sec, buf);
	
		// response = "message received";	
		// if ((num_bytes = write(client_sc, response, strlen(response))) < 0)
		// 	perror_exit("error writing to socket");
		
    // Check if it's a builtin, and execute if it is
    int (*func)() = getBuiltInFunc(buf);
    if (func) {
      func();
      continue;
    }

		close(client_sc);
	}

  exit(EXIT_SUCCESS);
}



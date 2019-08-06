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
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>

int BUF_MAX = 500;

/**
 * prints a message from the user followed by the msg associated
 * with errno and exits.
 */
void perror_exit(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

/**
 * Returns the number of tokens separated by the delimiter
 */
int
countArgsToken(const char *buf, char * delim)
{
  int count = 0;
  char * token;
  char bufCopy[strlen(buf)+ 1]; // Do not destroy original string
    
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
void
parseOnToken(char *src, char *result[], char *token)
{
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
 * Reads the contents from a file and sends to server over socket
 * Returns number of bytes sent on success, or -1 on failure.
 */
int sndfile(int sd, int fd, char *filename) {
  struct stat st;
  int bytes_recv, bytes_sent, filesize, sendsize;

  // Get file size
  if (stat(filename, &st) < 0) {
    return -1;
  }
  filesize = st.st_size;
  sendsize = htonl(filesize);

  // Send file size to server
  if (write(sd, (char *) &sendsize, sizeof(sendsize)) < 0) {
    return -1;
  }
  char *buf[filesize];
  memset(buf, 0, filesize);

  // Read the file until there is nothing left to read
  while ((bytes_recv = read(fd, buf, BUF_MAX-1)) != 0) {
    if (bytes_recv < 0) {
      return -1;
    }
    // Write file contents to socket
    if ((bytes_sent = write(sd, buf, filesize)) < 0) {
      return -1;
    }
    printf("Wrote %d bytes\n", bytes_sent);
  }
  close(fd);
  return bytes_sent;
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

// Exits the shell.
int builtin_exit(char ** args) {
  printf("Exiting...\n");
  exit(EXIT_SUCCESS);
}

/** STEP 2: Add builtin function here **/
int (*getBuiltInFunc(char * cmd))(char **) {
  if (strcmp(cmd, "exit") == 0)
    return &builtin_exit;
  else
    return NULL;
}

/****************************/
/** END BUILT-IN FUNCTIONS **/
/****************************/

/************ MAIN PROGRAM **************/

int main(int argc, char **argv) {
  struct sockaddr_in srv_addr;
  char buf[BUF_MAX];
  int port, sd, bytes_sent;
  char *host;
  
  if (argc != 3) {
    fprintf(stderr, "Usage: %s hostname port", argv[0]);
    exit(EXIT_FAILURE);
  }

  host = argv[1];
  port = strtol(argv[2], NULL, 10);

  while (1) {
    memset(&srv_addr, 0, sizeof(srv_addr)); 
    srv_addr.sin_family = AF_INET;              // IPv4
    srv_addr.sin_addr.s_addr = inet_addr(host); // resolve hostname
    srv_addr.sin_port = htons(port);            // convert int to network formatted uint16_t

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
      perror_exit("error opening socket");

    if (connect(sd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0)
      perror_exit("error connecting to socket");

    memset(buf, 0, BUF_MAX);
    printf("> ");
    fgets(buf, BUF_MAX, stdin);

    // Need newline in original buf to use as EOL delimiter when sending to server
    char bufCopy[strlen(buf) + 1];
    strcpy(bufCopy, buf);
    
    // remove '\n' from copy for getBuiltInFunc check
    bufCopy[strlen(bufCopy)-1] = '\0';

    // Check if it's a builtin, and execute if it is
    int (*func)() = getBuiltInFunc(bufCopy);
    if (func) {
      func();
      continue;
    }

/************ BEGIN PROCESSING PUT CMD **************/

    if (strncmp(buf, "put", 3) == 0) {
      printf("begin processing \"put\" command\n");

      // Send command to server
      if ((bytes_sent = write(sd, buf, strlen(buf))) < 0)
        perror_exit("error sending message");

      buf[strlen(buf)-1] = '\0';

      int arg_count = countArgsToken(buf, " ");
      if (arg_count != 2) {
        fprintf(stderr, "Error: expected 2 tokens but received %d\n", arg_count);
        continue;
      }
      char *args[arg_count + 1];
      parseOnToken(buf, args, " ");
      char *file = args[1];

      // Open the file for reading
      mode_t MODE = (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
      int fd;
      if (access(file, R_OK) < 0) {
        perror_exit("file access error");
      }
      if ((fd = open(file, O_RDONLY, MODE)) < 0) {
        perror_exit("open error");
      }
      
      // Send the file to the server
      if (sndfile(sd, fd, file) < 0) {
        perror("sndfile error");
      }
    }
    /************* END PROCESSING PUT CMD *************/

		//memset(buf, 0, BUF_MAX);

    // if ((bytes_recv = recv(sd, buf, BUF_MAX, MSG_WAITALL)) < 0)
    //   perror("error receiving message");

    // printf("Client sent %d bytes. Server sent %d bytes\n", bytes_sent, bytes_recv);
    // printf("Response from server: %s\n", buf);
  }
  close(sd);

  exit(EXIT_SUCCESS);
}

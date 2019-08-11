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

const char *FILE_OK = "received command";

int BUF_MAX = 4096;

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
  int bytes_recv, num_bytes = 0;
  unsigned long filesize, sendsize;
  char *buf[BUF_MAX];

  memset(buf, 0, BUF_MAX);

  // Get file size
  if (stat(filename, &st) < 0) {
    return -1;
  }
  filesize = (unsigned long) st.st_size;
  sendsize = (unsigned long) htonl(filesize);

  // Send file size to server
  if (write(sd, (char *) &sendsize, sizeof(sendsize)) < 0) {
    return -1;
  }
  printf("File size is %lu bytes\n", filesize);

  // Read the file until there is nothing left to read
  while ((bytes_recv = read(fd, buf, BUF_MAX)) != 0) {
    if (bytes_recv < 0) {
      return -1;
    }

    // Write file contents to socket
    void *bufp = buf;
    while (bytes_recv > 0) {
      int bytes_sent = write(sd, bufp, bytes_recv);
      if (bytes_sent <= 0) {
          perror_exit("write error");
      }
      bytes_recv -= bytes_sent;
      bufp += bytes_sent;
      num_bytes += bytes_sent;
    }
  }
  printf("Total bytes written: %d\n", num_bytes);
  close(fd);
  return num_bytes;
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

      // Send command size to server
      int size = htonl(strlen(buf));
      write(sd, (char *) &size, sizeof(size));

      // Send command to server
      if ((bytes_sent = write(sd, buf, strlen(buf))) < 0)
        perror_exit("error sending message");

      // Chomp newline
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
    /************* END PROCESSING PUT CMD *************/

    } else if (strncmp(buf, "get", 3) == 0) {
      printf("begin processing \"put\" command\n");

      // Send command size to server
      int size = htonl(strlen(buf));
      write(sd, (char *) &size, sizeof(size));

      // Send command to server
      if ((bytes_sent = write(sd, buf, strlen(buf))) < 0)
        perror_exit("error sending message");

      // Receive response from server
      int len;
      if ((bytes_recv = read(sd, &len, sizeof(len))) < 0)
        perror_exit("read error");
      
      len = ntohl(len);
      char response[len+1];
      memset(response, 0, len+1);
      if ((read(sd, &response, len)) < 0)
        perror_exit("read error");

      printf("response: %s\n", response);

      if (strcmp(response, FILE_OK) == 0) {
        buf[strlen(buf)-1] = '\0';

        int arg_count = countArgsToken(buf, " ");
        if (arg_count != 2) {
          fprintf(stderr, "Error: expected 2 tokens but received %d\n", arg_count);
          continue;
        }
        char *args[arg_count + 1];
        parseOnToken(buf, args, " ");
        char *file = args[1];

        // Receive the file
        if (recvfile(sd, file) < 0)
          perror_exit("recvfile error");

      } else {
        fprintf(stderr, "error: %s\n", response);
        continue;
      }      
    }
		//memset(buf, 0, BUF_MAX);

    // if ((bytes_recv = recv(sd, buf, BUF_MAX, MSG_WAITALL)) < 0)
    //   perror("error receiving message");

    // printf("Client sent %d bytes. Server sent %d bytes\n", bytes_sent, bytes_recv);
    // printf("Response from server: %s\n", buf);
  }
  close(sd);

  exit(EXIT_SUCCESS);
}
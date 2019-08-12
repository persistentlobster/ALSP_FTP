/**
 * ftpClient.c
 *
 * Joseph Venetucci and Micah Burnett, CS410P ALSP, 7/26/2019
 *
 * This is the CLIENT code for the myFTP program. The client
 * establishes a connection with the server and sends commands
 * until the user terminates the program.
*/

#include "ftpUtil.h"

const char *FILE_OK = "received command";
const int BAD_CMD = -1;


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
int builtin_exit(char * cmd, char ** args, int sd) {
  printf("Exiting...\n");
  exit(EXIT_SUCCESS);
}

int builtin_put(char * cmd, char ** args, int sd) {

  int  bytes_sent;
  printf("begin processing \"put\" command\n");

  // Send command size to server
  int size = htonl(strlen(cmd));
  write(sd, (char *) &size, sizeof(size));
/**
 * Sends BAD_CMD (error code: -1) to server so that server knows to abort
 * NOTE: server must be expecting to receive an integer value
 */
 /**
void snd_bad_cmd(int sd) {
  int bad_cmd = htonl(BAD_CMD);
  write(sd, &bad_cmd, sizeof(bad_cmd));
}
**/
/**
 * Reads the contents from a file and sends to server over socket
 * Returns number of bytes sent on success, or -1 on failure.
 */
/**
int sndfile(int sd, int fd, char *filename) {
  struct stat st;
  int bytes_recv, num_bytes = 0;
  int filesize, sendsize;
  char *buf[BUF_MAX];

  memset(buf, 0, BUF_MAX);

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
  printf("File size is %d bytes\n", filesize);
  **/

  // Send command to server
  if ((bytes_sent = write(sd, cmd, strlen(cmd))) < 0)
    perror_exit("error sending message");

/**
  int arg_count = countArgsToken(buf, " ");
  if (arg_count != 2) {
    fprintf(stderr, "Error: expected 2 tokens but received %d\n", arg_count);
    continue;
  }
  char *args[arg_count + 1];
  parseOnToken(buf, args, " ");
  **/
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

  return 0;
}

int builtin_get(char * cmd, char ** args, int sd) {

  int bytes_sent, bytes_recv;
  printf("begin processing \"put\" command\n");

  // Send command size to server
  int size = htonl(strlen(cmd));
  write(sd, (char *) &size, sizeof(size));

  // Send command to server
  if ((bytes_sent = write(sd, cmd, strlen(cmd))) < 0)
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
/**
  // 2. Read file contents from socket. NOTE: Do NOT use strlen as there could be a '\n' in buf
  while (total_read < filesize) {
    bytes_recv = read(sd, buf, BUF_MAX);
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
  }
  printf("Total bytes read: %d\n", total_read);
  printf("Total bytes written: %d\n", num_bytes);
  close(fd_dst);
  return num_bytes;
}
**/

  printf("response: %s\n", response);

  if (strcmp(response, FILE_OK) == 0) {
    /**
    buf[strlen(buf)-1] = '\0';

    int arg_count = countArgsToken(buf, " ");
    if (arg_count != 2) {
      fprintf(stderr, "Error: expected 2 tokens but received %d\n", arg_count);
      continue;
    } **/
    char *file = args[1];

    // Receive the file
    if (recvfile(sd, file) < 0)
      perror_exit("recvfile error");

  } else {
    fprintf(stderr, "error: %s\n", response);
    return -1;
  } 

  return 0;
}

/** STEP 2: Add builtin function here **/
int (*getBuiltInFunc(char * cmd))(char *, char **, int) {
  if (strcmp(cmd, "exit") == 0)
    return &builtin_exit;
  else if (strcmp(cmd, "put") == 0)
    return &builtin_put;
  else if (strcmp(cmd, "get") == 0)
    return &builtin_get;
  else
    return NULL;
}

/****************************/
/** END BUILT-IN FUNCTIONS **/
/****************************/

/************ MAIN PROGRAM **************/

int main(int argc, char **argv) {
  struct sockaddr_in srv_addr;
  char buf[BUF_MAX], bufCopy[BUF_MAX];
  int port, sd;
  char *host;

  if (argc != 3) {
    fprintf(stderr, "Usage: %s hostname port", argv[0]);
    exit(EXIT_FAILURE);
  }

  /** Setup connection to Server **/
  host = argv[1];
  port = strtol(argv[2], NULL, 10);
  memset(&srv_addr, 0, sizeof(srv_addr)); 
  srv_addr.sin_family = AF_INET;              // IPv4
  srv_addr.sin_addr.s_addr = inet_addr(host); // resolve hostname
  srv_addr.sin_port = htons(port);            // convert int to network formatted uint16_t

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    perror_exit("error opening socket");

  if (connect(sd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0)
    perror_exit("error connecting to socket");


  // After obtaining a socket to the server: get cmds, send to server, loop
  while (1) {
    // Read in from command line
    memset(buf, 0, BUF_MAX);
    printf("> ");
    fgets(buf, BUF_MAX, stdin);

    // Chomp newline and break input into command and args
    buf[strlen(buf)-1] = '\0';
    char * args[countArgsToken(buf, " ") + 1];
    strcpy(bufCopy, buf);
    parseOnToken(buf, args, " ");

    // Check if it's a builtin, and execute if it is
    int (*func)() = getBuiltInFunc(args[0]);
   /** 
    // remove '\n' from copy for getBuiltInFunc check
    buf[strlen(buf)-1] = '\0';

    // Check if it's a builtin, and execute if it is
    int (*func)() = getBuiltInFunc(buf);
    **/
    if (func) {
      func(bufCopy, args, sd);
      continue;
    }

    // Otherwise send it to the server.
    send_msg(bufCopy, sd);
    // Recieve a response from the server.
    char * response = NULL;
    rec_msg(&response, sd);
    printf("%s\n", response);
    free(response);
/************ BEGIN PROCESSING PUT CMD **************/
/**

    if (strncmp(buf, "put", 3) == 0) {
      printf("begin processing \"put\" command\n");

      // Parse args and keep original copy to pass to server
      char bufCopy[strlen(buf) + 1];
      strcpy(bufCopy, buf);

      int arg_count = countArgsToken(buf, " ");
      if (arg_count != 2) {
        fprintf(stderr, "Error: expected 2 tokens but received %d\n", arg_count);
        snd_bad_cmd(sd);
        continue;
      }

      char *args[arg_count + 1];
      parseOnToken(buf, args, " ");
      char *file = args[1];

      // Check if file access is OK (file exists or insufficient permissions)
      if (access(file, R_OK) < 0) {
        perror("error accessing file");
        snd_bad_cmd(sd);
        continue;
      } 

      // Command is OK. Send command (and size) to server
      int size = htonl(strlen(bufCopy));
      write(sd, (char *) &size, sizeof(size));

      if (write(sd, bufCopy, strlen(bufCopy)) < 0)
        perror_exit("error sending message");
      
      // Open the file for reading
      mode_t MODE = (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
      int fd;

      if ((fd = open(file, O_RDONLY, MODE)) < 0) {
        snd_bad_cmd(sd);
        perror_exit("open error");
      }
      
      // Send the file to the server
      if (sndfile(sd, fd, file) < 0) {
        perror("sndfile error");
      }
      **/
    /************* END PROCESSING PUT CMD *************/
/**
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
    **/
  }
  close(sd);

  exit(EXIT_SUCCESS);
}

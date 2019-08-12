/**
 * ftpServer.c
 *
 * Joseph Venetucci and Micah Burnett, CS410P ALSP, 7/26/2019
 *
 * This is the SERVER code for the myFTP program. The server
 * listens over a connection with the client for incoming requests.
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

int builtin_put(char * cmd, char ** args, int sd) {
  printf("received \"put\" command from client\n");
  printf("buf is: %s\n", cmd);

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

  // Receive the file
  if (recvfile(sd, file) < 0)
    perror_exit("recvfile error");

  return 0;
}

int builtin_get(char * cmd, char ** args, int sd) {
  printf("received \"get\" command from client\n");

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

  int err;
  char *response;
  if ((err = access(file, R_OK)) < 0) {
    switch (errno) {
      case ENOENT:
        response = "file does not exist";
        break;
      case EACCES:
        response = "permission denied";
        break;
      default:
        response = "an unknown error occured";
        break;
    }
  } else {
    response = FILE_OK;
  }
  int len = htonl(strlen(response));
  if (write(sd, &len, sizeof(len)) < 0)
    perror_exit("write error");
  
  if (write(sd, response, strlen(response)) < 0)
    perror_exit("write error");
  
  if (err < 0)
    return -1;

  if ((fd = open(file, O_RDONLY, MODE)) < 0) {
    perror_exit("open error");
  }
  
  // Send the file to the server
  if (sndfile(sd, fd, file) < 0) {
    perror("sndfile error");
  }

  return 0;
/**
  printf("Total bytes written: %d\n", num_bytes);
  close(fd);
  return num_bytes;
}
**/
/**
 * Creates a file (filename) with contents read from socket (sd).
 * Returns number of bytes written on success, or -1 on failure.
 */
 /**
int recvfile(int sd, const char *filename, int filesize) {
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
  **/
}

/** STEP 2: Add builtin function here **/
int (*getBuiltInFunc(char * cmd))(char *, char **, int) {
  if (strcmp(cmd, "put") == 0)
    return &builtin_put;
  else if (strcmp(cmd, "get") == 0)
    return &builtin_get;
  else
    return NULL;
}

/****************************/
/** END BUILT-IN FUNCTIONS **/
/****************************/

void run_loop(int client_sc) {
    char buf[BUF_MAX];
    while (1) {
      // Read message from client
      memset(buf, 0, BUF_MAX);
      char * msg = NULL;
      if (rec_msg(&msg, client_sc) == 0) {
        printf("Client disconnected");
        exit(0);
      }
      printf("Got %s from client\n", msg);
      strcpy(buf, msg);
      free(msg);

      // Break message into command and args
      char * args[countArgsToken(buf, " ")];
      parseOnToken(buf, args, " ");

      // Check if it's a builtin, and execute if it is
      int (*func)() = getBuiltInFunc(args[0]);
      if (func) {
        func(buf, args, client_sc);
        continue;
      }

      // Otherwise parse the input here
      // Send a message back to client
      memset(buf, 0, BUF_MAX);
      strcpy(buf, "Got your message");
      send_msg(buf, client_sc);
    }
    close(client_sc);
}

int main(int argc, char *argv[]) {
	int sd, client_sc, port;
	struct sockaddr_in serv_addr;
	
	if (argc < 2) {
		fprintf(stderr,"Usage: %s port\n", argv[0]);
		exit(EXIT_FAILURE);
	}

  /** Set up a listener socket **/
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
  printf("Sever started...\n");

  /** Wait for a client to connect **/
  while(1) {
    if ((client_sc = accept(sd, (struct sockaddr *) NULL, NULL)) < 0) 
      perror_exit("error accepting request");
/**
	while (1) {
		if ((client_sc = accept(sd, (struct sockaddr *) NULL, NULL)) < 0) 
			perror_exit("error accepting request");
		
    // Read command size (or error code) from client
    int size;
    read(client_sc, &size, sizeof(size));
    size = ntohl(size);

    // Check if it's a bad command, abort and handle next request
    if (size == BAD_CMD) {
        fprintf(stderr, "received bad command from client\n");
        continue;
    }
    // Read command from client
		memset(buf, 0, BUF_MAX);
    read(client_sc, buf, size);
    printf("cmd size is: %d\n", size);
**/
    /****************** BEGIN PROCESSING PUT CMD *******************/

    int pid = fork();

    if (pid == 0) { //Child
      /** Now that a client has connected, enter in a loop of recieving and sending messages **/
      close(sd);
      run_loop(client_sc);
    } else {  //If the parent, continue waiting for more connections to accept.
      close(client_sc);
/**
      // Receive file size (or error code) from server
      int len;
      if (read(client_sc, &len, sizeof(len)) < 0)
        perror_exit("read error");
      
      len = ntohl(len);
    if (strncmp(buf, "put", 3) == 0) {
      printf("received \"put\" command from client\n");
      printf("buf is: %s\n", buf);
      
      // (2 for tokens, 1 for terminating null byte)
      char *args[3];
      parseOnToken(buf, args, " ");
      char *file = args[1];

      // Receive the file
      if (recvfile(client_sc, file) < 0)
        perror_exit("recvfile error");
**/
      /****************** END PROCESSING PUT CMD *******************/
  /**  
    } else if (strncmp(buf, "get", 3) == 0) {
      printf("received \"get\" command from client\n");

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

      int err;
      const char *response;
      if ((err = access(file, R_OK)) < 0) {
        switch (errno) {
          case ENOENT:
            response = "file does not exist";
            break;
          case EACCES:
            response = "permission denied";
            break;
          default:
            response = "an unknown error occured";
            break;
        }
      } else {
        response = FILE_OK;
      }
      int len = htonl(strlen(response));
      if (write(client_sc, &len, sizeof(len)) < 0)
        perror_exit("write error");
      
      if (write(client_sc, response, strlen(response)) < 0)
        perror_exit("write error");
      
      if (err < 0)
        continue;

      if ((fd = open(file, O_RDONLY, MODE)) < 0) {
        perror_exit("open error");
      }
      
      // Send the file to the server
      if (sndfile(client_sc, fd, file) < 0) {
        perror("sndfile error");
      }
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
      **/
    }
  }
  exit(EXIT_SUCCESS);
}

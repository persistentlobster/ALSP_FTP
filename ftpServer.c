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
  
  // (2 for tokens, 1 for terminating null byte)
  /**
  char *args[3];
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
  if (write(sd, &len, sizeof(len)) < 0)
    perror_exit("write error");

  if (write(sd, response, strlen(response)) < 0)
    perror_exit("write error");

  if (err < 0)
    return -1;
    //continue;

  if ((fd = open(file, O_RDONLY, MODE)) < 0) {
    perror_exit("open error");
  }

  // Send the file to the server
  if (sndfile(sd, fd, file) < 0) {
    perror("sndfile error");
  }
  return 0;
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

    int pid = fork();

    if (pid == 0) { //Child
      /** Now that a client has connected, enter in a loop of recieving and sending messages **/
      close(sd);
      run_loop(client_sc);
    } else {  //If the parent, continue waiting for more connections to accept.
      close(client_sc);
    }
  }
  exit(EXIT_SUCCESS);
}

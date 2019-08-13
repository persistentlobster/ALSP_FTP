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
int bytes_recv;
char c;

/**
 * Executes shell commands on the server and sends
 * output back to client.
 */
int ls_cmd(const char *cmd, int sd) {
  FILE *fp;
  char buf[BUF_MAX];

  memset(&buf, 0, sizeof(buf));

  fp = popen(cmd, "r");
  if (!fp)
    return 0;

  int ch;
  while ((ch = fgetc(fp)) != EOF) {
    strcat(buf, (char*) &ch);
  } 
  
  // Check that it was EOF and not a read error
  if (!feof(fp)) {
    perror("error reading ls output");
    return 0;
  }

  // Remove trailing newline
  char *cp;
  if ((cp = strrchr(buf, '\n')) != NULL) {
    *cp = ' ';
  }

  pclose(fp);
  send_msg(buf, sd);
  return 1;
}

/**
 * Checks if shell command is supported by server
 */
int is_supported_command(char *cmd) {
  int supported = 0;
  if (strcmp(cmd, "ls") == 0)
    supported = 1;
  
  return supported;
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

// Sends the current working directory to socket sd
int builtin_pwd(char * cmd, char ** args, int sd) {
  char buf[PATH_MAX];
  int ret = 0;
  if (getcwd(buf,PATH_MAX) == NULL) {
    perror("Unable to get current working directory");
    strcpy(buf, "Server unable to get current working directory\n");
    ret = -1;
  }
  send_msg(buf, sd);
  return ret;
}

// Change the current working directory on the server.
// Sends back the new current working dir to client
int builtin_cd(char * cmd, char ** args, int sd) {
  char buf[BUF_MAX];
  int ret = 0;

  memset(buf, 0, BUF_MAX);
  if (args[1] == NULL) {
    fprintf(stderr, "No directory provided\n");
    strcpy(buf, "No directory provided\n");
    ret = -1;
  } else {
    if (chdir(args[1]) < 0) {
      perror(args[1]);
      strcpy(buf, strerror(errno));
      ret = -1;
    } else {
      return builtin_pwd(NULL, NULL, sd);
    }
  }
  send_msg(buf, sd);
  return ret;
}

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
  unsigned long len = htonl(strlen(response));
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
  else if (strcmp(cmd, "pwd") == 0)
    return &builtin_pwd;
  else if (strcmp(cmd, "cd") == 0)
    return &builtin_cd;
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

      // Save copy for commands that require whole command line
      char bufCopy[BUF_MAX];
      memset(bufCopy, 0, BUF_MAX);
      strcpy(bufCopy, buf);

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
      if (is_supported_command(buf)) {
        ls_cmd(bufCopy, client_sc);

      } else {
        memset(buf, 0, BUF_MAX);
        strcpy(buf, "Got your message");
        send_msg(buf, client_sc);
      }
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

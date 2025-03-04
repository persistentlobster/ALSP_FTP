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

const int L_INFO = 1;
const int L_ERR  = 2;
const int L_PERR = 3;

void logger(char * msg, int loglevel) {
  int errnoCopy = errno;
  time_t t = time(NULL);
  char timestamp[50];
  strftime(timestamp, 50, "[%D %T]", localtime(&t));
 
  if (loglevel == L_INFO)
    fprintf(stdout, "%s %s\n", timestamp, msg);
  else if (loglevel == L_ERR)
    fprintf(stderr, "%s %s\n", timestamp, msg);
  else if (loglevel == L_PERR)
    fprintf(stderr, "%s %s : %s\n", timestamp, msg, strerror(errnoCopy));
  else
    fprintf(stdout, "%s %s\n", timestamp, msg);
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
    //perror("Unable to get current working directory");
    logger("Unable to get current working directory", L_PERR);
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
    //fprintf(stderr, "No directory provided\n");
    logger("No directory provided", L_ERR);
    strcpy(buf, "No directory provided\n");
    ret = -1;
  } else {
    glob_t gl;
    expandPath(args[1], &gl);
    if (chdir(gl.gl_pathv[0]) < 0) {
      perror(gl.gl_pathv[0]);
      strcpy(buf, strerror(errno));
      ret = -1;
      globfree(&gl);
    } else {
      globfree(&gl);
      return builtin_pwd(NULL, NULL, sd);
    }
  }
  send_msg(buf, sd);
  return ret;
}

// Executes shell commands on the server and sends
// output back to client.
int builtin_ls(char *cmd, char ** args, int sd) {
  FILE *fp;
  char buf[BUF_MAX];

  memset(&buf, 0, sizeof(buf));

  fp = popen(cmd, "r");
  if (!fp)
    return -1;

  int ch;
  while ((ch = fgetc(fp)) != EOF) {
    strcat(buf, (char*) &ch);
  } 
  
  // Check that it was EOF and not a read error
  if (!feof(fp)) {
    //perror("error reading ls output");
    logger("error reading ls output", L_PERR);
    return -1;
  }

  // Remove trailing newline
  char *cp;
  if ((cp = strrchr(buf, '\n')) != NULL) {
    *cp = ' ';
  }

  pclose(fp);
  send_msg(buf, sd);
  return 0;
}

int builtin_put(char * cmd, char ** args, int sd) {
  //printf("received \"put\" command from client\n");
  //printf("buf is: %s\n", cmd);
  
  char *file = args[1];

  // Receive the file
  if (recvfile(sd, file) < 0)
    perror_exit("recvfile error");
  else 
    send_msg(cmd, sd);

  return 0;
}

int builtin_get(char * cmd, char ** args, int sd) {
  char * msg;
  //printf("received \"get\" command from client\n");

  glob_t gl;
  expandPath(args[1], &gl);
  //printf("%s has %zu options\n", cmd, gl.gl_pathc);
  char filename[BUF_MAX];
  for (int i = 0; i < gl.gl_pathc; i++) {
    char *file = gl.gl_pathv[i];

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

    // Send file name
    memset(filename, 0, BUF_MAX);
    strcpy(filename, basename(gl.gl_pathv[i]));
    send_msg(filename, sd);

    // Send the file to the server
    if (sndfile(sd, fd, file) < 0) {
      //perror("sndfile error");
      logger("sndfile error", L_PERR);
    } else {
      rec_msg(&msg, sd);
      //printf("%s\n", msg);
      logger(msg, L_INFO);
      free(msg);

      if ((i+1) < gl.gl_pathc)
        send_msg("More",sd);
    }
  }

  send_msg("Done",sd);
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
  else if (strcmp(cmd, "ls") == 0)
    return &builtin_ls;
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
        //printf("Client disconnected");
        logger("Client disconnected", L_INFO);
        exit(0);
      }
      //printf("Got %s from client\n", msg);
      logger(msg, L_INFO);
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
        func(bufCopy, args, client_sc);
        continue;
      }

      // If its not a builtin, its not a supported command
      // Send a message back to client
      //printf("%s is not a supported command\n", bufCopy);
      strcat(bufCopy, " not supported");
      logger(bufCopy, L_INFO);
      memset(buf, 0, BUF_MAX);
      strcpy(buf, "Invalid Command");
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
  //printf("Sever started...\n");
  logger("Server started...", L_INFO);

  /** Wait for a client to connect **/
  while(1) {
    if ((client_sc = accept(sd, (struct sockaddr *) NULL, NULL)) < 0) 
      perror_exit("error accepting request");

    logger("Client connected", L_INFO);
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

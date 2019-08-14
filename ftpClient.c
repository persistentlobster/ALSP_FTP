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
#include <sys/wait.h>

const char *FILE_OK = "received command";
const int BAD_CMD = -1;

/**
 * Sends BAD_CMD (error code: -1) to server so that server knows to abort
 * NOTE: server must be expecting to receive an integer value
 */
void snd_bad_cmd(int sd) {
  unsigned long bad_cmd = htonl(BAD_CMD);
  write(sd, &bad_cmd, sizeof(bad_cmd));
}

/**
 * Resolves a server IPv4 address from the hostname.
 * If the address is resolved, it is copied into ip parameter
 * and the function returns 1. Otherwise, it returns 0.
 */
int resolve_host(char *hostname, char *ip) {
  struct addrinfo hint, *res, *cur;
  struct sockaddr_in *sin;
  char buf[INET_ADDRSTRLEN];
  const char *addr = NULL;
  int result;

  memset(&hint, 0, sizeof(hint));
  hint.ai_family = AF_INET;         // IPv4
  hint.ai_socktype = SOCK_STREAM;

  if ((result = getaddrinfo(hostname, NULL, &hint, &res)) != 0) {
    fprintf(stderr, "error: %s", gai_strerror(result));
    return 0;
  }
  
  for (cur = res; cur != NULL; cur = cur->ai_next) {
    sin = (struct sockaddr_in *)cur->ai_addr;
    addr = inet_ntop(AF_INET, &sin->sin_addr, buf, INET_ADDRSTRLEN);
    if (addr) {
      strcpy(ip, addr);
      return 1;  
    }    
  } 
  freeaddrinfo(res);
  return 0;
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

// Run cmd in the shell
int builtin_escape(char * cmd, char ** args, int sd) {
  if (system(NULL) == 0) {
    fprintf(stderr, "No shell is available...\n");
    return -1;
  }

  system(++cmd);
  return 0;
}

// Exits the shell.
int builtin_exit(char * cmd, char ** args, int sd) {
  printf("Exiting...\n");
  exit(EXIT_SUCCESS);
}

// Prints out the current working directory
int builtin_lpwd(char * cmd, char ** args, int sd) {
  char buf[PATH_MAX];
  if (getcwd(buf,PATH_MAX) == NULL) {
    perror("Unable to get current working directory");
    return -1;
  }

  printf("%s\n", buf);
  return 0;
}

// Execs the client ls command.
// NOTE: Much of this code block was provided by Professor Mark Morrissey
// TODO: wildcard expansion
int builtin_lls(char * cmd, char ** args, int sd) {
  int retval;

  // Chomp leading "l"
  cmd++;

  if ((retval = system(cmd)) == -1) {
    perror("system() error");
    return -1;
  } else if (retval == 127) {
    fprintf(stderr, "shell failed to execute command\n");
    return -1;
  }
  return 0;
}

// Change the current working directory
// Then print the directory out to the user
int builtin_lcd(char * cmd, char ** args, int sd) {
  if (args[1] == NULL) {
    fprintf(stderr, "No directory provided\n");
    return -1;
  }
  glob_t gl;
  expandPath(args[1], &gl);
  if (chdir(gl.gl_pathv[0]) < 0) {
    perror(gl.gl_pathv[0]);
    globfree(&gl);
    return -1;
  }
  globfree(&gl);

  return builtin_lpwd(NULL, NULL, 0);
}

// Put the files onto the server
int builtin_put(char * cmd, char ** args, int sd) {
  printf("begin processing \"put\" command\n");

  glob_t gl;
  expandPath(args[1], &gl);
  char full_cmd[BUF_MAX];
  char * msg;
  for (int i = 0; i < gl.gl_pathc; i++) {
    char *file = gl.gl_pathv[i];

    // Check if file access is OK (file exists or insufficient permissions)
    if (access(file, R_OK) < 0) {
      perror("error accessing file");
      snd_bad_cmd(sd);
      //return -1;
      continue;
    } 

    // Command is OK. Send command (and size) to server
    memset(full_cmd, 0, BUF_MAX);
    strcpy(full_cmd, "put ");
    strcat(full_cmd, basename(gl.gl_pathv[i]));
    unsigned long size = htonl(strlen(full_cmd));
    write(sd, (char *) &size, sizeof(size));

    if (write(sd, full_cmd, strlen(full_cmd)) < 0)
      perror("error sending message");
      //perror_exit("error sending message");
    
    // Open the file for reading
    mode_t MODE = (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    int fd;

    if ((fd = open(file, O_RDONLY, MODE)) < 0) {
      perror("Open error");
      snd_bad_cmd(sd);
      //perror_exit("open error");
    }
    
    // Send the file to the server
    if (sndfile(sd, fd, gl.gl_pathv[i]) < 0) {
      perror("sndfile error");
    }
    
    rec_msg(&msg, sd);
    printf("%s\n", msg);
    free(msg);
  }
  globfree(&gl);
  return 0;
}

// Retrieve files from the server
int builtin_get(char * cmd, char ** args, int sd) {

  int bytes_sent, bytes_recv;
  int f_continue = 1;
  printf("begin processing \"get\" command\n");

  // Send command size to server
  unsigned long size = htonl(strlen(cmd));
  write(sd, (char *) &size, sizeof(size));

  // Send command to server
  if ((bytes_sent = write(sd, cmd, strlen(cmd))) < 0)
    perror_exit("error sending message");

  do {
    // Receive response from server
    unsigned long len;
    if ((bytes_recv = read(sd, &len, sizeof(len))) < 0)
      perror_exit("read error");
    
    len = ntohl(len);
    char response[len+1];
    memset(response, 0, len+1);
    if ((read(sd, &response, len)) < 0)
      perror_exit("read error");

    printf("response: %s\n", response);

    if (strcmp(response, FILE_OK) == 0) {
      //Get the file name from server
      char * filename;
      rec_msg(&filename, sd);
      char *file = filename;

      // Receive the file
      if (recvfile(sd, file) < 0)
        perror_exit("recvfile error");
      else {
        printf("Got %s\n", filename);
        free(filename);
        send_msg("Done", sd);
        char * msg;
        rec_msg(&msg, sd);
        if (strcmp(msg, "Done") == 0)
          f_continue = 0;
        free(msg);
      }
    } else {
      fprintf(stderr, "error: %s\n", response);
      return -1;
    }      
  }while(f_continue);
  return 0;
}

/** STEP 2: Add builtin function here **/
int (*getBuiltInFunc(char * cmd))(char *, char **, int) {
  if (cmd[0] == '!')
    return &builtin_escape;
  else if (strcmp(cmd, "exit") == 0)
    return &builtin_exit;
  else if (strcmp(cmd, "put") == 0)
    return &builtin_put;
  else if (strcmp(cmd, "get") == 0)
    return &builtin_get;
  else if (strcmp(cmd, "lpwd") == 0)
    return &builtin_lpwd;
  else if (strncmp(cmd, "lls", 3) == 0)
    return &builtin_lls;
  else if (strcmp(cmd, "lcd") == 0)
    return &builtin_lcd;
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
  char addr[INET6_ADDRSTRLEN];
  char *host;
  int port, sd;

  if (argc != 3) {
    fprintf(stderr, "Usage: %s hostname port", argv[0]);
    exit(EXIT_FAILURE);
  }

  /** Setup connection to Server **/
  host = argv[1];
  port = strtol(argv[2], NULL, 10);
  memset(&srv_addr, 0, sizeof(srv_addr)); 
  srv_addr.sin_family = AF_INET;              // IPv4
  
  if (!resolve_host(host, addr))
    exit(EXIT_FAILURE);

  srv_addr.sin_addr.s_addr = inet_addr(addr); // resolve hostname
  srv_addr.sin_port = htons(port);            // convert int to network formatted uint16_t

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    perror_exit("error opening socket");

  if (connect(sd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0)
    perror_exit("error connecting to socket");

  printf("Connected to server at address: %s\n", addr);

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
  }
  close(sd);

  exit(EXIT_SUCCESS);
}

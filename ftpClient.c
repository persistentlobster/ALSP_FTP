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

int BUF_MAX = 500;

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
  char buf[BUF_MAX], bufCopy[BUF_MAX];
  int port, sd, bytes_sent;
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
    if (func) {
      func(args);
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

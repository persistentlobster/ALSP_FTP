#include "ftpUtil.h"


int const BUF_MAX = 4096;

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
 * Wrapper around glob() to expand '~', wildcards, etc
 * Remember to call globfree() on holder
 */
int expandPath(char * path, glob_t *holder) {
  return glob(path, GLOB_TILDE, NULL, holder);
};

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
  filesize = st.st_size;
  sendsize = htonl(filesize);

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
}

// Read message and return size of message
int rec_msg(char ** msg, int sd) {
    // Get the size of the message
    unsigned long size;
    if (recv(sd, &size, sizeof(size), 0) == 0)
      return 0;
    unsigned long rec_size = ntohl(size);
    // Malloc memory to hold message
    *msg = malloc(rec_size+1);
    memset(*msg, 0, (rec_size+1));
    // Then get the message
    recv(sd, *msg, rec_size, 0);
    return rec_size;
}

void send_msg(char * msg, int sd) {
    // Get size of message
    unsigned long size = (unsigned long)strlen(msg);
    unsigned long send_size = (unsigned long) htonl(size);
    // Send the size first
    send(sd, &send_size, sizeof(send_size), 0);
    // then the command
    send(sd, msg, strlen(msg), 0);
}

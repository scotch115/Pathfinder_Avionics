#include <stddef.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "queue.h"
#include <termios.h>

#define DEBUG 2
#define DATA_SIZE 20480
#define DATA_FILE "/var/tmp/pdtpserverdata"

volatile sig_atomic_t listening = true;
volatile bool daemonMode = false;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct socketThread {
  int thread_id;
  pthread_t * thread;
  pthread_mutex_t * thread_lock;
  int fd;
  char * serial_port;
  bool thread_complete;
};

struct connectionList {
  struct socketThread * thread;
  SLIST_ENTRY(connectionList) entries;
};

void sysPrint(int logLevel, char * message) {
    // Level 0 => LOG_INFO
    // Level 1 => LOG_ERR
    // Level 2 => LOG_DEBUG
    if (logLevel == 0) {
        syslog(LOG_INFO, "%s\n", message);
        printf("%s\n", message);
    } else if (logLevel == 1) {
        syslog(LOG_ERR, "%s\n", message);
        printf("%s\n", message);
    } else if (logLevel == 2) {
        syslog(LOG_DEBUG, "%s\n", message);
        printf("%s\n", message);
    } else {
        syslog(LOG_ERR, "sysPrint() received an invalid 'logLevel' argument!\n");
        fprintf(stderr, "sysPrint() received an invalid 'logLevel' argument!\n");
    }
}

void signal_handler(int signal) {
    // Logs message to syslog "Caught signal, exiting" when SIGINT or SIGTERM are received
    sysPrint(0, "Caught signal, exiting.");
    listening = false;
    // Gracefully exits when SIGINT or SIGTERM are received; completing any open connection operations, closing any open sockets, and DELETING the /var/tmp/aesdsocketdata file
    exit(EXIT_SUCCESS);
}

int serial_port_init(const char * serial_port) {
  int fd = open(serial_port, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd == -1) {
    perror("serial_port_init: Unable to open port ");
    return -1;
  }
  fcntl(fd, F_SETFL, 0);
  return fd;
}

void serial_port_config(int fd) {
  struct termios tty;

  if (tcgetattr(fd, &tty) != 0) {
    perror("serial_port_init: Could not get terminal attributes");
    exit(EXIT_FAILURE);
  }

  // Set serial Baud rate
  speed_t baud_rate = B9600;
  cfsetispeed(&tty, baud_rate);
  cfsetospeed(&tty, baud_rate);

  // 8N1: 8 data bits, no parity, 1 stop bit
  tty.c_cflag &= ~PARENB;             // No parity bit
  tty.c_cflag &= ~CSTOPB;             // 1 stop bit
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;                 // 8 data bits
  tty.c_cflag &= ~CRTSCTS;            // No hardware flow control
  tty.c_cflag |= CREAD | CLOCAL;      // Enable receiver, ignore modem lines

  // Disable software flow control
  tty.c_iflag &= ~(IXON | IXOFF | IXANY);

  // Raw input (no canonical mode, no echo, no signals)
  tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

  // Raw output
  tty.c_oflag &= ~OPOST;

  // Disable special handling of received bytes
  tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

  // Blocking read: wait up to 1 second, return when at least 1 byte arrives
  tty.c_cc[VMIN] = 0;   // Return immediately if no bytes
  tty.c_cc[VTIME] = 10; // 10 * 100ms = 1 second timeout

  if (tcsetattr(fd, TCSANOW, &tty) != 0) {
    perror("serial_port_init: Could not set terminal attributes");
    exit(EXIT_FAILURE);
  }
}

int serial_port_read(int fd, char * buf, size_t max_len) {
  ssize_t n = read(fd, buf, max_len);
  if (n < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
    perror("read");
    return -1;
  }
  return n;
}

void * socket_func(void* socket_param) {
  struct socketThread* socket_thread = (struct socketThread *) socket_param;

  int threadLock = pthread_mutex_lock(socket_thread->thread_lock);
  if (threadLock != 0) {
    sysPrint(1, "Failed to lock socket thread!");
  } else {
    if (DEBUG == 2) {
      printf("Successfully locked socket thread!\n");
    }
  }

  char read_buffer[256];
  int maxlen = 14;
  int fd = serial_port_init(socket_thread->serial_port);
  if (fd != -1) {
    printf("Successfully set 'fd' to serial_port\n");
  }
  serial_port_config(fd);
  ssize_t n = serial_port_read(fd, read_buffer, maxlen - 1);
  close(fd);
  if (n > 0) {
    read_buffer[n] = '\n';
    printf("%s\n", read_buffer);
    fd = open(DATA_FILE, O_CREAT | O_APPEND | O_WRONLY, 0644);
    if (fd != -1) {
      write(fd, read_buffer, maxlen);
      close(fd);
    }
  } else {
    printf("No response within timeout.\n");
  }
  // close(fd);

  // fd = open(DATA_FILE, O_CREAT | O_APPEND | O_WRONLY, 0644);
  // if (fd != -1) {
  //   write(fd, read_buffer, sizeof(read_buffer));
  //   close(fd);
  // }

  int threadUnlock = pthread_mutex_unlock(socket_thread->thread_lock);
  if (threadUnlock != 0) {
    sysPrint(1, "Failed to unlock socket thread!");
  } else {
    if (DEBUG == 2) {
      printf("Successfully unlocked socket thread!\n");
    }
  }

  socket_thread->thread_complete = true;
  return 0;  
}

int main(int argc, char *argv[]) {
  openlog ("pdtp-server", LOG_DEBUG | LOG_ERR, LOG_USER);
  syslog(LOG_INFO, "STARTING PDTP SERVER PROGRAM");
  if (errno != 0) {
    fprintf(stderr, "UNABLE TO OPEN SYSLOG IN PDTP SERVER PROGRAM!!\n");
  }
  if (argc > 1) {
    daemonMode = true;
    sysPrint(0, "Started pdtp server daemon");
  }
      
  if (daemonMode) {
    pid_t pid = fork();
    if (pid > 0) {
      exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
      perror("setsid");
      exit(EXIT_FAILURE);
    }

    chdir("/");
    umask(0);

    int fd = open("/dev/null", O_RDWR);
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    if (fd > STDERR_FILENO) {
      close(fd);
    }
  }

  while(1) {
    char serial_port[256];
    strcpy(serial_port, "/dev/ttyACM2");
    int threadNum = 0;
    SLIST_HEAD(listhead, connectionList) head;
    SLIST_INIT(&head);

    struct sigaction sigrecv;
    memset(&sigrecv, 0, sizeof(sigrecv));
    sigrecv.sa_handler = signal_handler;

    fprintf(stdout, "Attempting to start PDTP server...");

    while(1) {
      if (!listening) {
        printf("Quitting\n");
      }

      if (sigaction(SIGINT, &sigrecv, NULL) == -1) {
        break;
      }

      if (sigaction(SIGTERM, &sigrecv, NULL) == -1) {
        break;
      }

      pthread_t thread;

      struct socketThread * sockThread = malloc(sizeof(struct socketThread));

      if (sockThread == NULL) {
        sysPrint(1, "Could not allocate memory for socket thread!");
        return false;
      }

      sockThread->thread_lock = &mutex;
      sockThread->thread_complete = false;
      sockThread->serial_port = serial_port;
      sockThread->thread = &thread;
      sockThread->thread_id = threadNum;
      // sockThread->fd = read_port;

      int threadStatus = pthread_create(&thread, NULL, &socket_func, sockThread);
      if (threadStatus != 0) {
        sysPrint(1, "Thread(s) not created!");
        free(sockThread);
        return 1;
      }

      if (DEBUG == 2) {
        fprintf(stdout, "Thread created with ID - %d\n", threadNum);
      }

      struct connectionList * connection = malloc(sizeof(struct connectionList));
      if (connection == NULL) {
        sysPrint(1, "Could not allocate memory for connectionList!");
        return 1;
      }
      
      struct connectionList *cur;
      connection->thread = sockThread;

      SLIST_INSERT_HEAD(&head, connection, entries);

      threadNum += 1;

      SLIST_FOREACH(cur, &head, entries) {
        // if (DEBUG == 2) {
        //   printf("THREAD ID: %d\n", cur->thread->thread_id);
        //   printf("THREAD COMPLETE: %d\n", cur->thread->thread_complete);
        // }
        if (cur->thread->thread_complete == 1) {
          pthread_join(*cur->thread->thread, NULL);
          // if (DEBUG == 2) {
          //   printf("Joining/freeing Thread_%d\n", cur->thread->thread_id);
          // }
        }
      }
      usleep(1500000);
    }
    return 0;  
  }
  return 0; 
}

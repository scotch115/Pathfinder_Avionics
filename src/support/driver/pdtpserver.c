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

#define BUFFER_SIZE 256
#define DEBUG 2
#define DATA_LEN 32
#define DATA_FILE "/var/tmp/pdtpserverdata"

volatile sig_atomic_t listening = true;
volatile bool daemonMode = false;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct serial_thread {
  int thread_id;
  pthread_t * thread;
  pthread_mutex_t * thread_lock;
  int serial_port_fd;
  bool thread_complete;
};

struct connectionList {
  struct serial_thread * thread;
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

void serial_port_config(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    perror("serial_port_init: F_GETFL failed ");
    close(fd);
    exit(EXIT_FAILURE);
  }

  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    perror("serial_port_init: fcntl F_SETFL failed");
    close(fd);
    exit(EXIT_FAILURE);
  }
}

void serial_port_init(int fd) {
  struct termios tty;

  if (tcgetattr(fd, &tty) != 0) {
    perror("serial_port_init: Could not get terminal attributes");
    close(fd);
    exit(EXIT_FAILURE);
  }

  // Set serial Baud rate
  cfsetispeed(&tty, B115200);
  cfsetospeed(&tty, B115200);

  // 8N1: 8 data bits, no parity, 1 stop bit
  tty.c_cflag &= ~PARENB;     // No parity bit
  tty.c_cflag &= ~CSTOPB;     // 1 stop bit
  tty.c_cflag &= ~CSIZE;      // Clear size bits
  tty.c_cflag |= CS8;         // 8 data bits
  tty.c_cflag |= CLOCAL;      // Ignore modem control lines
  tty.c_cflag |= CREAD;       // Enable receiver

  // Disable software flow control
  tty.c_iflag &= ~(IXON | IXOFF | IXANY);

  // Disable canonical mode (raw input) and echo
  tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

  // Blocking read: wait up to 1 second, return when at least 1 byte arrives
  tty.c_cc[VMIN] = 0;   // Return immediately if no bytes
  tty.c_cc[VTIME] = 10; // 10 * 100ms = 1 second timeout

  if (tcsetattr(fd, TCSANOW, &tty) != 0) {
    perror("serial_port_init: Could not set terminal attributes");
    close(fd);
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

struct payloadPacket {
  int16_t tmp;
  int16_t kpa;
  int16_t alt;
  int16_t hum;
  int16_t yaw;
  int16_t pch;
  int16_t rol;
  int16_t acx;
  int16_t acy;
  int16_t acz;
  int16_t yrc;
  int16_t prc;
  int16_t rrc;
};

union PDTP_Packet {
  struct payloadPacket payload;
  uint16_t bytes[sizeof(struct payloadPacket)]; // Two bytes
};

// CRC-8 Checksum Encode
uint8_t encodeCRC8(uint16_t *data, size_t length) {
  uint8_t polynomial = 0x31;
  uint8_t crc = 0xFF;
  size_t i, j;

  for (i = 0; i < length; i++) {
    crc ^= data[i] << 8;
    for (j = 0; j < 8; j++) {
      if ((crc & 0x80) != 0) {
        crc = (crc << 1) ^ polynomial;
      } else {
        crc <<= 1;
      }
    }
  }

  return crc;
}

// CRC-8 Checksum Decode
bool decodeCRC8(uint16_t *data, uint8_t rx_crc) {
  // Generate CRC-8 using the data received from the PDTP packet
    // PDTP protocol constrains payload size to 26 
  uint8_t localCRC = encodeCRC8(data, 26);
  // Check if the calculated CRC-8 value matches the value received
  if (rx_crc == localCRC) {
    return true;
  } else {
    return false;
  }
}  

void pdtpRx(char * packet_buffer, char * write_buff) {
  uint8_t rxPkt[32];
  union PDTP_Packet packet;
  for (int b = 0; b < 32; b++) {
    rxPkt[b] = packet_buffer[b];
  }

  // Extract PDTP Header and Footer bytes
  // uint8_t statusByte = rxPkt[0];
  // uint8_t destAddr = rxPkt[1];
  // uint8_t srcAddr = rxPkt[2];
  uint8_t crc = rxPkt[31];

  // Extract PDTP Payload bytes into payloadPacket struct
  int16_t tmp = ((int16_t)rxPkt[3]  << 8) | rxPkt[4];
  int16_t kpa = ((int16_t)rxPkt[5]  << 8) | rxPkt[6];
  int16_t alt = ((int16_t)rxPkt[7]  << 8) | rxPkt[8];
  int16_t hum = ((int16_t)rxPkt[9]  << 8) | rxPkt[10];
  int16_t yaw = ((int16_t)rxPkt[11] << 8) | rxPkt[12];
  int16_t pch = ((int16_t)rxPkt[13] << 8) | rxPkt[14];
  int16_t rol = ((int16_t)rxPkt[15] << 8) | rxPkt[16];
  int16_t acx = ((int16_t)rxPkt[17] << 8) | rxPkt[18];
  int16_t acy = ((int16_t)rxPkt[19] << 8) | rxPkt[20];
  int16_t acz = ((int16_t)rxPkt[21] << 8) | rxPkt[22];
  int16_t yrc = ((int16_t)rxPkt[23] << 8) | rxPkt[24];
  int16_t prc = ((int16_t)rxPkt[25] << 8) | rxPkt[26];
  int16_t rrc = ((int16_t)rxPkt[27] << 8) | rxPkt[28];
  int16_t sn  = ((int16_t)rxPkt[29] << 8) | rxPkt[30];

  if(decodeCRC8(packet.bytes, crc)) {
    printf("- CRC MATCH -\n");
  } else {
    printf("-== CRC MISMATCH ==-\n");
  }

  char payload_string[256];
  int str_n = sprintf(payload_string, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
        tmp, kpa, alt, hum, yaw, pch, rol, acx, acy, acz, yrc, prc, rrc, sn);
  FILE * fptr = fopen(DATA_FILE, "a+");
  if (str_n > 0) {
    fputs(payload_string, fptr);
    printf("%s\n", payload_string);
  }
}

void * serial_func(void* serial_param) {
  struct serial_thread* serial_port_thread = (struct serial_thread *) serial_param;

  int threadLock = pthread_mutex_lock(serial_port_thread->thread_lock);
  if (threadLock != 0) {
    sysPrint(1, "Failed to lock socket thread!");
  } else {
    if (DEBUG == 2) {
      printf("Successfully locked socket thread!\n");
    }
  }

  char buffer[BUFFER_SIZE];
  ssize_t num_bytes = serial_port_read(serial_port_thread->serial_port_fd, buffer, BUFFER_SIZE - 1);
  if (num_bytes == -1) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      perror("read failed");
      close(serial_port_thread->serial_port_fd);
      return 0;
    }
  } else if (num_bytes > 0) {
    buffer[num_bytes] = '\0'; // Null-terminate for printing/processing
    char write_buffer[BUFFER_SIZE];
    printf("Received %zd bytes: %s\n", num_bytes, buffer);

    pdtpRx(buffer, write_buffer);

    int fd = open(DATA_FILE, O_CREAT | O_APPEND | O_WRONLY, 0644);
    if (fd != -1) {
      write(fd, write_buffer, DATA_LEN);
      close(fd);
    }
  }
  
  int threadUnlock = pthread_mutex_unlock(serial_port_thread->thread_lock);
  if (threadUnlock != 0) {
    sysPrint(1, "Failed to unlock socket thread!");
  } else {
    if (DEBUG == 2) {
      printf("Successfully unlocked socket thread!\n");
    }
  }

  serial_port_thread->thread_complete = true;
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

  int threadNum = 0;
  SLIST_HEAD(listhead, connectionList) head;
  SLIST_INIT(&head);

  // Signal handling
  struct sigaction sigrecv;
  memset(&sigrecv, 0, sizeof(sigrecv));
  sigrecv.sa_handler = signal_handler;
  
  int serial_fd;
  const char *port_path = "/dev/ttyACM2"; // TBD

  serial_fd = open(port_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (serial_fd == -1) {
    perror("Failed to open serial port");
    return EXIT_FAILURE;
  }
  printf("Serial port opened successfully (fd: %d)\n", serial_fd);

  serial_port_init(serial_fd);
  serial_port_config(serial_fd);

  fd_set read_fds;
  struct timeval timeout;
  int ret;

  while(1) {
    // Check to see if we can still open/access the serial port
    int check_port;
    if ((check_port = open(port_path, O_RDWR | O_NOCTTY | O_NONBLOCK) == -1)) {
      close(check_port);
      close(serial_fd);
      printf("SERIAL PORT UNAVAILABLE!\nQuitting.");
      exit(EXIT_FAILURE);
    }
    // Clear and initialize read set
    FD_ZERO(&read_fds);
    FD_SET(serial_fd, &read_fds);

    // Set timeout (1 second)
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    // Wait for activity or timeout
    ret = select(serial_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (ret == -1) {
      perror("select failed");
      break;
    } else if (ret == 0) {
      // Timeout: no data received
      printf("Timeout: no data available\n");
      continue;
    }

    if (FD_ISSET(serial_fd, &read_fds)) {
      printf("Data available on the serial port!\n");
      // Read data / create thread
      pthread_t thread;
      struct serial_thread * serialThread = malloc(sizeof(struct serial_thread));
      if (serialThread == NULL) {
        sysPrint(1, "Could not allocate memory for socket thread!");
        return false;
      }

      serialThread->thread_lock = &mutex;
      serialThread->thread_complete = false;
      serialThread->serial_port_fd = serial_fd;
      serialThread->thread = &thread;
      serialThread->thread_id = threadNum;

      int threadStatus = pthread_create(&thread, NULL, &serial_func, serialThread);
      if (threadStatus != 0) {
        sysPrint(1, "Thread(s) not created!");
        free(serialThread);
        return 1;
      }

      struct connectionList * connection = malloc(sizeof(struct connectionList));
      if (connection == NULL) {
        sysPrint(1, "Could not allocate memory for connectionList!");
        return 1;
      }

      struct connectionList *cur;
      connection->thread = serialThread;

      SLIST_INSERT_HEAD(&head, connection, entries);

      threadNum += 1;

      SLIST_FOREACH(cur, &head, entries) {
        if (cur->thread->thread_complete == 1) {
          pthread_join(*cur->thread->thread, NULL);
        }
      }
    }
  }

  // Cleanup
  close(serial_fd);
  printf("Serial port closed\n");
  return EXIT_SUCCESS;
}

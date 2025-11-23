// SERVER CODE + QUESTIONS |
//                         V
/*
Questions to answer at top of server.c:
(You should not need to change client.c)
Understanding the Client:
1. How is the client sending data to the server? What protocol?
-The client is using socket(AF_INET, SOCK_STREAM, 0), connect(), and write. The
protocol is TCP due to the usage.

2. What data is the client sending to the server?
-The client is sending through 5 different messages each padded to 1024 bytes
(Hello, Apple, Car, Green, Dog,)

Understanding the Server:
1. Explain the argument that the `run_acceptor` thread is passed as an argument.
-run_acceptor in server receives a pointer to a struct (acceptor_args) which
provides a run flag to set a stop, pointer to the shared struct, and pointer to
the mutex protecting the linked list.

2. How are received messages stored?
-Messages are stored in a thread-safe singly-linked list. Each of the messages
are copied into a dynamic allocation node, protected by a mutex.

3. What does `main()` do with the received messages?
-Main() waits for all the messages, stops the threads from continuing, and then
uses the function collect_all() to print and free stored messages.

4. How are threads used in this sample code?
-Threads are being used firstly in the acceptor thread where it handles incoming
connections. Secondly, each client thread reads messages and adds them to shared
list, and lastly, the main thread controls the start-up, shutdown, and final
processing.
*/

/*
 * Explain the use of non-blocking sockets in this lab
 * -Sockets are made non-blocking by calling fcntl() to retrieve the current
 * flags and the use of O_NONBLOCK flag. This allows any read/accept operation
 * to return immediately
 *
 *  -The types of sockets that are non-blocking are 1, the server listening
 * socket(sfd) and each of the client connection thread(cfd).
 *
 *  -Sockets that are non-blocking prevent threads from getting stuck waiting
 * for data. For the listening socket(sfd) the acceptor thread keeps checking
 * the run flag and stops cleanly when instructed to. For each client
 * thread(cfd), it also checks the run flag and stops when shutdown begins.
 * Non-blocking serve the purpose for threads to stay responsive, regularly
 * check flags, exit without error, and avoid full block of program.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define BUF_SIZE 1024
#define PORT 8001
#define LISTEN_BACKLOG 32
#define MAX_CLIENTS 4
#define NUM_MSG_PER_CLIENT 5

#define handle_error(msg)                                                      \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

struct list_node {
  struct list_node *next;
  void *data;
};

struct list_handle {
  struct list_node *last;
  volatile uint32_t count;
};

struct client_args {
  atomic_bool run;
  int cfd;
  struct list_handle *list_handle;
  pthread_mutex_t *list_lock;
};

struct acceptor_args {
  atomic_bool run;
  struct list_handle *list_handle;
  pthread_mutex_t *list_lock;
};

int init_server_socket() {
  struct sockaddr_in addr;

  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1)
    handle_error("socket");

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    handle_error("bind");
  if (listen(sfd, LISTEN_BACKLOG) == -1)
    handle_error("listen");
  return sfd;
}

void set_non_blocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    perror("fcntl F_GETFL");
    exit(EXIT_FAILURE);
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    perror("fcntl F_SETFL");
    exit(EXIT_FAILURE);
  }
}

void add_to_list(struct list_handle *list_handle, struct list_node *new_node) {
  struct list_node *last_node = list_handle->last;
  last_node->next = new_node;
  list_handle->last = last_node->next;
  list_handle->count++;
}

int collect_all(struct list_node head) {
  struct list_node *node = head.next;
  uint32_t total = 0;

  while (node != NULL) {
    printf("Collected: %s\n", (char *)node->data);
    total++;

    struct list_node *next = node->next;
    free(node->data);
    free(node);
    node = next;
  }
  return total;
}

static void *run_client(void *args) {
  struct client_args *cargs = (struct client_args *)args;
  int cfd = cargs->cfd;
  set_non_blocking(cfd);

  char msg_buf[BUF_SIZE];

  while (cargs->run) {
    ssize_t bytes_read = read(cfd, &msg_buf, BUF_SIZE);
    if (bytes_read == -1) {
      if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
        perror("Problem reading from socket!\n");
        break;
      }
    } else if (bytes_read > 0) {
      struct list_node *new_node = malloc(sizeof(struct list_node));
      new_node->next = NULL;
      new_node->data = malloc(BUF_SIZE);
      memcpy(new_node->data, msg_buf, BUF_SIZE);

      // TODO: Safely add node to list
      pthread_mutex_lock(cargs->list_lock);
      add_to_list(cargs->list_handle, new_node);
      pthread_mutex_unlock(cargs->list_lock);
    }
  }

  close(cfd);
  return NULL;
}

static void *run_acceptor(void *args) {
  int sfd = init_server_socket();
  set_non_blocking(sfd);

  struct acceptor_args *aargs = (struct acceptor_args *)args;
  pthread_t threads[MAX_CLIENTS];
  struct client_args client_args[MAX_CLIENTS];

  printf("Accepting clients...\n");

  uint16_t num_clients = 0;

  while (aargs->run) {
    if (num_clients < MAX_CLIENTS) {
      int cfd = accept(sfd, NULL, NULL);
      if (cfd == -1) {
        if (!(errno == EAGAIN || errno == EWOULDBLOCK))
          handle_error("accept");
      } else {
        printf("Client connected!\n");

        // fill args
        client_args[num_clients].cfd = cfd;
        client_args[num_clients].run = true;
        client_args[num_clients].list_handle = aargs->list_handle;
        client_args[num_clients].list_lock = aargs->list_lock;
        num_clients++;

        // TODO: Create the thread (must use num_clients - 1)
        pthread_create(&threads[num_clients - 1], NULL, run_client,
                       &client_args[num_clients - 1]);
      }
    }
  }

  printf("Not accepting any more clients!\n");

  // Shutdown and cleanup
  for (int i = 0; i < num_clients; i++) {
    // TODO: Stop client thread
    client_args[i].run = false;

    // TODO: join and close client socket
    pthread_join(threads[i], NULL);
    close(client_args[i].cfd);
  }

  close(sfd);
  return NULL;
}

int main() {
  pthread_mutex_t list_mutex;
  pthread_mutex_init(&list_mutex, NULL);

  struct list_node head = {NULL, NULL};
  struct list_handle list_handle = {.last = &head, .count = 0};

  pthread_t acceptor_thread;
  struct acceptor_args aargs = {
      .run = true, .list_handle = &list_handle, .list_lock = &list_mutex};

  pthread_create(&acceptor_thread, NULL, run_acceptor, &aargs);

  // TODO: Busy-wait until enough messages received
  while (1) {
    pthread_mutex_lock(&list_mutex);
    uint32_t current = list_handle.count;
    pthread_mutex_unlock(&list_mutex);

    if (current >= MAX_CLIENTS * NUM_MSG_PER_CLIENT)
      break;
  }

  aargs.run = false;
  pthread_join(acceptor_thread, NULL);

  if (list_handle.count != MAX_CLIENTS * NUM_MSG_PER_CLIENT) {
    printf("Not enough messages were received!\n");
    return 1;
  }

  int collected = collect_all(head);
  printf("Collected: %d\n", collected);

  if (collected != list_handle.count) {
    printf("Not all messages were collected!\n");
    return 1;
  }

  printf("All messages were collected!\n");

  pthread_mutex_destroy(&list_mutex);
  return 0;
}

// CLIENT CODE |
//             V
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8001
#define BUF_SIZE 1024
#define ADDR "127.0.0.1"

#define handle_error(msg)                                                      \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

#define NUM_MSG 5

static const char *messages[NUM_MSG] = {"Hello", "Apple", "Car", "Green",
                                        "Dog"};

int main() {
  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1) {
    handle_error("socket");
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  if (inet_pton(AF_INET, ADDR, &addr.sin_addr) <= 0) {
    handle_error("inet_pton");
  }

  if (connect(sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    handle_error("connect");
  }

  char buf[BUF_SIZE];
  for (int i = 0; i < NUM_MSG; i++) {
    sleep(1);
    // prepare message
    // this pads the desination with NULL
    strncpy(buf, messages[i], BUF_SIZE);

    if (write(sfd, buf, BUF_SIZE) == -1) {
      handle_error("write");
    } else {
      printf("Sent: %s\n", messages[i]);
    }
  }

  exit(EXIT_SUCCESS);
}

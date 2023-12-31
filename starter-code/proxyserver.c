#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include "proxyserver.h"
#include "safequeue.h"

/*
 * Constants
 */
#define RESPONSE_BUFSIZE 10000

/*
 * Global configuration variables.
 * Their values are set up in main() using the
 * command line arguments (already implemented for you).
 */
int num_listener;
int *listener_ports;
int num_workers;
char *fileserver_ipaddr;
int fileserver_port;
int max_queue_size;

void send_error_response(int client_fd, status_code_t err_code, char *err_msg) {
    http_start_response(client_fd, err_code);
    http_send_header(client_fd, "Content-Type", "text/html");
    http_end_headers(client_fd);
    char *buf = malloc(strlen(err_msg) + 2);
    sprintf(buf, "%s\n", err_msg);
    http_send_string(client_fd, buf);
    return;
}

/*
 * forward the client request to the fileserver and
 * forward the fileserver response to the client
 */
void serve_request(int client_fd) {
    printf("Entering serve_request\n");
    // create a fileserver socket
    int fileserver_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fileserver_fd == -1) {
        fprintf(stderr, "Failed to create a new socket: error %d: %s\n", errno, strerror(errno));
        exit(errno);
    }

    // create the full fileserver address
    struct sockaddr_in fileserver_address;
    fileserver_address.sin_addr.s_addr = inet_addr(fileserver_ipaddr);
    fileserver_address.sin_family = AF_INET;
    fileserver_address.sin_port = htons(fileserver_port);

    // connect to the fileserver
    int connection_status = connect(fileserver_fd, (struct sockaddr *)&fileserver_address,
                                    sizeof(fileserver_address));
    if (connection_status < 0) {
        // failed to connect to the fileserver
        printf("Failed to connect to the file server\n");
        send_error_response(client_fd, BAD_GATEWAY, "Bad Gateway");
        return;
    }

    // successfully connected to the file server
    char *buffer = (char *)malloc(RESPONSE_BUFSIZE * sizeof(char));

    // forward the client request to the fileserver
    int bytes_read = read(client_fd, buffer, RESPONSE_BUFSIZE);
    int ret = http_send_data(fileserver_fd, buffer, bytes_read);
    if (ret < 0) {
        printf("Failed to send request to the file server\n");
        send_error_response(client_fd, BAD_GATEWAY, "Bad Gateway");

    } else {
        // forward the fileserver response to the client
        while (1) {
            printf("reading bytes\n");
            int bytes_read = recv(fileserver_fd, buffer, RESPONSE_BUFSIZE - 1, 0);
            if (bytes_read <= 0) // fileserver_fd has been closed, break
                break;
            ret = http_send_data(client_fd, buffer, bytes_read);
            if (ret < 0) { // write failed, client_fd has been closed
                break;
            }
        }
    }

    // close the connection to the fileserver
    shutdown(fileserver_fd, SHUT_WR);
    close(fileserver_fd);

    // Free resources and exit
    free(buffer);
}


int *server_fd;

void *listener(void *arg) {
    printf("Entering listener\n");
     // create a socket to listen
    int listening_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (listening_fd == -1) {
        perror("Failed to create a new socket");
        exit(errno);
    }



    // manipulate options for the socket
    int socket_option = 1;
    if (setsockopt(listening_fd, SOL_SOCKET, SO_REUSEADDR, &socket_option,
                   sizeof(socket_option)) == -1) {
        perror("Failed to set socket options");
        exit(errno);
    }


    int listener_port = *((int *)arg);
    // create the full address of this proxyserver
    struct sockaddr_in listener_address;
    memset(&listener_address, 0, sizeof(listener_address));
    listener_address.sin_family = AF_INET;
    listener_address.sin_addr.s_addr = INADDR_ANY;
    listener_address.sin_port = htons(listener_port); // listening port

    printf("Trying to bind socket...\n");
    // bind the socket to the address and port number specified in
    if (bind(listening_fd, (struct sockaddr *)&listener_address,
             sizeof(listener_address)) == -1) {
        perror("Failed to bind on socket");
        exit(errno);
    }

    // starts waiting for the client to request a connection
    if (listen(listening_fd, 1024) == -1) {
        perror("Failed to listen on socket");
        exit(errno);
    }

    printf("Listening on port %d...\n", listener_port);

    struct sockaddr_in client_address;
    size_t client_address_length = sizeof(client_address);
    int client_fd;
    while (1) {
        client_fd = accept(listening_fd,
                           (struct sockaddr *)&client_address,
                           (socklen_t *)&client_address_length);
        if (client_fd < 0) {
            perror("Error accepting socket");
            continue;
        }


        printf("Accepted connection from %s on port %d\n",
               inet_ntoa(client_address.sin_addr),
               client_address.sin_port);
        
        struct http_request req = parse_client_request(client_fd);
        req.fd =  client_fd;
        if(strcmp(req.path, GETJOBCMD) == 0) {
            // getJob code
            
            // Remove highest prio job from queue
            struct element req = get_work_nonblocking();

            // If queue was empty, the returned struct will have a -1 prio, so 
            // check that and return QUEUE_EMPTY error if so
            if(req.prio == -1) {
                send_error_response(client_fd, QUEUE_EMPTY, "Queue is currently empty\n");
            }
            else{

                // int fd = req.fd;

                http_start_response(client_fd, 200);
                http_send_header(client_fd, "Content-Type", "text/html");
                http_end_headers(client_fd);
                http_send_string(client_fd, req.path);
            
            }

            shutdown(client_fd, SHUT_WR);
            close(client_fd);
            continue;


        } else {
            struct element *a = malloc(sizeof(struct element));
            a->delay = req.delay;
            a->fd = req.fd;
            a->method = req.method;
            a->path = req.path;
            a->prio = req.prio;
            if(add_work(*a) == -1) {
                send_error_response(client_fd, QUEUE_FULL, "Queue is full\n");
                shutdown(client_fd, SHUT_WR);
                close(client_fd);
            }


        }

    }
    close(listening_fd);
    pthread_exit(NULL);
}

void *worker(void *arg) {
    while(1){
        printf("Trying to get work\n");
        struct element req = get_work();
        printf("Work retrieved\n");
        if(atoi(req.delay) > 0) {
            printf("Sleeping for %s\n", req.delay);
            sleep(atoi(req.delay));
        }

        serve_request(req.fd);
        shutdown(req.fd, SHUT_WR);
        close(req.fd);
    }

    pthread_exit(NULL);

}

/*
 * opens a TCP stream socket on all interfaces with port number PORTNO. Saves
 * the fd number of the server socket in *socket_number. For each accepted
 * connection, calls request_handler with the accepted fd number.
 */
void serve_forever(int *server_fd) {
    printf("Entering serve_forever\n");
    create_queue(max_queue_size);
    pthread_t listeners[num_listener];
    pthread_t workers[num_workers];
    
    // Init listener threads
    printf("Creating listener threads\n");
    for(int i = 0; i < num_listener; i++) {
        pthread_create(&listeners[i], NULL, listener, (void *)&listener_ports[i]);
        
    }
    
    printf("Creating worker threads\n");
    // Init worker threads
    for(int i = 0; i < num_workers; i++) {
        pthread_create(&workers[i], NULL, worker, NULL);
    }

    printf("Joining listener threads\n");
    // Join listener threads
    for(int i = 0; i < num_listener; i++) {
        pthread_join(listeners[i], NULL);
    }
    
    printf("Joining worker threads\n");
    // Join worker threads
    for(int i = 0; i < num_workers; i++) {
        pthread_join(workers[i], NULL);
    }
    

    
}

/*
 * Default settings for in the global configuration variables
 */
void default_settings() {
    num_listener = 1;
    listener_ports = (int *)malloc(num_listener * sizeof(int));
    listener_ports[0] = 8000;

    num_workers = 1;

    fileserver_ipaddr = "127.0.0.1";
    fileserver_port = 3333;

    max_queue_size = 100;
}

void print_settings() {
    printf("\t---- Setting ----\n");
    printf("\t%d listeners [", num_listener);
    for (int i = 0; i < num_listener; i++)
        printf(" %d", listener_ports[i]);
    printf(" ]\n");
    printf("\t%d workers\n", num_listener);
    printf("\tfileserver ipaddr %s port %d\n", fileserver_ipaddr, fileserver_port);
    printf("\tmax queue size  %d\n", max_queue_size);
    printf("\t  ----\t----\t\n");
}

void signal_callback_handler(int signum) {
    printf("Caught signal %d: %s\n", signum, strsignal(signum));
    for (int i = 0; i < num_listener; i++) {
        if (close(*server_fd) < 0) perror("Failed to close server_fd (ignoring)\n");
    }
    free(listener_ports);
    exit(0);
}

char *USAGE =
    "Usage: ./proxyserver [-l 1 8000] [-n 1] [-i 127.0.0.1 -p 3333] [-q 100]\n";

void exit_with_usage() {
    fprintf(stderr, "%s", USAGE);
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    signal(SIGINT, signal_callback_handler);

    /* Default settings */
    default_settings();

    server_fd = malloc(sizeof(int) * num_listener);

    int i;
    for (i = 1; i < argc; i++) {
        if (strcmp("-l", argv[i]) == 0) {
            num_listener = atoi(argv[++i]);
            free(listener_ports);
            listener_ports = (int *)malloc(num_listener * sizeof(int));
            for (int j = 0; j < num_listener; j++) {
                listener_ports[j] = atoi(argv[++i]);
            }
        } else if (strcmp("-w", argv[i]) == 0) {
            num_workers = atoi(argv[++i]);
        } else if (strcmp("-q", argv[i]) == 0) {
            max_queue_size = atoi(argv[++i]);
        } else if (strcmp("-i", argv[i]) == 0) {
            fileserver_ipaddr = argv[++i];
        } else if (strcmp("-p", argv[i]) == 0) {
            fileserver_port = atoi(argv[++i]);
        } else {
            fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
            exit_with_usage();
        }
    }
    print_settings();
    printf("Calling serve_forever\n");
    serve_forever(server_fd);

    return EXIT_SUCCESS;
}

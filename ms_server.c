
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "ms.h"
#include "utils.h"

/* Struct of a single connection request */
typedef struct conn_req conn_req_t;
struct conn_req{
    int socket_fd;
    ms_user user;
    conn_req_t* next;
};

/* Struct of a single scoreboard entry */
typedef struct scoreboard_entry scoreboard_entry_t;
struct scoreboard_entry{
    int seconds_taken;
    ms_user user;
    scoreboard_entry_t* next;
};

/* macOS has different mutex initializers */
#ifdef __APPLE__
pthread_mutex_t request_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
pthread_mutex_t scoreboard_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
#else
pthread_mutex_t request_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
pthread_mutex_t scoreboard_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif

/* Condition to signal unhandled requests waiting */
pthread_cond_t requests_outstanding = PTHREAD_COND_INITIALIZER;

/* Pointers for connection requests linked list */
int conn_reqs_num = 0;
conn_req_t* conn_reqs = NULL;
conn_req_t* conn_reqs_last = NULL;

/* Pointers for scoreboard entry linked list */
int scoreboard_entry_num = 0;
scoreboard_entry_t* scoreboard_entries = NULL;
scoreboard_entry_t* scoreboard_entries_last = NULL;

int listen_socket_fd;

/* Function definitions */

void add_conn_req(int socket_fd, ms_user user);
req_t request_valid(ms_game_t game, coord_req_t request);
conn_req_t* get_conn_req();
void handle_conn_req(conn_req_t conn_request);
void handle_conn_reqs_loop(void* data);
coord_req_t receive_user_req(int socket_fd);
req_t verify_user(ms_user user);
void close_socket(int socket_fd);
void send_game(int socket_fd, ms_game_t game);
void send_response(int socket_fd, req_t response);

int main(int argc, char* argv[]){

    int i;
    struct sockaddr_in server_addr;

    srand(42);

    /* Thread IDs */
    int thread_ids[QUEUE_SIZE];
    /* Thread structures */
    pthread_t p_threads[QUEUE_SIZE];
    /* Thread attributes */
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    
    /* Choose port */
    switch(argc){
        //Server will use default port
        case 1:
            server_addr.sin_port = htons(DEFAULT_PORT);
            break;
        //Server will use custom port
        case 2:
            server_addr.sin_port = htons(atoi(argv[1]));
            break;
        //Too many args
        default:
            printf("\nUsage --> %s [port]\n\n",argv[0]);
            exit(1);
    }

    /* Create socket */
	if ((listen_socket_fd = socket(AF_INET, SOCK_STREAM, PF_UNSPEC)) == ERROR) {
		perror("Creating socket");
		exit(EXIT_FAILURE);
	}

    /* Bind socket to server port */
	if (bind(listen_socket_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == ERROR) {
		perror("Binding socket");
		exit(EXIT_FAILURE);
	}

    clear_screen();

    int cols = 64;
    print_line(cols);
    printf("Starting the Minesweeper server on port %d\n",ntohs(server_addr.sin_port));
    print_line(cols);
    fflush(stdout);

    /* Start listening on socket port */
	if (listen(listen_socket_fd, QUEUE_SIZE) == ERROR) {
		perror("Listening on socket");
		exit(1);
	}

    /* Create threads */
    for (i=0;i<QUEUE_SIZE;i++){
        thread_ids[i] = i;
        pthread_create(&p_threads[i], &attr, (void*) handle_conn_reqs_loop, &thread_ids[i]);
    }
    
    /* Listen for connections and add to queue */
    while (true){
        struct sockaddr_in client_addr;
        socklen_t sin_size = sizeof(struct sockaddr_in);

        int user_fd;
        ms_user user;

        if ((user_fd = accept(listen_socket_fd, (struct sockaddr *)&client_addr, &sin_size)) == ERROR){
            perror("Accepting message");
            continue;
        }

        /* Receive credentials */
        if (recv(user_fd, &user, sizeof(ms_user), PF_UNSPEC) == ERROR){
            perror("Receiving user credentials");
        }

        switch(verify_user(user)){
            req_t response;
            case valid:
                response = valid;
                if (send(user_fd, &response, sizeof(int), PF_UNSPEC) == ERROR){
                    perror("Sending login response");
                }
                break;
            case invalid:
                response = invalid;
                if (send(user_fd, &response, sizeof(int), PF_UNSPEC) == ERROR){
                    perror("Sending login response");
                }
                close_socket(user_fd);
                continue;
            default:
                break;
        }

        printf("\nConnection from %s @ %s. ", user.username, inet_ntoa(client_addr.sin_addr));

        add_conn_req(user_fd,user);
        printf("Added user to queue.\n");
    }

    return 0;
}

void add_conn_req(int socket_fd, ms_user user){
    conn_req_t* request;

    request = (conn_req_t*)malloc(sizeof(conn_req_t));
    if (!request){
        perror("System has run out of memory");
        return;
    }

    request->socket_fd = socket_fd;
    request->user = user;
    request->next = NULL;

    /* Lock request mutex */
    pthread_mutex_lock(&request_mutex);

    if (conn_reqs_num== 0){
        conn_reqs = request;
        conn_reqs_last = request;
    } else {
        conn_reqs_last->next = request;
        conn_reqs_last = request;
    }

    conn_reqs_num++;

    /* Unlock request mutex */
    pthread_mutex_unlock(&request_mutex);

    /* Signal that there is now a request outstanding */
    pthread_cond_signal(&requests_outstanding);
}

conn_req_t* get_conn_req(){

    conn_req_t* request;

    /* Lock request mutex */
    pthread_mutex_lock(&request_mutex);

    if (conn_reqs_num > 0){
        request = conn_reqs;
        conn_reqs = conn_reqs->next;

        /* The request was the last of the list */
        if (conn_reqs == NULL){
            conn_reqs_last = NULL;
        }

        conn_reqs_num--;
    } else {
        /* The requests list is empty */
        request = NULL;
    }

    /* Unlock request mutex */
    pthread_mutex_unlock(&request_mutex);

    return request;

}

void handle_conn_reqs_loop(void* data){

    int thread_id = *((int *)data);
    conn_req_t* request;

    pthread_mutex_lock(&request_mutex);

    while(true){
        if (conn_reqs_num > 0){
            request = get_conn_req();

            if (request){
                pthread_mutex_unlock(&request_mutex);
                printf("%s now playing in Game Room #%d\n", request->user.username, thread_id+1);
                fflush(0);
                handle_conn_req(*request);
                free(request);
                pthread_mutex_lock(&request_mutex);
            }
        } else {
            pthread_cond_wait(&requests_outstanding, &request_mutex);
        }
    }

}

void handle_conn_req(conn_req_t conn_request){

    /* User information for this session */
    ms_game_t game = new_game(rand());

    bool ingame = true;

    while (ingame){
        coord_req_t request = receive_user_req(conn_request.socket_fd);
        switch (request.request_type){
            req_t response;
            case reveal:
                if (request_valid(game, request) == valid){
                    reveal_tile(&game,request.x,request.y);
                    response = valid;
                    send_response(conn_request.socket_fd,response);
                } else {
                    response = invalid;
                    send_response(conn_request.socket_fd,response);
                }
                break;
            case flag:
                if (request_valid(game, request) == valid){
                    flag_tile(&game,request.x,request.y);
                    response = valid;
                    send_response(conn_request.socket_fd,response);
                } else {
                    response = invalid;
                    send_response(conn_request.socket_fd,response);
                }
                break;
            case gameboard:
                send_game(conn_request.socket_fd, game);
                break;
            case leaderboard:
                break;
            case lost:
                ingame = false;
                response = valid;
                send_response(conn_request.socket_fd,response);
                break;
            case quit:
                return;
            default:
                break;
        }
    }
    
}

req_t request_valid(ms_game_t game, coord_req_t request){

    switch (request.request_type){
        case reveal:
            if (!location_valid(&game, request.x, request.y)){
                return invalid;
            }
            if (location_revealed(&game, request.x, request.y)){
                return invalid;
            }
            if (location_flagged(&game,request.x,request.y)){
                return invalid;
            }
            return valid;
        case flag:
            if (!location_valid(&game, request.x, request.y)){
                return invalid;
            }
            if (location_revealed(&game, request.x, request.y)){
                return invalid;
            }
            return valid;
        default:
            return invalid;
    }

}

void send_response(int socket_fd, req_t response){
    if (send(socket_fd, &response, sizeof(req_t), PF_UNSPEC) == ERROR){
        perror("Sending request response");
    }
}

void send_game(int socket_fd, ms_game_t game){

    int cols = MS_COLS;
    int rows = MS_ROWS;

    if (send(socket_fd, &cols, sizeof(int), PF_UNSPEC) == ERROR){
        perror("Sending data packet size");
    }

    if (send(socket_fd, &rows, sizeof(int), PF_UNSPEC) == ERROR){
        perror("Sending data packet size");
    }

    int x,y;
    uint16_t value;

    for (y=0;y<rows;y++){
        for (x=0;x<cols;x++){
            if (game.board[x][y].flagged){
                value = htons(FLAG_VAL);
            } else if (game.board[x][y].revealed){
                if (game.board[x][y].bomb){
                    value = htons(BOMB_VAL);
                } else {
                    value = htons(game.board[x][y].adjacent);
                }
            } else {
                value = htons(UNSELECTED_VAL);
            }

            if (send(socket_fd, &value, sizeof(uint16_t),PF_UNSPEC) == ERROR){
                perror("Sending game values");
                continue;
            };
        }
    }

}

coord_req_t receive_user_req(int socket_fd){
    coord_req_t request;
    if (recv(socket_fd, &request, sizeof(coord_req_t),PF_UNSPEC) == ERROR){
        perror("Receiving user coord request");
    }
    return request;
}

req_t verify_user(ms_user user){
    return valid;
}

void close_socket(int socket_fd){
    shutdown(socket_fd, SHUT_RDWR);
    close(socket_fd);
}
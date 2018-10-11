#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> 
#include <unistd.h>

#include <signal.h>

#include "ms.h"
#include "utils.h"

//If we can do reverse to get ip from sock addr, can have "IPADDRESS is now playing in lobby threadid+1"

//validate user

/* macOS has different mutex initializers */
#ifdef __APPLE__
pthread_mutex_t request_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
pthread_mutex_t scoreboard_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
#else
pthread_mutex_t request_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
pthread_mutex_t scoreboard_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif

/* Condition to signal if there are unhandled requests waiting */
pthread_cond_t requests_outstanding = PTHREAD_COND_INITIALIZER;

/* Struct of a single request */
typedef struct conn_request conn_request;
struct conn_request{
    int socket_fd;
    ms_user user;
    conn_request* next;
};
conn_request* conn_requests = NULL;
conn_request* last_conn_request = NULL;
int conn_request_num = 0;

/* Struct of a single scoreboard entry */
typedef struct scoreboard_entry scoreboard_entry;
struct scoreboard_entry{
    int seconds_taken;
    ms_user user;
    scoreboard_entry* next;
};
scoreboard_entry* scoreboard_list;

int verify_user(ms_user user);
void exit_gracefully();

void send_game(int socket_fd, ms_game game);

ms_coord_req receive_user_request(int socket_fd);

void add_conn_request(int socket_fd, ms_user user);
struct request* get_conn_request();
void handle_conn_request(conn_request request);
void handle_conn_requests_loop(void* data);

int main(int argc, char* argv[]){

    signal(SIGINT, exit_gracefully);
    signal(SIGPIPE, SIG_IGN); //TODO: This dont work
    srand(42);

    /* Thread IDs */
    int thread_ids[QUEUE_SIZE];
    /* Thread structures */
    pthread_t p_threads[QUEUE_SIZE];
    /* Thread attributes */
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    int i;

    int socket_fd;
    struct sockaddr_in server_addr;
    
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
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, PF_UNSPEC)) == ERROR) {
		perror("Creating socket");
		exit(1);
	}

    /* Bind socket to server port */
	if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == ERROR) {
		perror("Binding socket");
		exit(1);
	}

    clear_screen();

    int cols = 64;
    print_line(cols);
    printf("Starting the Minesweeper server on port %d\n",ntohs(server_addr.sin_port));
    print_line(cols);
    fflush(0);

    /* Start listening on socket port */
	if (listen(socket_fd, QUEUE_SIZE) == ERROR) {
		perror("Listening on socket");
		exit(1);
	}

    /* Create threads */
    for (i=0;i<QUEUE_SIZE;i++){
        thread_ids[i] = i;
        pthread_create(&p_threads[i], &attr, handle_conn_requests_loop, &thread_ids[i]);
    }

    while(true){

        struct sockaddr_in client_addr;
        socklen_t sin_size = sizeof(struct sockaddr_in);
        int user_fd;

        if ((user_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &sin_size)) == ERROR){
            perror("Accepting message");
            continue;
        }

        ms_user user;

        /* Receive credentials */
        if (recv(user_fd, &user, sizeof(ms_user), PF_UNSPEC) == ERROR){
            perror("Receiving user credentials");
        }

        /* User verification */
        switch (verify_user(user)){
            int response;
            case ACCEPTED:
                response = ACCEPTED;
                if (send(user_fd, &response, sizeof(int), PF_UNSPEC) == ERROR){
                    perror("Sending login response");
                }
                break;
            case REJECTED:
                response = REJECTED;
                if (send(user_fd, &response, sizeof(int), PF_UNSPEC) == ERROR){
                    perror("Sending login response");
                    shutdown(socket_fd, SHUT_RDWR);
                    close(socket_fd);
                    continue;
                }
                break;
        }

        printf("\nConnection from %s @ %s. ", user.username, inet_ntoa(client_addr.sin_addr));

        add_conn_request(user_fd,user);
        printf("Added user to queue.\n");
    }

    return 0;
}

void send_game(int socket_fd, ms_game game){
    #define game(x,y) game.board[x][y]

    int cols = MS_COLS;
    int rows = MS_ROWS;

    if (send(socket_fd, &cols, sizeof(int),0) == ERROR){
        perror("Sending data packet size");
    }

    if (send(socket_fd, &rows, sizeof(int),0) == ERROR){
        perror("Sending data packet size");
    }

    int x,y;
    uint16_t value;

    for (y=0;y<rows;y++){
        for (x=0;x<cols;x++){
            if (game(x,y).flagged){
                value = htons(FLAG_VAL);
            } else if (game(x,y).revealed){
                if (game(x,y).bomb){
                    value = htons(BOMB_VAL);
                } else {
                    value = htons(game(x,y).adjacent);
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

void add_conn_request(int socket_fd, ms_user user){
    conn_request* request;

    request = (conn_request*)malloc(sizeof(conn_request));
    if (!request){
        //TODO:error("System has run out of memory");
        return;
    }

    request->socket_fd = socket_fd;
    request->user = user;
    request->next = NULL;

    /* Lock request mutex */
    pthread_mutex_lock(&request_mutex);

    if (conn_request_num == 0){
        conn_requests = request;
        last_conn_request = request;
    } else {
        last_conn_request->next = request;
        last_conn_request = request;
    }


    conn_request_num++;

    /* Unlock request mutex */
    pthread_mutex_unlock(&request_mutex);

    /* Signal that there is now a request outstanding */
    pthread_cond_signal(&requests_outstanding);
}

struct request* get_conn_request(){

    conn_request* request;

    /* Lock request mutex */
    pthread_mutex_lock(&request_mutex);

    if (conn_request_num > 0){
        request = conn_requests;
        conn_requests = conn_requests->next;

        /* Was the last request of the list */
        if (conn_requests == NULL){
            last_conn_request == NULL;
        }

        conn_request_num--;
    } else {
        /* The requests list is empty */
        request = NULL;
    }

    /* Unlock request mutex */
    pthread_mutex_unlock(&request_mutex);

    return request;

}

void handle_conn_requests_loop(void* data){

    int thread_id = *((int *)data);
    conn_request* request;

    pthread_mutex_lock(&request_mutex);

    while(true){
        if (conn_request_num > 0){
            request = get_conn_request();

            printf("%s now playing in Game Room #%d\n", request->user.username, thread_id+1);
            fflush(0);

            if (request){
                pthread_mutex_unlock(&request_mutex);
                handle_conn_request(*request);
                free(request);
                pthread_mutex_lock(&request_mutex);
            }
        } else {
            pthread_cond_wait(&requests_outstanding, &request_mutex);
        }
    }

}

void handle_conn_request(conn_request request){

    /* User information for this session */
    ms_game game;
    int socket_fd = request.socket_fd;

    bool game_in_progress = true;
    bool game_init = false;

    game = new_game(rand());

    while (game_in_progress){
        ms_coord_req request = receive_user_request(socket_fd);
        switch (request.type){
            case reveal:
                reveal_tile(&game,request.x,request.y);
                break;
            case flag:
                flag_tile(&game, request.x, request.y);
                break;
            case game_board:
                send_game(socket_fd, game);
                break;
            case leaderboard:
                break;
        }
    }
}

ms_coord_req receive_user_request(int socket_fd){
    ms_coord_req request;
    if (recv(socket_fd, &request, sizeof(ms_coord_req),PF_UNSPEC) == ERROR){
        perror("Receiving user coord request");
    }
    return request;
}

int verify_user(ms_user user){
    //TODO:FILE CHECK
    return 1;
}

void exit_gracefully(){
    //TODO: Exit gracefully
}
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Minesweeper definitions */
#include "ms.h"
/* Utility definitions */
#include "utils.h"

/* Struct of a single connection request */
typedef struct conn_req conn_req_t;
struct conn_req{
    int socket_fd;
    ms_user_t user;
    conn_req_t* next;
};

/* Struct of a user currently logged in */
typedef struct ms_user_current ms_user_current_t;
struct ms_user_current{
    ms_user_t user;
    ms_user_current_t* next;
};

/* Pointer for currently logged in users linked lsit */
ms_user_current_t* current_users = NULL;

/* Pointers for connection requests linked list */
int conn_reqs_num = 0;
conn_req_t* conn_reqs = NULL;
conn_req_t* conn_reqs_last = NULL;

/* Pointer for user histories linked list */
ms_user_history_entry_t* user_histories = NULL;

/* Pointer for scoreboard entry linked list */
int scoreboard_entry_num = 0;
scoreboard_entry_t *scoreboard_entries = NULL;
scoreboard_entry_t *scoreboard_entries_last = NULL;

/* macOS has different mutex initializers */
#ifdef __APPLE__
pthread_mutex_t request_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
pthread_mutex_t scoreboard_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
pthread_mutex_t current_users_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
#else
pthread_mutex_t request_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
pthread_mutex_t scoreboard_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
pthread_mutex_t current_users_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif

/* Condition to signal unhandled requests waiting */
pthread_cond_t requests_outstanding = PTHREAD_COND_INITIALIZER;

/* Thread structures */
pthread_t p_threads[QUEUE_SIZE];

/* Socket for the server to listen on */
int listen_socket_fd;

/* Function definitions */
void add_conn_req(int socket_fd, ms_user_t user);
void add_loss(ms_user_t user);
void add_score(ms_user_t user, int score);
void close_server();
void close_socket(int socket_fd);
void handle_conn_req(conn_req_t conn_request);
void handle_conn_reqs_loop(void* data);
void scoreboard_swap(scoreboard_entry_t *a, scoreboard_entry_t *b);
void send_game(int socket_fd, ms_game_t game);
void send_response(int socket_fd, req_t response);
void send_scoreboard(int socket_fd);
void sort_leaderboard();
void user_login(ms_user_t user);
void user_logout(ms_user_t user);

bool user_logged_in(ms_user_t user);

req_t request_valid(ms_game_t game, coord_req_t request);
req_t verify_user(ms_user_t user);

conn_req_t* get_conn_req();

coord_req_t receive_user_req(int socket_fd);

ms_user_history_entry_t* find_user_history(ms_user_t user);


/***********************************************************************
 * func:            Entry point of the program.
***********************************************************************/
int main(int argc, char* argv[]){

    int i;
    struct sockaddr_in server_addr;

    signal(SIGINT, close_server);

    srand(42);

    /* Thread IDs */
    int thread_ids[QUEUE_SIZE];
    
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
            exit(EXIT_FAILURE);
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
        ms_user_t user;

        if ((user_fd = accept(listen_socket_fd, (struct sockaddr *)&client_addr, &sin_size)) == ERROR){
            perror("Accepting message");
            continue;
        }

        /* Receive credentials */
        if (recv(user_fd, &user, sizeof(ms_user_t), PF_UNSPEC) == ERROR){
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
                shutdown(user_fd,SHUT_RDWR);
                close_socket(user_fd);
                continue;
            default:
                break;
        }

        printf("\nConnection from %s @ %s. ", user.username, inet_ntoa(client_addr.sin_addr));

        user_login(user);
        add_conn_req(user_fd,user);

        printf("Added user to queue.\n");
    }

    return 0;
}

/***********************************************************************
 * func:            Adds a given connection request to the connections
 *                  linked list.
 * 
 * param socket_fd: The socket file descriptor of the incoming
 *                  connection request.
 * 
 * param user:      The verified user that belongs to the request.
************************************************************************/
void add_conn_req(int socket_fd, ms_user_t user){
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

    if (conn_reqs_num == 0){
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

/***********************************************************************
 * func:            Returns the head of the connection request linked
 *                  list and removes it from the linked list.
***********************************************************************/
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

/***********************************************************************
 * func:            A loop function which waits for outstanding
 *                  requests. If there is an outstanding request, the
 *                  request will be sent to a handler function.
 * param data:      The data produced by creating a thread.
***********************************************************************/
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
                printf("\n%s has left Game Room #%d\n", request->user.username, thread_id+1);
                user_logout(request->user);
                free(request);
                pthread_mutex_lock(&request_mutex);
            }
        } else {
            pthread_cond_wait(&requests_outstanding, &request_mutex);
        }
    }

}

/***********************************************************************
 * func:            The function to handle a request. It handles client
 *                  requests, updating its internal game state, and
 *                  sends the respective response.
 * param conn_req:  The connection request to handle. This contains
 *                  the socket information and user information of the
 *                  request.
***********************************************************************/
void handle_conn_req(conn_req_t conn_req){

    /* User information for this session */
    ms_game_t game = new_game(rand());

    bool connected = true;
    time_t start,end;

    bool timer_started = false;

    while (connected){
        coord_req_t request = receive_user_req(conn_req.socket_fd);
        switch (request.request_type){
            req_t response;
            case reveal:
                if (request_valid(game, request) == valid){
                    req_t reveal_response = reveal_tile(&game,request.x,request.y);
                    if (reveal_response == lost){
                        response = lost;
                        send_response(conn_req.socket_fd,response);
                        break;
                    } else if (reveal_response == won){
                        response = won;
                        send_response(conn_req.socket_fd, response);
                        end = time(NULL);
                        int time_taken = end-start;
                        add_score(conn_req.user,time_taken);
                        game = new_game(rand());
                        timer_started = false;
                        break;
                    }
                    response = valid;
                    send_response(conn_req.socket_fd,response);
                } else {
                    response = invalid;
                    send_response(conn_req.socket_fd,response);
                }
                break;
            case flag:
                if (request_valid(game, request) == valid){
                    req_t reveal_response = flag_tile(&game,request.x,request.y);
                    if (reveal_response == won){
                        response = won;
                        send_response(conn_req.socket_fd, response);
                        end = time(NULL);
                        int time_taken = end-start;
                        add_score(conn_req.user,time_taken);
                        game = new_game(rand());
                        timer_started = false;
                        break;
                    }
                    response = valid;
                    send_response(conn_req.socket_fd,response);
                } else {
                    response = invalid;
                    send_response(conn_req.socket_fd,response);
                }
                break;
            case gameboard:
                if (!timer_started){
                    start = time(NULL);
                    timer_started = true;
                }
                send_game(conn_req.socket_fd, game);
                break;
            case scoreboard:
                send_scoreboard(conn_req.socket_fd);
                break;
            case lost: 
                timer_started = false;
                game = new_game(rand());
                response = valid;
                send_response(conn_req.socket_fd,response);
                add_loss(conn_req.user);
                break;
            case quit:
                close_socket(conn_req.socket_fd);
                return;
            default:
                break;
        }
    }
    
}

/***********************************************************************
 * func:            A function used to validate if a request is valid
 *                  based on the parameters of the board and its
 *                  current state.
 * param game:      The game board state to check.
 * param request:   The request to validate.
***********************************************************************/
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

/***********************************************************************
 * func:            A function used to send a response to a given
 *                  socket connection.
 * param socket_fd: The socket file descriptor of the desired
 *                  connection to send to.
 * param response:  The response to send.
***********************************************************************/
void send_response(int socket_fd, req_t response){
    if (send(socket_fd, &response, sizeof(req_t), PF_UNSPEC) == ERROR){
        perror("Sending request response");
    }
}

/***********************************************************************
 * func:            A function used to send a game state to a given
 *                  socket connection.
 * param socket_fd: The socket file descriptor of the desired
 *                  connection to send to.
 * param game:      The game state to send.
***********************************************************************/
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
            };
        }
    }

    value = htons(bombs_remaining(&game));
    if (send(socket_fd, &value, sizeof(uint16_t), PF_UNSPEC) == ERROR){
        perror("Sending bombs remaining");
    }

}

/***********************************************************************
 * func:            A function used to recieve and then return a users
 *                  coordinate request.
 * param socket_fd: The socket file descriptor of the desired
 *                  connection to recieve from.
***********************************************************************/
coord_req_t receive_user_req(int socket_fd){
    coord_req_t request;

    if (recv(socket_fd, &request, sizeof(coord_req_t),PF_UNSPEC) == ERROR){
        perror("Receiving user coord request");
    }

    return request;
}

/***********************************************************************
 * func:            A function used to validate a user based on the
 *                  usernames and passwords stored in the file
 *                  "Authentication.txt".
 * param user:      The user to validate.
***********************************************************************/
req_t verify_user(ms_user_t user){

    if (user_logged_in(user)){
        return invalid;
    }
    
    FILE *auth_file = fopen("Authentication.txt","r");
    char buffer[256];

    int i = 0;

    if (!auth_file){
        printf("No authentication file found\n");
        return invalid;
    }

    while (fgets(buffer,256, auth_file)!=NULL)
        /* Dont column headers */
        if (i==0){
            i++;
        } else {
            char *token = strtok(buffer,"\t\n\r ");
            /* If the username is correct, then check the password */
            if (strcmp(user.username,token) == 0){
                token = strtok(NULL, "\t\n\r ");
                if (strcmp(user.password,token) == 0){
                    return valid;
                }
            }
        }	

    return invalid;
}

/***********************************************************************
 * func:            A function used to add a loss to a given users
 *                  history.
 * param user:      The user to add a loss to.
***********************************************************************/
void add_loss(ms_user_t user){
    /* Lock scoreboard mutex */
    pthread_mutex_lock(&scoreboard_mutex);

    ms_user_history_entry_t* pointer = find_user_history(user);
    pointer->user.lost++;

    /* Unlock scoreboard mutex */
    pthread_mutex_unlock(&scoreboard_mutex);
}


/***********************************************************************
 * func:            A function used to find a specified user in the
 *                  user history linked list.
 * param user:      The specified user to retrieve.
***********************************************************************/
ms_user_history_entry_t* find_user_history(ms_user_t user){
    ms_user_history_entry_t* pointer = user_histories;

    if (pointer == NULL){
        pointer = malloc(sizeof(ms_user_history_entry_t));
        pointer->user.user = user;
        pointer->user.won = 0;
        pointer->user.lost = 0;
        user_histories = pointer;
        return pointer;
    }

    for(;pointer!=NULL;pointer=pointer->next){
        if (strcmp(user.username, pointer->user.user.username) == 0){
            return pointer;
        }
        if (pointer->next == NULL){
            pointer->next = malloc(sizeof(ms_user_history_entry_t));
            pointer->next->user.user = user;
            pointer->next->user.won = 0;
            pointer->next->user.lost = 0;
            return pointer->next;
        }
    }

    return NULL;
}

/***********************************************************************
 * func:            A function used to add a score to the servers
 *                  scoreboard.
 * param user:      The user associated with the achieved score.
 * param score:     The time taken to complete the game.
***********************************************************************/
void add_score(ms_user_t user, int score){

    /* Lock scoreboard mutex */
    pthread_mutex_lock(&scoreboard_mutex);

    /* Add a win to user history */
    ms_user_history_entry_t* uh_pointer = find_user_history(user);
    uh_pointer->user.won++;
    
    /* Add time to scoreboard */
    scoreboard_entry_t* entry = malloc(sizeof(scoreboard_entry_t));

    entry->seconds_taken = score;
    entry->user = user;

    if (scoreboard_entry_num== 0){
        scoreboard_entries = entry;
        scoreboard_entries_last = entry;
    } else {
        scoreboard_entries_last->next = entry;
        scoreboard_entries_last = entry;
    }

    sort_leaderboard();

    /* Unlock scoreboard mutex */
    pthread_mutex_unlock(&scoreboard_mutex);

    scoreboard_entry_num++;
    printf("\nTime of %d added\n", score);

}

/***********************************************************************
 * func:            A function used to properly terminate a given
 *                  socket.
 * param socket_fd: The socket_fd of the required socket to be 
 *                  terminated.
***********************************************************************/
void close_socket(int socket_fd){
    shutdown(socket_fd,SHUT_RDWR);
    close(socket_fd);
}

/***********************************************************************
 * func:            A function used to properly close the server state.
***********************************************************************/
void close_server(){
    printf("\nServer is shutting down now...\n");
    shutdown(listen_socket_fd, SHUT_RDWR);
    close(listen_socket_fd);
    exit(EXIT_SUCCESS);
}

/***********************************************************************
 * func:            A function used to send the current scoreboard to
 *                  a given connection.
 * param socket_fd: The socket file descriptor of the specified
 *                  connection.
***********************************************************************/
void send_scoreboard(int socket_fd){
    
    int i;
    scoreboard_entry_t *pointer = scoreboard_entries;

    if (send(socket_fd, &scoreboard_entry_num, sizeof(int), PF_UNSPEC) == ERROR){
        perror("Sending scoreboard size");
    }

    if (scoreboard_entry_num == 0){
        return;
    }
    
    for (i=0;i<scoreboard_entry_num;i++){
        if (send(socket_fd, pointer, sizeof(scoreboard_entry_t), PF_UNSPEC) == ERROR){
            perror("Sending scoreboard entry");
        }
        if (send(socket_fd, find_user_history(pointer->user), sizeof(ms_user_history_entry_t), PF_UNSPEC) == ERROR){
            perror("Receiving scoreboard entry");
        }
        pointer = pointer->next;
    }

}

/***********************************************************************
 * func:            A function used to swap two given scoreboard
 *                  entries in the servers leaderboard state.
 * param a:         A scoreboard entry to be swapped.
 * param b:         A scoreboard entry to be swapped.
***********************************************************************/
void scoreboard_swap(scoreboard_entry_t *a, scoreboard_entry_t *b){
    ms_user_t user_temp = a->user;
    int time_temp = a->seconds_taken;

    a->user = b->user;
    a->seconds_taken = b->seconds_taken;

    b->user = user_temp;
    b->seconds_taken = time_temp;
}

/***********************************************************************
 * func:            A function used to sort the leaderboard, with
 *                  regards to predefined specifications. This is
 *                  achieved using a Bubblesort algorithm.
***********************************************************************/
void sort_leaderboard(){

    int swapped;
    scoreboard_entry_t* ptr1;
    scoreboard_entry_t* last = NULL;

    if (scoreboard_entries == NULL){
        return;
    }

    do {
        swapped = 0;
        ptr1 = scoreboard_entries;

        while (ptr1->next != last){
            if (ptr1->seconds_taken < ptr1->next->seconds_taken){
                scoreboard_swap(ptr1, ptr1->next);
                swapped = 1;
            } else if (ptr1->seconds_taken == ptr1->next->seconds_taken){
                if (find_user_history(ptr1->user)->user.won > find_user_history(ptr1->next->user)->user.won){
                    scoreboard_swap(ptr1, ptr1->next);
                    swapped = 1;
                } else if (strcasecmp(ptr1->user.username,ptr1->next->user.username) > 0){
                    scoreboard_swap(ptr1, ptr1->next);
                    swapped = 1;
                }
            }
            ptr1 = ptr1->next;
        }
        last = ptr1;
    } while (swapped);

}

/***********************************************************************
 * func:            A function used to determine whether a given user
 *                  is currently logged into the server.
 * param user:      The specified user to check.
***********************************************************************/
bool user_logged_in(ms_user_t user){

    /* Lock current users mutex */
    pthread_mutex_lock(&current_users_mutex);

    ms_user_current_t *pointer = current_users;

    while (pointer!=NULL){
        if (strcmp(user.username, pointer->user.username) == 0){
            /* Lock current users mutex */
            pthread_mutex_unlock(&current_users_mutex);
            return true;
        }
        pointer = pointer->next;
    }

    /* Lock current users mutex */
    pthread_mutex_unlock(&current_users_mutex);
    return false;
}

/***********************************************************************
 * func:            A function used to log a user into the systems
 *                  linked list, so that their connection state may be
 *                  monitored throughout the session.
 * param user:      The specified user to log in.
***********************************************************************/
void user_login(ms_user_t user){

    /* Lock current users mutex */
    pthread_mutex_lock(&current_users_mutex);

    ms_user_current_t *pointer = malloc(sizeof(ms_user_current_t));

    pointer->user = user;
    pointer->next = current_users;

    current_users = pointer;

    /* Unlock current users mutex */
    pthread_mutex_unlock(&current_users_mutex);

}

/***********************************************************************
 * func:            A function used to log a user out of the systems
 *                  connection linked list.
 * param user:      The specified user to log out.
***********************************************************************/
void user_logout(ms_user_t user){

    ms_user_current_t *current = current_users;
    ms_user_current_t *delete = NULL;

    /* Lock current users mutex */
    pthread_mutex_lock(&current_users_mutex);

    if (strcmp(current->user.username, user.username) == 0){
        current_users = current->next;
        free(current);
        pthread_mutex_unlock(&current_users_mutex);
        return;
    }

    while (current != NULL){
        if (strcmp(current->next->user.username, user.username) == 0){
            delete = current->next;
            if (delete->next == NULL){
                current->next = NULL;
            } else {
                current->next = delete->next;
            }
            free(delete);
            pthread_mutex_unlock(&current_users_mutex);
            return;
        }
        current = current->next;
    }

    /* Unlock current users mutex */
    pthread_mutex_unlock(&current_users_mutex);
}
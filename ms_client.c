#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> 
#include <unistd.h>

/* Utility definitions */
#include "utils.h"

//TODO: keep global count of threads used, if == 10 then queue message
//TODO: Send message if ctrl c so that client quits when server quits <- have threads as global?
//TODO: rand() mutex?

/* Users connection point to server */
int socket_fd;

/* The current user */
ms_user_t session_user;

/* Function definitions */
coord_req_t get_coords();

req_t send_request(coord_req_t request);

int get_menu_choice();

void connect_to_server(char* argv[]);
void exit_gracefully();
void ms_process();
void print_menu(menu_t menu_type);
void recieve_game();
void recieve_scoreboard();
void verify_user();
void welcome_screen();

/***********************************************************************
 * func:            Entry point of the program.
***********************************************************************/
int main(int argc, char* argv[]){

    signal(SIGINT, exit_gracefully);

    /* If incorrect program usage */
    if (argc != 3){
        printf("\nUsage --> %s [hostname] [port]\n\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    clear_screen();

    welcome_screen();
    connect_to_server(argv);
    verify_user();

    int main_menu_choice = 0;

    while (true){
        print_menu(main_menu);
        main_menu_choice = get_menu_choice();

        switch(main_menu_choice){
            /* Play Minesweeper */
            case 1:
                ms_process();
                break;
            /* Show Leaderboard */
            case 2:
                recieve_scoreboard();
                break;
            /* Quit */
            case 3:
                exit_gracefully();
                break;
        }
    }

    return 0;
}

/***********************************************************************
 * func:            A function used to connect to a given server
 *                  address.
 * param argv:      The command line arguments given to the program on
 *                  launch.
***********************************************************************/
void connect_to_server(char* argv[]){

    struct hostent *host;
    struct sockaddr_in server_addr;

    /* Get host info */
    if ((host = gethostbyname(argv[1])) == NULL){
        herror("Host name translation.");
        exit(EXIT_FAILURE);
    }
    /* Create socket */
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, PF_UNSPEC)) == ERROR){
        perror("Creating socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_addr = *(struct in_addr *)host->h_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));

    //TODO: COMMENT THIS BAD BOY
    memset(&server_addr.sin_zero, 0, sizeof(server_addr.sin_zero)/sizeof(server_addr.sin_zero[0]));

    if (connect(socket_fd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_in)) == ERROR){
        printf("Wrong server details or server is offline! Please try again.\n");
        exit(EXIT_FAILURE);
    }

}

/***********************************************************************
 * func:            A function used to represent the minesweeper game
 *                  process. This is the main process of the client,
 *                  with it handling all user input, and sending the
 *                  respective queries and requests to the server.
***********************************************************************/
void ms_process(){
    
    int game_menu_choice = 0;
    bool ingame = true;

    coord_req_t request;
    req_t response = valid;

    while (ingame){

        if (response == valid){
            clear_screen();
            recieve_game();
        } else {
            printf("\nInvalid choice...\n");
        }

        if (game_menu_choice == 0){
            print_menu(game_menu);
            game_menu_choice = get_menu_choice();
        } else {
            switch (game_menu_choice){
                case 1:
                    printf("\n   --> Currently in reveal mode");
                    break;
                case 2:
                    printf("\n   --> Currently in flag mode");
                    break;
            }
            printf("\n 0 --> Change mode\n");
        }

        switch (game_menu_choice){
            request.x = -1; /* Reset request state */
            request.y = -1; /* Reset request state */
            /* Place tile */
            case 1:
                request = get_coords();
                if (request.x < 0){
                    request.y = -1;
                    game_menu_choice = 0;
                    response = valid;
                    break;
                }
                request.request_type = reveal;
                response = send_request(request);
                if (response == lost){
                    recieve_game();
                    printf("\nYou've hit a bomb! Game over!\n");
                    request.request_type = lost;
                    send_request(request);
                    return;
                } else if (response == won){
                    recieve_game();
                    printf("\nCongratulations %s, you have won!\nYour score has been added to the scoreboard.\n", session_user.username);
                    return;
                }
                break;
            /* Place flag */
            case 2:
                request = get_coords();
                if (request.x < 0){
                    request.y = -1;
                    game_menu_choice = 0;
                    response = valid;
                    break;
                }
                request.request_type = flag;
                response = send_request(request);
                break;
            /* Exit */
            case 3:
                ingame = false;
                coord_req_t quit;
                quit.request_type = lost;
                response = send_request(quit);
                ingame = false;
                return;
            default:
                printf("Invalid choice...\n");
                game_menu_choice = 0;
                break;
        }
    }

}

/***********************************************************************
 * func:            A function used to handle user input at the games
 *                  various menus.
***********************************************************************/
int get_menu_choice(){
        char* val = malloc(sizeof(char));
        printf("   --> ");
        scanf("%s", val);
        int value = atoi(&val[0]);
        return value;
}

/***********************************************************************
 * func:            A function used to handle user input when queried
 *                  for a game location.
***********************************************************************/
coord_req_t get_coords(){
        //TODO: take x and y individually
        coord_req_t coord_req;
        printf("X,Y--> ");
        scanf("%d,%d", &coord_req.x, &coord_req.y);
        coord_req.x--;
        coord_req.y--;
        return coord_req;
}

/***********************************************************************
 * func:            A function used to print a given menu type to the
 *                  screen.
 * param menu_type: The specified menu type.
***********************************************************************/
void print_menu(menu_t menu_type){

        printf("\n");
        switch (menu_type){
        case main_menu:
            printf("Choose an option to proceed:\n");
            printf(" 1 --> Play Minesweeper\n");
            printf(" 2 --> Show leaderboard\n");
            printf(" 3 --> Quit\n");
            break;
        case game_menu:
            printf("Select a keyboard mode:\n");
            printf(" 1 --> Reveal a tile\n");
            printf(" 2 --> Place a flag\n");
            printf(" 3 --> Quit\n");
            break;
    }

}

/***********************************************************************
 * func:            A function used to recieve the gameboard from the
 *                  server.
***********************************************************************/
void recieve_game(){
    int x,y;
    int cols, rows;

    uint16_t value;

    coord_req_t request;
    request.request_type = gameboard;

    if (send(socket_fd,&request, sizeof(coord_req_t), PF_UNSPEC) == ERROR){
        perror("Sending game query");
    }

    if (recv(socket_fd, &cols, sizeof(int), PF_UNSPEC) == ERROR){
        perror("Receiving data packet size");
    }

    if (recv(socket_fd, &rows, sizeof(int), PF_UNSPEC) == ERROR){
        perror("Receiving data packet size");
    }

    int values[cols][rows];

    for (y=0;y<rows;y++){
        for (x=0;x<cols;x++){
            if (recv(socket_fd,&value,sizeof(uint16_t), PF_UNSPEC) == ERROR){
                perror("Receiving array");
            }
            values[x][y] = ntohs(value);
        }
    }

    print_game(cols, rows, values);
}

/***********************************************************************
 * func:            A function used to recieve the scoreboard from the
 *                  server.
***********************************************************************/
void recieve_scoreboard(){

    int i, scoreboard_size;
    coord_req_t request;
    ms_user_history_entry_t historyentry;

    request.request_type = scoreboard;

    scoreboard_entry_t *entry = malloc(sizeof(scoreboard_entry_t));

    if (send(socket_fd, &request, sizeof(coord_req_t), PF_UNSPEC) == ERROR){
        perror("Sending scoreboard query");
    }
    
    if (recv(socket_fd, &scoreboard_size, sizeof(int), PF_UNSPEC) == ERROR){
        perror("Recieving scoreboard size");
    }

    printf("\n");
    print_line(64);
    printf("\n");

    if (scoreboard_size == 0){
        printf("Scoreboard is currently empty!\n\n");
        print_line(64);
        fflush(stdout);
        return;
    }

    for (i=0;i<scoreboard_size;i++){
        if (recv(socket_fd, entry, sizeof(scoreboard_entry_t), PF_UNSPEC) == ERROR){
            perror("Receiving scoreboard entry");
        }
        if (recv(socket_fd, &historyentry, sizeof(ms_user_history_entry_t), PF_UNSPEC) == ERROR){
            perror("Receiving scoreboard entry");
        }
        printf("Time of %d seconds by %s.\tUser has won %d of %d games\n", entry->seconds_taken, entry->user.username, historyentry.user.won, historyentry.user.won+historyentry.user.lost);
        fflush(0);
    }

    printf("\n");
    print_line(64);

}

/***********************************************************************
 * func:            A function used to gracefully exit the client and
 *                  properly close the connection with the server.
***********************************************************************/
void exit_gracefully(){
    printf("\n");
    coord_req_t req;
    req.request_type = quit;
    send_request(req);
    shutdown(socket_fd,SHUT_RDWR);
    close(socket_fd);
    exit(EXIT_SUCCESS);
}

/***********************************************************************
 * func:            A function used to send a given request to the
 *                  server.
 * param request:   The specified request.
***********************************************************************/
req_t send_request(coord_req_t request){

    if (send(socket_fd, &request, sizeof(coord_req_t), PF_UNSPEC) == ERROR){
        perror("Sending request");
    }

    req_t response;
    if (recv(socket_fd, &response, sizeof(req_t), PF_UNSPEC) == ERROR){
        perror("Receiving response");
    }

    return response;
}

/***********************************************************************
 * func:            A function used to verify the user.
***********************************************************************/
void verify_user(){
    
    /* Send user credentials to server */
    if (send(socket_fd, &session_user, sizeof(ms_user_t), PF_UNSPEC) == ERROR){
        perror("Sending user credentials");
    }

    req_t response;
    if (recv(socket_fd, &response, sizeof(req_t), PF_UNSPEC) == ERROR){
        perror("Receiving login response");
    }

    switch(response){
        case valid:
            printf("Login successful! Welcome to the server %s!\n", session_user.username);
            break;
        case invalid:
            printf("Wrong username or password...\n");
            printf("Login unsucessful...Diconnecting.\n");
            exit_gracefully();
            break;
        default:
            /* This will never happen */
            break;
    }

}

/***********************************************************************
 * func:            A function used to print the welcome screen and
 *                  query the user for their credentials.
***********************************************************************/
void welcome_screen(){

    int cols = 64;

    print_line(cols);
    printf("Welcome to the CAB403 online Minesweeper server!\n");
    print_line(cols);

    printf("\nPlease enter your login details below:\n");

    printf("Username: ");
    if (fgets(session_user.username, MAX_USERNAME_LEN, stdin)){
        sscanf(session_user.username,"%s",session_user.username);
    };

    printf("Password: ");
    if (fgets(session_user.password, MAX_PASSWORD_LEN, stdin)){
        sscanf(session_user.password,"%s", session_user.password);
    }

    printf("\n");

}
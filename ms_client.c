
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h> 

#include "utils.h"

#include <strings.h> //TODO: Remove, with bzero in connect_to_server

/* Users connection point to server */
int socket_fd;

/* The current user */
ms_user session_user;

/* Function definitions */
void connect_to_server(char* argv[]);
int get_menu_choice();
void print_menu(menu_t menu_type);
void recieve_game();
req_t send_request(coord_req_t request);
void verify_user();
coord_req_t get_coords();
void welcome_screen();
void ms_process();

void game_sig(int* menu_choice);

int main(int argc, char* argv[]){

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

                break;
            /* Quit */
            case 3:

                break;
        }
    }

    return 0;
}

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
    //TODO: Below is deprecated
    bzero(&server_addr.sin_zero,sizeof(server_addr.sin_zero));

    if (connect(socket_fd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_in)) == ERROR){
        perror("Wrong server details or server is offline! Please try again.");
        exit(EXIT_FAILURE);
    }

}

void game_sig(int *choice){
    choice = 0;
}

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
            
        }
        printf("\n 0 --> Change mode\n");

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

int get_menu_choice(){
        char* val = malloc(sizeof(char));
        printf("   --> ");
        scanf("%s", val);
        int value = atoi(&val[0]);
        return value;
}

coord_req_t get_coords(){
        //TODO: take x and y individually
        coord_req_t coord_req;
        printf("X,Y--> ");
        scanf("%d,%d", &coord_req.x, &coord_req.y);
        coord_req.x--;
        coord_req.y--;
        return coord_req;
}

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

req_t send_request(coord_req_t request){

    if (send(socket_fd, &request, sizeof(coord_req_t), PF_UNSPEC) == ERROR){
        perror("Sending coordinate request");
    }

    req_t response;
    if (recv(socket_fd, &response, sizeof(req_t), PF_UNSPEC) == ERROR){
        perror("Receiving coordinate response");
    }

    return response;
}

void verify_user(){
    
    /* Send user credentials to server */
    if (send(socket_fd, &session_user, sizeof(ms_user), PF_UNSPEC) == ERROR){
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
            shutdown(socket_fd,SHUT_RDWR);
            close(socket_fd);
            exit(EXIT_FAILURE);
            break;
        default:
            /* This will never happen */
            break;
    }

}

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
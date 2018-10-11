#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> 
#include <unistd.h>

#include <signal.h>

#include "utils.h"

/* Enum for menu types */
typedef enum {
    main_menu,
    game_menu
} menu_type;

/* A struct representing the current user */
ms_user session_user;

int socket_fd;

//TODO: Waiting for spot in queue.....
//Maybe even have current number in queue, IE numreq-queuesize

void recieve_game(int socket_fd);

void exit_gracefully();
void print_menu(menu_type menu_type);
void send_request(int socket_fd, ms_coord_req request);
void show_leaderboard();
void verify_user();
void welcome_screen();

int main(int argc, char* argv[]){

    signal(SIGINT, exit_gracefully);

    struct hostent *host;
    struct sockaddr_in server_addr;

    /* If incorrect program usage */
    if (argc != 3){
        printf("\nUsage --> %s [hostname] [port]\n\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Get host info TODO: This is deprecated? */
    if ((host = gethostbyname(argv[1])) == NULL){
        herror("Host name");
        exit(EXIT_FAILURE);
    }

    /* Create socket */
    if ((socket_fd = socket(AF_INET,SOCK_STREAM, PF_UNSPEC)) == ERROR){ 
        perror("Creating socket");
        exit(EXIT_FAILURE);
    } 

    server_addr.sin_addr = *(struct in_addr *)host->h_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    bzero(&server_addr.sin_zero,sizeof(server_addr.sin_zero));

    clear_screen();
    welcome_screen();

    /* Client requests connection to server */
    if (connect(socket_fd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_in)) == ERROR) {
		perror("Wrong server details or server is offline! Please try again.");
		exit(1);
	}

    bool connected = true;

    verify_user(socket_fd);

    while (connected){

        print_menu(main_menu);
        printf("   --> ");
        int main_menu_choice = 0;
        scanf("%d", &main_menu_choice);

        bool ingame = false;
        int game_menu_choice = 0;

        ms_coord_req request;
       
        switch(main_menu_choice){

            case 1:
                ingame = true;
                while (ingame){

                    signal(SIGPIPE, SIG_IGN); //TODO: This dont work
                    

                    recieve_game(socket_fd);

                    if (game_menu_choice == 0){
                        print_menu(game_menu);
                        printf("   --> ");
                        scanf("%d",&game_menu_choice);
                    }

                    switch(game_menu_choice){
                        case 1:
                            printf("\nESC--> Change Mode\n");
                            printf("X,Y--> ");
                            
                            scanf("%d,%d", &request.x, &request.y);
                            request.type = reveal;
                            request.x--;
                            request.y--;
                            send_request(socket_fd, request);
                            break;
                        case 2:
                            break;
                        case 3:
                            ingame = false;
                            continue;
                        default:
                            printf("Invalid choice...\n");
                    }
                }
                break;
            case 2:
                show_leaderboard();
                break;
            case 3:
                exit_gracefully();
                break;
            default:
                printf("Invalid choice...\n");
        }
    }
    
    exit_gracefully();
    return 0;
}

void exit_gracefully(){
    shutdown(socket_fd,SHUT_RD);
    close(socket_fd);
    printf("\nThanks for playing!\n\n");
    fflush(0);
    exit(0);
}

void recieve_game(int socket_fd){
    
    int x,y;
    int cols, rows;

    uint16_t value;

    ms_coord_req request;
    request.type = game_board;

    if (send(socket_fd,&request, sizeof(ms_coord_req), PF_UNSPEC) == ERROR){
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

void send_request(int socket_fd, ms_coord_req request){
    if (send(socket_fd, &request, sizeof(ms_coord_req), PF_UNSPEC) == ERROR){
        perror("Sending user coord request");
    }
}

void show_leaderboard(){
    //TODO:
}

void print_menu(menu_type menu_type){
    printf("\n");
    switch(menu_type){
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

void verify_user(int socket_fd){

    /* Send user to server */
    if (send(socket_fd, &session_user, sizeof(ms_user), PF_UNSPEC) == ERROR){
        perror("Sending user credentials");
    }

    /* Recieve response from server */
    int response;
    if (recv(socket_fd,&response, sizeof(int), PF_UNSPEC) == ERROR){
        perror("Receiving login response");
    }

    switch(response){
        case ACCEPTED:
            printf("Login successful! Welcome to the server %s!\n", session_user.username);
            break;
        case REJECTED:
            printf("Wrong username or password...\nLogin unsuccessful...Disconnecting.\n");
            shutdown(socket_fd, SHUT_RDWR);
            close(socket_fd);
            exit(1);
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
    scanf("%s", session_user.username);

    printf("Password: ");
    scanf("%s", session_user.password);

    printf("\n");
}
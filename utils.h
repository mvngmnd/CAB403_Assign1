#ifndef UTILS_H_
#define UTILS_H_

/* No sys/socket.h definition */
#define NO_FLAGS 0

/* Definition for program wide errors */
#define ERROR -1

 /* Default port if not specified */
#define DEFAULT_PORT 1588  

/* Concurrent connections to be held */
#define QUEUE_SIZE 10    

/* Tile values & respective char to print */
#define UNSELECTED_CHAR "\u25FC"
#define UNSELECTED_VAL 10
#define FLAG_CHAR "\u2690"
#define FLAG_VAL 11
#define BOMB_CHAR "\u06DE"
#define BOMB_VAL 12

/* A struct representing a user */
#define MAX_USERNAME_LEN 64
#define MAX_PASSWORD_LEN 64
typedef struct {
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
} ms_user_t;

/* Enums for standardized communication */
typedef enum{
    gameboard,
    leaderboard,
    flag,
    reveal,
    quit,
    won,
    lost,
    valid,
    invalid
} req_t;

/* Enums for menu types */
typedef enum{
    main_menu,
    game_menu
} menu_t;

/* Struct for coordinate request */
typedef struct{
    int x;
    int y;
    req_t request_type;
} coord_req_t;

/****************************************************
 * func: Clear the screen
****************************************************/
void clear_screen();

/****************************************************
 * func: 
 * param cols:
 * param rows:
 * param board:
****************************************************/
void print_game(int cols, int rows, int board[cols][rows]);

/****************************************************
 * func:
 * param len
****************************************************/
void print_line(int len);

#endif /* UTILS_H_ */
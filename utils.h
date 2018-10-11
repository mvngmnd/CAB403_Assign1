#ifndef UTILS_H_
#define UTILS_H_

 /* Default port if not specified */
#define DEFAULT_PORT 1588  

/* Concurrent connections to be held */
#define QUEUE_SIZE 10    

/* State values */
#define ERROR -1
#define ACCEPTED 1
#define REJECTED -1

/* No sys/socket.h definition */
#define NO_FLAGS 0

/* Max lengths for user struct fields */
#define MAX_PASSWORD_LEN 64
#define MAX_USERNAME_LEN 64

/* Tile values, with respective char to print */
#define UNSELECTED_CHAR "\u25FC"
#define UNSELECTED_VAL 10
#define FLAG_CHAR "\u2690"
#define FLAG_VAL 11
#define BOMB_CHAR "\u06DE"
#define BOMB_VAL 12

typedef enum{
    game_board,
    leaderboard,
    flag,
    reveal,
    quit
} request_type;

/* A struct used to represent a user */
typedef struct {
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
}  ms_user;

/* A struct used to represent a request */
typedef struct{
	int x;
	int y;
	request_type type;
} ms_coord_req;

/******************************************************************
* func: Clears the screen.
******************************************************************/
void clear_screen();

/******************************************************************
* func:       Prints the current state of the game to the screeen
* param cols: The columns/width of the array to be passed
* param rows: The rows/length of the array to be passed
* param game: An array containing a numerical representation of a
*             game board state.
******************************************************************/
void print_game(int cols, int rows, int game[cols][rows]);

/******************************************************************
* func:         Draws a line on screen of a specified length
* param Length: The desired length of the line, in numerical format
******************************************************************/
void print_line(int length);

#endif /* UTILS_H_ */
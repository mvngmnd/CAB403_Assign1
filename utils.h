#ifndef UTILS_H_
#define UTILS_H_

/* No sys/socket.h definition */
#define NO_FLAGS 0

/* Definition for program wide errors */
#define ERROR -1

 /* Default port if not specified */
#define DEFAULT_PORT 1901

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
    scoreboard,
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

/* Struct of a single scoreboard entry */
typedef struct scoreboard_entry scoreboard_entry_t;
struct scoreboard_entry{
    int seconds_taken;
    ms_user_t user;
    scoreboard_entry_t* next;
};

/* Struct of a user history */
typedef struct ms_user_history ms_user_history_t;
struct ms_user_history{
    int won;
    int lost;
    ms_user_t user;
};

typedef struct ms_user_history_entry ms_user_history_entry_t;
struct ms_user_history_entry{
    ms_user_history_t user;
    ms_user_history_entry_t* next;
};

/***********************************************************************
 * func:            Clears the screen
***********************************************************************/
void clear_screen();

/***********************************************************************
 * func:            Prints a given game state to the screen. The game
 *                  state given is only a numerical representation of
 *                  the game.
 * param cols:      The columns/width of the game board.
 * param rows:      The rows/height of the game board.
 * param board:     The integer array containing a numerical
 *                  representation of the game state.
***********************************************************************/
void print_game(int cols, int rows, int board[cols][rows]);

/***********************************************************************
 * func:            Prints a beautiful line on the screen.
 * param len:       The length of the beautiful line.
***********************************************************************/
void print_line(int len);

#endif /* UTILS_H_ */
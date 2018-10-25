#ifndef MS_H_
#define MS_H_

#include <stdbool.h>

/* Utility definitions */
#include "utils.h"

#define MS_COLS 9       /* Size of x axis of a game board. Undef behaviour if > 99 */
#define MS_ROWS 9       /* Size of y axis of a game board. Undef behaviour if > 99 */
#define MS_BOMBS 10     /* Number of bombs to be in a single game. Undef behaviour if > MS_COLS*MS_ROWS */

/* A struct representing a single square on the game board */
typedef struct {
    bool bomb;
    bool flagged;
    bool revealed;
    int adjacent;
} ms_tile_t;

/* A struct representing a group of tiles, indicative of a game board */
typedef struct {
    ms_tile_t board[MS_COLS][MS_ROWS];
    bool first_turn;
} ms_game_t;

/***********************************************************************
 * func:            Creates a new game state based on a given seed.
 * param rand_seed: The specified seed value.
***********************************************************************/
ms_game_t new_game(int rand_seed);

/***********************************************************************
 * func:            Flags a tile at a given location on a given game
 *                  board.
 * param game:      The game board to alter.
 * param x:         The x location of the flag.
 * param y:         The y location of the flag.
***********************************************************************/
req_t flag_tile(ms_game_t *game, int x, int y);

/***********************************************************************
 * func:            Reveals a tile at a given location on a given game
 *                  board.
 * param game:      The game board to alter.
 * param x:         The x location of the flag.
 * param y:         The y location of the flag.
***********************************************************************/
req_t reveal_tile(ms_game_t *game, int x, int y);

/***********************************************************************
 * func:            Determines if a given location on a given game
 *                  board has been flagged.
 * param game:      The game board to check.
 * param x:         The x location to check.
 * param y:         The y location to check.
***********************************************************************/
bool location_flagged(ms_game_t *game, int x, int y);

/***********************************************************************
 * func:            Determines if a given location on a given game
 *                  board has been revealed.
 * param game:      The game board to check.
 * param x:         The x location to check.
 * param y:         The y location to check.
***********************************************************************/
bool location_revealed(ms_game_t *game, int x, int y);

/***********************************************************************
 * func:            Determines if a given location on a given game
 *                  board is valid, IE: out of bounds.
 * param game:      The game board to check.
 * param x:         The x location to check.
 * param y:         The y location to check.
***********************************************************************/
bool location_valid(ms_game_t *game, int x, int y);

/***********************************************************************
 * func:            Returns the total number of bombs remaining on a
 *                  given game board. A bomb is considered to be
 *                  remaining if it is not flagged.
***********************************************************************/
int bombs_remaining(ms_game_t *game);

#endif /* MS_H_ */
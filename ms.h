#ifndef MS_H_
#define MS_H_

#include <stdbool.h>

/* Utility definitions */
#include "utils.h"

#define MS_COLS 9       /* Size of x axis of a game board. Undef behaviour if > 99 */
#define MS_ROWS 9       /* Size of y axis of a game board. Undef behaviour if > 99 */
#define MS_BOMBS 10     /* Number of bombs to be in a single game. Undef behaviour if > MS_COLS*MS_ROWS */

typedef struct {
    bool bomb;
    bool flagged;
    bool revealed;
    int adjacent;
} ms_tile_t;

typedef struct {
    ms_tile_t board[MS_COLS][MS_ROWS];
    bool first_turn;
} ms_game_t;

ms_game_t new_game(int rand_seed);
req_t reveal_tile(ms_game_t *game, int x, int y);
void flag_tile(ms_game_t *game, int x, int y);
bool location_valid(ms_game_t *game, int x, int y);
bool location_revealed(ms_game_t *game, int x, int y);
bool location_flagged(ms_game_t *game, int x, int y);

#endif /* MS_H_ */
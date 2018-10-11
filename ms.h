#include <stdbool.h>

#ifndef MS_H_
#define MS_H_

#define MS_COLS 9
#define MS_ROWS 9
#define MS_BOMBNO 10

typedef struct {
    bool revealed;
    int bomb;
    bool flagged;
    int adjacent;
} ms_tile;

typedef struct{
    ms_tile board[MS_COLS][MS_ROWS];
	bool first_turn;
} ms_game;

ms_game new_game(int rand_seed);

#endif /* MS_H_ */
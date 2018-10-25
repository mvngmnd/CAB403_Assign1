#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

/* Minesweeper definitions */
#include "ms.h"
/* Utility definitions */
#include "utils.h"

bool location_bomb(ms_game_t *game, int x, int y){
    return (game->board[x][y].bomb);
}

bool location_valid(ms_game_t *game, int x, int y){
    return (x>= 0 && y>=0 && x<MS_COLS && y<MS_ROWS);
}

bool location_revealed(ms_game_t *game, int x, int y){
    return (game->board[x][y].revealed);
}

bool location_flagged(ms_game_t *game, int x, int y){
    return game->board[x][y].flagged;
}

bool place_bomb(ms_game_t *game, int x, int y){

    /* If there is already a bomb at location */
	if (game->board[x][y].bomb){
        return false;
    }
    
    int xi, yi;
    game->board[x][y].bomb = true;

	/* Increment the adjacent value of all adjacent tiles */
    for (yi=y-1;yi<=y+1;yi++){
		for (xi=x-1;xi<=x+1;xi++){
            if (!(xi==x && yi==y)){
                if (xi>=0 && xi<MS_COLS && yi>=0 && yi<MS_ROWS){
                    game->board[xi][yi].adjacent++;
                }
            }
        }
    }

    return true;

}

void remove_bomb(ms_game_t *game, int x, int y){
    int xi,yi;

    game->board[x][y].bomb = false;
    for (xi=x-1;xi<=x+1;xi++){
        for (yi=y-1;yi<=y+1;yi++){
            if (location_valid(game, xi, yi)){
                if (!(xi==x && yi==y)&&!location_bomb(game,xi,yi)){  /* If not the bomb itself, or any other bomb */
                    game->board[xi][yi].adjacent--;
                }
            }
        }
    }

}

int bombs_remaining(ms_game_t *game){
    int x,y,remaining = 0;

    for (y=0;y<MS_ROWS;y++){
        for (x=0;x<MS_COLS;x++){
            if (game->board[x][y].bomb && !game->board[x][y].flagged){
                remaining++;
            }
        }
    }

    return remaining;
}

void reveal_board(ms_game_t *game){
    int x, y;

    for (y=0;y<MS_ROWS;y++){
        for(x=0;x<MS_COLS;x++){
            if (game->board[x][y].flagged){
                game->board[x][y].flagged = false;
            }
            game->board[x][y].revealed = true;
        }
    }

}

bool check_win(ms_game_t *game){
    int x,y;

    if (bombs_remaining(game) == 0){
        reveal_board(game);
        return true;
    }

    for (y=0;y<MS_ROWS;y++){
        for (x=0;x<MS_COLS;x++){
            /* If there is a tile that is not revealed and isnt a bomb, they havent won yet */
            if (!game->board[x][y].revealed && !game->board[x][y].bomb){
                return false;
            }
        }
    }

    return true;
}

req_t flag_tile(ms_game_t *game, int x, int y){

    game->board[x][y].flagged = !game->board[x][y].flagged;

    if (check_win(game)){
        reveal_board(game);
        return won;
    }

    return valid;
    
}

req_t reveal_tile(ms_game_t *game, int x, int y){

	if (location_bomb(game,x,y)) {
        if (game->first_turn){                   /* Impossible for a bomb to be hit on first go */
            remove_bomb(game,x,y);
            for (int j = 0;j<MS_ROWS;j++){
                for (int i=0;i<MS_COLS;i++){
                    if (place_bomb(game,i,j)){   /* Places the bomb at first possible x location */
                        reveal_tile(game,x,y);
                        game->first_turn = false;
                        return valid;
                    }
                }
            }
        }
        
        reveal_board(game);
        return lost;

    } else if (game->board[x][y].adjacent == 0){  /* If blank spot chosen, reveal all nearby blank spots */
        for (int i = x-1; i<=x+1; i++){
            for (int j = y-1; j<=y+1; j++){
                    if (i>=0 && j>=0 && i<MS_COLS && j<MS_ROWS){
                        if (!game->board[i][j].revealed){
                            game->board[i][j].revealed = true;
                            reveal_tile(game,i,j);
                        }
                    }
                
            }
        }
    } else {
        game->board[x][y].revealed = true;
    }

    if (game->first_turn){
        game->first_turn = false;
    }

    if (check_win(game)){
        return won;
    }

    return valid;
    
}

ms_game_t new_game(int rand_seed){
    ms_game_t game;
    srand(rand_seed);

    int i,x,y;

    game.first_turn = true;

    /* Initialize array */
	for (y=0;y<MS_ROWS;y++){
		for (x=0;x<MS_COLS;x++){
			game.board[x][y].adjacent = 0;
			game.board[x][y].bomb = false;
			game.board[x][y].flagged = false;
			game.board[x][y].revealed = false;
		}
	}

    /* Place bombs */
	for (i=0;i<MS_BOMBS;i++){
		do{
			x = rand() % MS_COLS;
			y = rand() % MS_ROWS;
		} while (!place_bomb(&game,x,y));
	}
    
    return game;
}
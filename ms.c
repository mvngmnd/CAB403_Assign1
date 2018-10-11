#include "ms.h"

#include <stdbool.h>
#include <stdlib.h> 
#include <stdio.h> 
#include <unistd.h>

bool location_is_bomb(ms_game *game, int x, int y){
    return (game->board[x][y].bomb);
}

bool location_is_revealed(ms_game *game, int x, int y){
    return (game->board[x][y].revealed);
}

bool location_is_valid(ms_game *game, int x, int y){
    /* Determine that the location is on the board */
    return (x>= 0 && y>=0 && x<MS_COLS && y<MS_ROWS);
}

void reveal_board(ms_game *game){
    int x, y;
    for (y=0;y<MS_ROWS;y++){
        for(x=0;x<MS_COLS;x++){
            game->board[x][y].revealed = true;
        }
    }
}

void remove_bomb(ms_game *game, int x, int y){
    int xi,yi;

    game->board[x][y].bomb = false;
    for (xi=x-1;xi<=x+1;xi++){
        for (yi=y-1;yi<=y+1;yi++){
            if (location_is_valid(game, xi, yi)){
                if (!(xi==x && yi==y)&&!location_is_bomb(game,xi,yi)){  /* If not the bomb itself, or any other bomb */
                    game->board[xi][yi].adjacent--;
                }
            }
        }
    }

}

bool check_win(ms_game *game){
    int x,y;
    bool won = true;
    
    /* Check if all non-bombs have been revealed */
    for (y=0;y<MS_ROWS;y++){
        for (x=0;x<MS_COLS;x++){
            if (!location_is_bomb(game,x,y) && !game->board[x][y].revealed){
                won = false;
            }
        }
    }
    return won;
}

bool place_bomb(ms_game *game, int x, int y){

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

void flag_tile(ms_game *game, int x, int y){
    game->board[x][y].flagged = !game->board[x][y].flagged;
}

void reveal_tile(ms_game *game, int x, int y){

	if (location_is_bomb(game,x,y)) {
        if (game->first_turn){                   /* Impossible for a bomb to be hit on first go */
            remove_bomb(game,x,y);
            for (int j = 0;j<MS_ROWS;j++){
                for (int i=0;i<MS_COLS;i++){
                    if (place_bomb(game,i,j)){   /* Places the bomb at first possible x location */
                        reveal_tile(game,x,y);
                        game->first_turn = false;
                        return;
                    }
                }
            }
        }
        
        reveal_board(game); // ADD LOSE FUNCTIONALITY

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
    
}

ms_game new_game(int seed){

	ms_game game;
	
	int i, x, y;

	srand(seed);
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
	for (i=0;i<MS_BOMBNO;i++){
		do{
			x = rand() % MS_COLS;
			y = rand() % MS_ROWS;
		} while (!place_bomb(&game,x,y));
	}
    
    return game;

}
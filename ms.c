#include "ms.h"

#include <stdbool.h>
#include <stdlib.h> 
#include <stdio.h> 
#include <unistd.h>


void remove_bomb(ms_game *game, int x, int y){

    game->board[x][y].bomb = false;
    for (int i=x-1;i<=x+1;i++){
        for (int j=y-1;j<=y+1;j++){
            if (!(i==x && j==y)&&(game->board[i][j].bomb == false)){ /* If not the bomb itself, or any other bomb */
                if (i>=0 && i<MS_COLS && j>=0 && j<MS_ROWS){        /* If not outside the board */
                    game->board[i][j].adjacent--;
                }
            }
        }
    }

}

bool place_bomb(ms_game *game, int x, int y){
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

	if (game->board[x][y].bomb) {
        /* Results it in being impossible for a bomb to be hit on first go */
        if (game->first_turn){
            remove_bomb(game,x,y);
            for (int j = 0;j<MS_ROWS;j++){
                for (int i=0;i<MS_COLS;i++){
                    /* Places the bomb at first possible x location */
                    if (place_bomb(&game,i,j)){
                        reveal_tile(game,x,y);
                        game->first_turn = false;
                        return;
                    }
                }
            }
        }

    } else if (game->board[x][y].adjacent == 0){
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
	game.first_turn = false;

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
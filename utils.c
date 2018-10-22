#include <stdio.h>
#include <stdlib.h>

/* Utility definitions */
#include "utils.h"

void clear_screen(){
    system("@cls||clear");
}

void print_game(int cols, int rows, int board[cols][rows]){

    int x,y;

    printf("\n");

    /* If more than 10 columns, print base-10 indicies on top */
    if (cols>=10){
        printf("  ");
        for (x=1;x<=cols;x++){
            if (x%10==0){
                printf(" %d",x/10);
            } else {
                printf("  ");
            }    
        }
        printf("\n");
    }

    /* Print x axis coordinates */
    printf("  ");
    for (x=1;x<=cols;x++){
        printf("|%d",x%10);
    }

    /* Print dividing line ----- */
    printf("\n");
    for (x=0;x<=cols;x++){
        printf("--");
    }
    printf("\n");

    /* Print y axis */
    for (y=0;y<rows;y++){

        if (y<9 && cols>9){
            /* Print y axis coordinates */
            printf(" %d|",y+1);
        } else {
            printf("%d|",y+1);
        }
    
        for (x=0;x<cols;x++){
            /* Print y axis tile values */
            switch(board[x][y]){
                case BOMB_VAL:
                    printf("%s ", BOMB_CHAR);
                    break;
                case FLAG_VAL:
                    printf("%s ", FLAG_CHAR);
                    break;
                case UNSELECTED_VAL:
                    printf("%s ", UNSELECTED_CHAR);
                    break;
                default:
                    if (board[x][y]>0){
                        printf("%d ", board[x][y]);
                        break;
                    } else {
                        printf("  ");
                        break;
                    }
            }
        }
        printf("\n");
    }
}

void print_line(int length){

    int i;

    for (i=0;i<length;i++){
        printf("=");
    }

    printf("\n");

}
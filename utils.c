#include "utils.h"
#include <stdio.h>

void clear_screen(){
    system("@cls||clear");
}

void print_game(int cols, int rows, int game[cols][rows]){

    printf("\n");

    int x,y;

    /* If more than ten columns, have base-10 indicies */
    if (cols>=10){
        for (x=1;x<=cols;x++){
            if (!x%10==0){
                printf("  ");
            } else {
                printf(" %d",x/10);
            }
        }
        printf("\n");
    }

    /* Print x axis */
    printf("  ");
    for (x=1;x<=cols;x++){
        printf("|%d",x%10);
    }

    /* Print ------- */
    printf("\n");
    for (x=0;x<=cols;x++){
        printf("--");
    }
    printf("\n");

    /* Print y axis */
    for (y=0;y<rows;y++){
        printf("%d| ",y+1);
        for (x=0;x<cols;x++){
            switch(game[x][y]){
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
                    if (game[x][y]>0){
                        printf("%d ", game[x][y]);
                        break;
                    } else {
                        printf("  ");
                        break;
                    }
                    
            }
        }
        printf("\n");
    }
    printf("\n");
}

void print_line(int length){
    for (int i = 0;i<length;i++){
        printf("=");
    }
    printf("\n");
}
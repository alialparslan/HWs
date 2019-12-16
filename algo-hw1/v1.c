#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct{
    int x;
    int y;
} coordinate;

typedef struct{
    int x1;
    int x2;
    int y1;
    int y2;
}pair;

void swap(coordinate* coord1, coordinate* coord2){
    coordinate temp;
    temp = *coord1;
    *coord1 = *coord2;
    *coord2 = temp;
}

void quickSort(coordinate* coords, int l, int r){
    if(l >= r){
        return;
    }
    int pivot = coords[r].x;

    int move = l;

    for(int i = l; i <= r; i++){
        if(coords[i].x <= pivot){
            swap(&coords[move], &coords[i]);
            move++;
        }
    }
    quickSort(coords, l, move-2);
    quickSort(coords, move, r);
}



void printDots(int n, coordinate* coords){
    int i;
    for(i = 0; i < n; i++){
        printf("Dot#%d %d:%d\n", i+1, coords[i].x, coords[i].y);
    }
}

double calcDistance(coordinate coord1, coordinate coord2){
    return pow( pow(coord1.x-coord2.x, 2)+pow(coord1.y-coord2.y, 2), 0.5 );  
}

// returns and array consists of indexes of closest pairs
int* findClosestPair(coordinate* coords, int l, int r){
    double median; // Median of X axis values of coords
    int* closestPair; // To store closest pair found
    int* closestPairR; // To stores results from recursive call responsible from rigt side
    int* closestPairL; // To stores results from recursive call responsible from left side
    double distance, distanceR, distanceL;
    int count = r-l+1; // Dot count in current range
    int i, j; // loop variables
    if(count <= 3){ // There will be l-r+1 dots between l and r (including both r and l)
        closestPair = malloc(sizeof(int)*2);
        closestPair[0] = l;
        closestPair[1] = l+1;
        distance = calcDistance(coords[l], coords[l+1]);
        for(i = l; i <= r; i++){
            for(j = i+1; j <= r; j++){
                if(i != j && calcDistance(coords[i],coords[j]) < distance){
                    closestPair[0] = i;
                    closestPair[1] = j; 
                    distance = calcDistance(coords[i],coords[j]);
                }
            }
        }
        return closestPair;
    }

    // Finding median, and closest dots to it from sides in X axis
    if(count%2 == 0){ 
        median = (coords[l+count/2-1].x+coords[l+count/2].x)/2.0;
    }else{
        median = coords[l+count/2].x;
    }
    closestPairR = findClosestPair(coords, l, l+count/2-1);
    closestPairL = findClosestPair(coords, l+count/2, r);

    distanceR = calcDistance(coords[closestPairR[0]],coords[closestPairR[1]]);
    distanceL = calcDistance(coords[closestPairL[0]],coords[closestPairL[1]]);

    if(distanceR > distanceL){
        closestPair = closestPairL;
        free(closestPairR);
        distance = distanceL;
    }else{
        closestPair = closestPairR;
        free(closestPairL);
        distance = distanceR;
    }

    //Checking dots in the areas between x=m and x=distance, -distance
    for(i = l+count/2-1; i >= l && distance >= (median-coords[i].x); i--){
        for(j = l+count/2; j <= r && distance >= (coords[j].x-median); j++){
            if(i != j && calcDistance(coords[i], coords[j]) < distance){

                distance = calcDistance(coords[i], coords[j]);
                closestPair[0] = i;
                closestPair[1] = j;
            }
        }
    }
    return closestPair;
}


void main(){
    int i; // Loop var.
    int count; // Count of dots
    int* closestPair; // To store indexes of closest pair
    coordinate* coords; // Array that hold coordinates of dots

    printf("Dot count:\n");
    scanf("%d",&count);
    coords = malloc(sizeof(coordinate) * count);
    printf("Type coordinates in [X]:[Y] format, i.e. 17:23\n");
    for(i = 0; i < count; i++){
        printf("Type Dot#%d:", i+1);
        scanf("%d:%d", &coords[i].x, &coords[i].y);
    }

    printf("\nPrinting coordinates of all dots:\n");
    printDots(count, coords);

    quickSort(coords, 0, count-1);
    printf("\nPrinting sorted coordinates of all dots:\n");
    printDots(count, coords);

    closestPair = findClosestPair(coords, 0, count-1);
    printf("Closest Pair: %d:%d , ", coords[closestPair[0]].x, coords[closestPair[0]].y);
    printf("%d:%d", coords[closestPair[1]].x, coords[closestPair[1]].y);

    free(closestPair);
}
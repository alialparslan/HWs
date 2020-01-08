#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>

#define DATA_FILE "input-mpaa.txt"

#define DEBUG_MODE 0

#if DEBUB_MODE 
    #define DEBUG(p1, ...)  printf(p1, __VA_ARGS__)
#else
    #define DEBUG
#endif

#define LINUX 0

#if LINUX
    // http://man7.org/linux/man-pages/man3/iconv.3.html
    #include <iconv.h> // For conversion from cp1252 to utf-8 in linux
    #define CLEAR_COMMAND "clear"
#else
    #define CLEAR_COMMAND "clear"
#endif

#define ALLOC_INCREMENT 10
#define HASHMAP_SIZE 200000
#define QUEUE_CONTAINER_SIZE 1000

// min_available has to be greater than 0 and less than ALLOC_INCREMENT
#define ARR_CAPACITY_ADJUST(arr, count, unit_size, min_available) \
    if(count > 0 && count % ALLOC_INCREMENT == 0) \
        arr = realloc(arr, unit_size * ( ALLOC_INCREMENT + count + min_available -1));

#define CREATE_STR(var, source, length) \
    var = malloc(sizeof(char) + (length + 1)); \
    var[length] = '\0'; \
    strncpy(var, source, length);

#define CONSUME_BUFFER(var, buffer, length) \
    var = realloc(buffer, sizeof(char) * (length + 1) ); \
    var[length] = '\0'; \
    buffer = malloc(sizeof(char)*ALLOC_INCREMENT);


#define TYPE_MOVIE 0
#define TYPE_STAR 1    
//Both for stars and movies
typedef struct node{
    int id; // Unique id (Uniqe among same type)
    char type; // 0 => a movie; 1 => a star
    char *name;
    int pairCount;
    struct node **pairs; // aka edges
}node; // This is node for graph structure and only struct for graph, others are for star search tree

// Instead of single array, our queue will use multiple container and allocate news as capacity grows meanwhile freeing when a container becomes empty
typedef struct queue_container{
    node* array[QUEUE_CONTAINER_SIZE];
    struct queue_container *next; // Single linked list
}queue_container;

// Main queue strcture
typedef struct{
    queue_container *front; // Container which new values will be insterted
    queue_container *rear;  // Container which values will dequeued from
    int enqueuePos; // Position to insert new value in front container
    int dequeuePos; // Position of oldest value in rear container to dequeue
}queue;

// Read until new line or maxLength and returns the length
int getLine(char *buffer, int maxLength){
    int i = 0;
    while(i < maxLength){
        buffer[i] = getchar();
        if(buffer[i] == '\n'){
            buffer[i] = '\0';
            return i;
        }
        i++;
    }
    buffer[i] = '\0';
    return i;
}

/////////////////////////////////////////////// QUEUE FUNCTIONS ///////////////////////////////////////////////
// Initializes/allocates new queue on given pointer
void initializeQueue(queue *q){
    q->front = malloc(sizeof(queue_container));
    q->front->next = NULL;
    q->rear = q->front;
    q->enqueuePos = 0;
    q->dequeuePos = 0;
}

void enQueue(queue *q, node* e){
    queue_container *newContainer;
    if(q->enqueuePos == QUEUE_CONTAINER_SIZE){ // If queue is full
        newContainer = malloc(sizeof(queue_container)); // Allocate new container
        newContainer->next = NULL;
        q->front->next = newContainer; // Set previous container's next to point new container
        q->front = newContainer;
        q->enqueuePos = 0;
    }
    q->front->array[q->enqueuePos++] = e; //Add element to array and increase next insert position
}

// Since our queue holds pointers and we won't enqueue any NULL ptr, it will return NULL when queue is empty
node* deQueue(queue *q){
    queue_container *containerPtr;
    if(q->rear == q->front){ // If rear and front containers are same queue may be empty
        if(q->enqueuePos == q->dequeuePos){
            return NULL;
        }else{
            return q->rear->array[q->dequeuePos++];
        }
    }else if(q->dequeuePos == QUEUE_CONTAINER_SIZE){
        // This means current rear container is empty, so time to move next container
        // Since enQueue does not allocate new container without inserting a new value
        // We know our new rear container has at least one valu
        q->dequeuePos = 1;
        containerPtr = q->rear;
        q->rear = q->rear->next;
        free(containerPtr);
        return q->rear->array[0];
    }else{
        return q->rear->array[q->dequeuePos++];
    }
}

// Frees all containers belonging to queue
void freeQueue(queue *q){
    queue_container *tmpPtr;
    tmpPtr = q->rear;
    while(q->rear && q->rear->next){
        tmpPtr = q->rear->next;
        free(q->rear);
        q->rear = tmpPtr;
    }
    if(q->rear) free(q->rear);
}


/////////////////////////////////////////////// HASHMAP FUNCTIONS ///////////////////////////////////////////////
typedef struct{
    int *counts;
    node ***arrays;
}hash_map;

unsigned int calcHash(char *string){
    unsigned int hash = 0;
    while(*string){
        hash = (hash*323 + *string) % HASHMAP_SIZE;
        string++;
    }
    return hash;
}

void AddNodeHM(hash_map hm, node* n){
    unsigned int hash = calcHash(n->name);
    int count;
    count = ++(hm.counts[hash]);
    if(count > 1){
        hm.arrays[hash] = realloc(hm.arrays[hash], sizeof(node**) * count);
        hm.arrays[hash][count-1] = n;
    }else{
        hm.arrays[hash] = malloc(sizeof(node**));
        hm.arrays[hash][0] = n;
    }
}

// Return NULL if there isn't a node with given name otherwise returns node's pointer
node* searchHM(hash_map hm, char *name){
    unsigned int hash = calcHash(name);
    node **nodes;
    int count;
    int i;
    count = hm.counts[hash];
    if(count){
        nodes = hm.arrays[hash];
        for(i = 0; i < count; i++){
            if( strcmp(name, nodes[i]->name) == 0){
                return nodes[i];
            }
        }
    }
    return NULL;
}

node* addStar(hash_map hm, char* name, node *movie){
    node *starNode;

    starNode = searchHM(hm, name);
    if(starNode)
        return starNode;

    starNode = malloc(sizeof(node));
    
    starNode->name = name;
    starNode->type = 1;
    if(movie){
        starNode->pairs = malloc( sizeof(node*) );
        starNode->pairs[0] = movie;
        starNode->pairCount = 1;
    }else{
        starNode->pairCount = 0;
        starNode->pairs = NULL;
    }
    AddNodeHM(hm, starNode);
    return starNode;
}

void pairNodes(node *node1, node *node2){
    if(node1->type == node2->type){
        printf("This should never happen!");
        return;
    }
    if(node1->pairCount == 0){
        node1->pairs = malloc(sizeof(node*));
    }else{
        node1->pairs = realloc(node1->pairs, sizeof(node*) * (node1->pairCount + 1) );
    }
    if(node2->pairCount == 0){
        node2->pairs = malloc(sizeof(node*));
    }else{
        node2->pairs = realloc(node2->pairs, sizeof(node*) * (node2->pairCount + 1) );
    }
    node1->pairs[node1->pairCount++] = node2;
    node2->pairs[node2->pairCount++] = node1;
}

#define NEW_NODE(var, TYPE, uniqueID) \
    var = malloc(sizeof(node)); \
    var->type = TYPE; \
    var->pairCount = 0; \
    var->id = uniqueID;

// Returns list of movie nodes and registers all stars to tree
node** generateGraphFromFile(hash_map hm, char* file, int *totalMovie, int *totalStar){    
    //Those six needed for iconv (converting cp1252 to utf8) function
    #if LINUX
        iconv_t iconv_handle = iconv_open("UTF-8", "CP1252");
        size_t iconv_inbytesleft;
        size_t iconv_outbytesleft;
        size_t iconv_return;
        char *iconv_inbuf;
        char *iconv_outbuf;
    #endif
    node **movies = malloc(sizeof(node*) * ALLOC_INCREMENT); // List of all movie nodes
    char *buffer = malloc( ALLOC_INCREMENT * sizeof(char)); // 1 for null, 3 for in case of multi byte sequence
    int buffer_capacity = ALLOC_INCREMENT;
    int i = 0; // Counts of bytes currently in buffer
    FILE *f;
    int count = 0; // Movie count, or total record in file
    int starCount = 0;
    char ch; // temp to store currently processed char
    node *movie = NULL;
    node *star;
    char *strPtr;

    f = fopen(file, "rb");
    if(f == NULL){
        printf("Failed to open file!\n");
        exit(1);
    }
    while(!feof(f)){
        ch = fgetc(f);
        if(ch == '/' || ch == '\n'){
            buffer[i] = '\0';
            if(movie){
                star = searchHM(hm, buffer);
                if(star == NULL){
                    starCount++;
                    NEW_NODE(star, TYPE_STAR, starCount);
                    CONSUME_BUFFER(star->name, buffer, i);
                    AddNodeHM(hm, star);
                }
                pairNodes(movie, star);
            }else if(ch != '\n'){
                ARR_CAPACITY_ADJUST(movies, count, sizeof(node*), 1);
                NEW_NODE(movie, TYPE_MOVIE, count);
                CONSUME_BUFFER(movie->name, buffer, i);
                movies[count++] = movie;
            }
            buffer_capacity = ALLOC_INCREMENT;
            i = 0;
            if(ch == '\n'){
                movie = NULL;
            }
        }else{
            if(i > buffer_capacity-4){
                buffer_capacity += ALLOC_INCREMENT;
                buffer = realloc(buffer, buffer_capacity * sizeof(char) );
            }
            #if LINUX
                iconv_inbytesleft = 1;
                iconv_outbytesleft = 4;
                iconv_outbuf = buffer+i;
                iconv_inbuf = &ch;
                iconv_return = iconv(iconv_handle, &iconv_inbuf, &iconv_inbytesleft, &iconv_outbuf, &iconv_outbytesleft);
                if(iconv_return == -1) printf("iconv error: %s\n", strerror(errno));
                i += 4 - iconv_outbytesleft;
            #else
                buffer[i++] = ch;
            #endif
        }
    }
    movies = realloc(movies, sizeof(node*) * (count+1));
    movies[count] = NULL;
    *totalMovie = count;
    *totalStar = starCount;
    #if LINUX
        iconv_close(iconv_handle);
    #endif
    return movies;
}

char* strAdd(char* dest, char *strToAdd){
    int i = 0;
    while(dest[i]) i++;
    i++;
    while(*strToAdd){
        if(i >= ALLOC_INCREMENT && i % ALLOC_INCREMENT == 0)
            dest = realloc(dest, sizeof(char) * (i + ALLOC_INCREMENT + 1)); // +1 for null terminator
        dest[i++] = *strToAdd;
        strToAdd++;
    }
    dest[i] = '\0';
}

// Utility for testing, not part of real code
char* strFromConst(const char* constStr){
    char *str = malloc(sizeof(char) *( strlen(constStr) + 1));
    strcpy(str, constStr);
    return str;
}

// Returns 0 if there isnt a path found between the given stars
// Otherwise returns distance and fills pathPtr with nodes of stars and movies in path
// Path will store star2, a movie, a star, a movie, star1.... starts with a start2 and ends with a star1
// Note that path will store nodes starting from star2 to star1
int findPath(node *star1, node *star2, int movieCount, int starCount, node ***pathPtr){
    int i;
    node **prevMovies = calloc(movieCount, sizeof(node*)); // Null if not visited yet
    node **prevStars = calloc(starCount, sizeof(node*)); // Null if not visited yet
    queue q;
    queue *qPtr = &q;
    node *aNode; // temp ptr
    node **path;
    node *kevinsNode;
    int id; // temp
    int targetNotFoundYet = 1; //boolean to stop BFS when came accross with Kevin Bacon
    int kevinsID = star2->id;
    initializeQueue(qPtr);

    prevStars[star1->id] = star1; // Well, we need to point to somewhere
    enQueue(qPtr, star1);
    
    while(targetNotFoundYet && (aNode = deQueue(qPtr)) != NULL){
        // Depending on type of the node we are using different arrays to keep track of whether
        // node is visited or not and preeceded by which node if it is visited.
        if(aNode->type == TYPE_MOVIE){
            for(i = 0; i < aNode->pairCount; i++){
                id = aNode->pairs[i]->id;
                if(prevStars[id] == NULL){
                    prevStars[id] = aNode;
                    if(id == kevinsID){
                        kevinsNode = aNode->pairs[i];
                        targetNotFoundYet = 0;
                        i = aNode->pairCount; //to break from loop
                    }
                    enQueue(qPtr, aNode->pairs[i]);
                }
            }
        }else{ // If type of node is star
            for(i = 0; i < aNode->pairCount; i++){
                id = aNode->pairs[i]->id;
                if(prevMovies[id] == NULL){
                    prevMovies[id] = aNode;
                    enQueue(qPtr, aNode->pairs[i]);
                }
            }
        }
    }
    if(targetNotFoundYet == 0){
        path = malloc(sizeof(node*) * ALLOC_INCREMENT);
        i = 1;
        path[0] = star2;
        aNode = star2;
        while(aNode != star1){
            ARR_CAPACITY_ADJUST(path, i, sizeof(node*), 1);
            aNode = prevStars[aNode->id];
            path[i++] = aNode;
            aNode = prevMovies[aNode->id];
            path[i++] = aNode;
        }
        *pathPtr = realloc(path, sizeof(node*) * i);
    }

    freeQueue(qPtr);
    free(prevMovies);
    free(prevStars);

    if(targetNotFoundYet == 0)
        return i/2;
    else
        return 0;
}

// Prints the path produced by findPath function
void printPath(node **path, int distance){
    int i;
    // distance*2 is index of last record, count of nodes in path wil be distance*2
    for(i = distance*2; i > 1; i-=2){
        printf("%s - %s \"%s\"\n", path[i]->name, path[i-2]->name, path[i-1]->name);
    }
}

// If search fails with given star name and there isn't a comma in the name
// it modifies given name like the format in the file and tries another search with that
// i.e. when starName given as "Ali PINAR", it will also try "PINAR, Ali"
node* smartSearch(hash_map hm, char *starName){
    int comma = 0;
    int i = 0;
    int newI = 0; //Next insert position in new string
    int skippedSpace = 0;
    int space = 0;
    char *newStr; // New string for modified star name
    node *star;
    while(starName[i]){
        if(starName[i] == ',') comma = i;
        else if(starName[i] == ' ') space++;
        i++;
    }
    star = searchHM(hm, starName);
    if(star == NULL && comma == 0){
        newStr = malloc(sizeof(char) * (i+2));
        for(i = 0; skippedSpace < space; i++){
            if(starName[i] == ' ') skippedSpace++;
        }
        comma = i-1; // where name ends
        while(starName[i]) newStr[newI++] = starName[i++];
        newStr[newI++] = ',';
        newStr[newI++] = ' ';
        for(i = 0; i<comma; i++) newStr[newI++] = starName[i];
        newStr[newI] = '\0';
        star = searchHM(hm, newStr);
        free(newStr);
    }
    return star;
}

// Report distrubition of values in hashmap
int reportHashMap(hash_map hm){
    int ranges[10];
    int i;
    int count;
    for(i= 0; i<10;i++) ranges[i] = 0;

    for(i = 0; i< HASHMAP_SIZE; i++){
        count = hm.counts[i];
        if(count > 0){
            count = count / 10;
            count++;
        }
        if(count > 9) count = 9;
        ranges[count]++;
    }   
    printf("\nHashMap value per cell distrubiton:\n");
    for(i = 0; i < 10; i++){
        if(i == 0){
            printf("Empty cells: \t");
        }else if(i == 9){
            printf("More Than 79: \t");
        }else{
            printf("Between %d-%d: \t", (i-1)*10, i*10-1);
        }
        printf("%d\n", ranges[i]);
    }
}

int main(){
    int movieCount;
    int starCount;
    char buffer[101], buffer2[101]; // To store inputs from user
    node **movies;
    node *aMovie; // Tmp to store porinter of movie node
    node *star, *star2; // temp vars to store a stars
    int i;
    int j;
    int distance;
    node *kevinsNode; // Node of Kevin Bacon
    node **path; // to Store path between two star
    char operation;
    hash_map hashMap;
    hashMap.arrays = calloc(HASHMAP_SIZE, sizeof(node**));
    hashMap.counts = malloc(HASHMAP_SIZE * sizeof(int));
    for(i = 0; i< HASHMAP_SIZE; i++) hashMap.counts[i] = 0;
    srand(time(0));

    system(CLEAR_COMMAND);
    printf("Generating tree and graph...\nPlease hang few seconds...\n");
    movies = generateGraphFromFile(hashMap, DATA_FILE, &movieCount, &starCount);
    printf("Total %d movie and %d star loaded.\n", movieCount, starCount);
    reportHashMap(hashMap);
    kevinsNode = searchHM(hashMap, "Bacon, Kevin");
    if(kevinsNode == NULL){
        printf("Couldn't find Kevin Bacon!\n");
        exit(1);
    }
    printf("Press Enter to continue...\n");
    while( getchar() != '\n');
    while(1){
        system(CLEAR_COMMAND);
        printf("Select Operation:\n1)Find Kevin Bacon Number of an artist.\n2)Find distance between two artist.\n");
        printf("3)Find distance between two random artist.\n4)Query an artist.\n0)Exit\n>");
        operation = getchar();
        if(operation != '\n') while(getchar() != '\n'); // To clear new line from stdin
        system(CLEAR_COMMAND);
        if(operation == '1'){
            printf("Type name of the artist: ");
            getLine(buffer, 100);
            star = searchHM(hashMap, buffer);
            system(CLEAR_COMMAND);
            if(star){
                distance = findPath(star, kevinsNode, movieCount, starCount, &path);
                if(distance == 0 || distance > 6){
                    printf("There isn't a connenction between artisy and Bacon!\n");
                }else{
                    printf("%s's Kevin Bacon Number: %d\n", buffer, distance);
                    printPath(path, distance);
                }
                if(distance != 0) free(path);

            }else{
                printf("Artist with name of \"%s\" not found!\n", buffer);
            }
        }else if(operation == '2'){
            printf("Type name of first artist: ");
            getLine(buffer, 100);
            star = searchHM(hashMap, buffer);
            system(CLEAR_COMMAND);
            if(star){
                printf("Type name of second artist: ");
                getLine(buffer2, 100);
                star2 = searchHM(hashMap, buffer2);
                system(CLEAR_COMMAND);
                if(star2){
                    distance = findPath(star, star2, movieCount, starCount, &path);
                    if(distance == 0){
                        printf("There isn't a connection between \"%s\" and \"%s\"\n", buffer, buffer2);
                    }else{
                        printf("Distance between \"%s\" and \"%s\": %d\n", buffer, buffer2, distance);
                        printPath(path, distance);
                    }
                    if(distance != 0) free(path);
                }else{
                    printf("Artist with name of \"%s\" not found!\n", buffer2);
                }
            }else{
                printf("Artist with name of \"%s\" not found!\n", buffer);
            }

        }else if(operation == '3'){
            // Since we don't have array of stars (normally not needed)
            // fist we will pick up a movie at random then we will pick up one of the star from that movie
            aMovie = movies[rand()%movieCount];
            star = aMovie->pairs[rand()%(aMovie->pairCount)];
            aMovie = movies[rand()%movieCount];
            star2 = aMovie->pairs[rand()%(aMovie->pairCount)];

            distance = findPath(star, star2, movieCount, starCount, &path);
            if(distance == 0){
                printf("There isn't a connection between \"%s\" and \"%s\"\n", star->name, star2->name);
            }else{
                printf("Distance between \"%s\" and \"%s\": %d\n", star->name, star2->name, distance);
                printPath(path, distance);
            }
            if(distance != 0) free(path);

        }else if(operation == '4'){
            printf("Type name of the artist: ");
            getLine(buffer, 100);
            star = searchHM(hashMap, buffer);
            if(star == NULL){
                printf("Star not found!\n");
            }else{
                printf("Star: %s\n", star->name);
                printf("Played %d movies in total:\n", star->pairCount);
                for(i = 0; i < star->pairCount; i++){
                    printf("\t%s\n", star->pairs[i]->name);
                }
            }
        }else if(operation == '0'){
            break;
        }else{
            continue;
        }
        printf("\nPress Enter to continue...\n");
        while(getchar() != '\n');
    }
    //dumpTree(treeRoot, 0);

    for(i = 0; i < movieCount; i++){
        free(movies[i]->name);
        if(movies[i]->pairCount > 0) free(movies[i]->pairs);
    }
    for(i = 0; i < HASHMAP_SIZE; i++){
        if(hashMap.counts[i]){
            for(j = 0; j < hashMap.counts[i]; j++){
                star = hashMap.arrays[i][j];
                free(star->name);
                if(star->pairCount) free(star->pairs);
                free(star);
            }
            free(hashMap.arrays[i]);
        }
    }
    free(hashMap.counts);
    free(hashMap.arrays);
    return 0;
}
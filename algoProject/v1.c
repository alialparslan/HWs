#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
// http://man7.org/linux/man-pages/man3/iconv.3.html
#include <iconv.h> // For conversion from cp1252 to utf-8

#define DATA_FILE "input-mpaa.txt"

#define DEBUG_MODE 0

#define CLEAR_COMMAND "clear"

#if DEBUB_MODE 
    #define DEBUG(p1, ...)  printf(p1, __VA_ARGS__)
#else
    #define DEBUG
#endif


#define BUFFER_SIZE 10
#define ALLOC_INCREMENT 10

// min_available has to be greater than 0 and less than ALLOC_INCREMENT
#define ARR_CAPACITY_ADJUST(arr, count, unit_size, min_available) \
    if(count > 0 && count % ALLOC_INCREMENT == 0) \
        arr = realloc(arr, unit_size * ( ALLOC_INCREMENT + count + min_available -1));



#define CREATE_ROOT_TREENODE(var) \
    var = malloc(sizeof(treeNode)); \
    var->str = malloc(sizeof(char)); \
    var->str[0] = '\0'; \
    var->ptr.childs = malloc(sizeof(treeNode*)); \
    var->ptr.childs[0] = NULL;

#define CREATE_STR(var, source, length) \
    var = malloc(sizeof(char) + (length + 1)); \
    var[length] = '\0'; \
    strncpy(var, source, length);

#define CONSUME_BUFFER(var, buffer, length) \
    var = realloc(buffer, sizeof(char) * (length + 1) ); \
    var[length] = '\0'; \
    buffer = malloc(sizeof(char)*ALLOC_INCREMENT);
    
//Both for stars and movies
typedef struct node{
    int id; // Unique id (Uniqe among same type)
    char type; // 0 => a movie; 1 => a star
    char *name;
    int pairCount;
    struct node **pairs; // aka edges
}node; // This is node for graph structure and only struct for graph, others are for star search tree
#define TYPE_MOVIE 0
#define TYPE_STAR 1

//Since we will access stars by searching with their name we will need another structure
// I preferred a tree that every leaf points to a star and accessed by their name in O(1) complexity
typedef struct treeNode{
    char *str; // If it is null than it is a leaf
    union{
        struct node *star;
        struct treeNode **childs;
    }ptr;
}treeNode;

// Search result on tree
typedef struct{
    // result == -1: Star found ptr contians a node*
    // result >= 0 : There isn't a star with such name, result is cmpStr variable until the leaf in ptr
    int result;
    union{
        node* star;
        treeNode *addHere; // Where should star to be added.
    }ptr;
}searchResult;

#define QUEUE_CONTAINER_SIZE 1000

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

// Searches star name on the tree 
// params : root node , name of the start
void searchStar(treeNode *node, searchResult *result, char *name){
    int i;
    int compare;
    char *strPtr; //temp variable
    treeNode **childs;
    treeNode *child;
    int cmpStart = 0;
    int lastI = 0;
    if(node->str){ //If node is not a leaf   
        childs = node->ptr.childs;
        child = *childs;
        while(child){
            if(child->str){ // Child is not leaf, so also has childs
                strPtr = child->str;
                for(i = 0; name[cmpStart+i] && name[cmpStart+i] == strPtr[i]; i++);
                if(i > 0){
                    if(strPtr[i] != '\0' || name[cmpStart+i] == '\0'){
                        result->result = cmpStart;
                        result->ptr.addHere = child;
                        return;
                    }
                    DEBUG("%s, %s, %d, %d\n", child->str, name, i, cmpStart);
                    node = child;
                    childs = child->ptr.childs;
                    child = *childs;
                    lastI = i;
                    cmpStart += i;
                }else{
                    child = *++childs;
                }
            }else{ // child is a leaf
                strPtr = child->ptr.star->name+cmpStart;
                for(i = 0; name[cmpStart+i] && name[cmpStart+i] == strPtr[i]; i++);
                DEBUG("Comparing with leaf: %s, %s, %d, %d\n", name, child->ptr.star->name, i, cmpStart);
                if(i>0){
                    if(name[cmpStart+i] == strPtr[i]){ // Which means both are NULL so they are same
                        DEBUG("Same name with leaf: %s\n", name);
                        result->result = -1;
                        result->ptr.star = child->ptr.star;
                    }else{
                        result->result = cmpStart;
                        result->ptr.addHere = child;
                        
                    }
                    DEBUG("add here: %s\n", child->ptr.star->name);
                    return;
                }
                child = *(++childs);
            }
            
        }
        if(!child){
            result->result = cmpStart- lastI;
            result->ptr.addHere = node;
            DEBUG("add here: %s\n", node->str);
            return;
        }
    }else{ // Node is a leaf // Actually this should never happen!
        result->result = cmpStart;
        result->ptr.addHere = node;
        return;
    }
}


// If not exists adds and returns star's node in any case
node* addStarBySearchResult(searchResult* result, node *starNode){
    DEBUG(">Adding %s\n", starNode->name);
    treeNode *treeNodeOfStar;
    treeNode *whereToAdd = result->ptr.addHere;
    treeNode *treeNodePtr; // temp var
    char *strPtr; // temp var
    int cmpStart = result->result;
    int i;
    int l; // temp
    char *name = starNode->name;

    treeNodeOfStar = malloc(sizeof(treeNode));
    treeNodeOfStar->str = NULL;
    treeNodeOfStar->ptr.star = starNode;

    // So far we allocated and completed creation of node for a star
    if(whereToAdd->str){ // It has childs and we will ad our start as another child (most probably)
        //Except our name is substring of this node, then we will need to create new treenode for others and change str
        strPtr = whereToAdd->str;
        for(i=0; strPtr[i] && strPtr[i] == name[cmpStart+i]; i++);
        if(strPtr[i] != '\0'){
            for(l = 0; strPtr[i+l];l++);
            treeNodePtr = malloc(sizeof(treeNode));
            treeNodePtr->str = malloc(sizeof(char) * (l+1));
            strcpy(treeNodePtr->str, strPtr+i);
            treeNodePtr->ptr.childs = whereToAdd->ptr.childs;
            whereToAdd->ptr.childs = malloc( sizeof(treeNode*) * 3);
            whereToAdd->ptr.childs[0] = treeNodePtr;
            whereToAdd->ptr.childs[1] = treeNodeOfStar;
            whereToAdd->ptr.childs[2] = NULL;
            strPtr = malloc(sizeof(char) * (i+1));
            whereToAdd->str[i] = '\0';
            strcpy(strPtr, whereToAdd->str);
            free(whereToAdd->str);
            whereToAdd->str = strPtr;
        }else{
            i = 0;
            while(whereToAdd->ptr.childs[i]) i++;
            whereToAdd->ptr.childs = realloc(whereToAdd->ptr.childs, sizeof(treeNode*) * (i + 2));
            whereToAdd->ptr.childs[i] = malloc(sizeof(treeNode));
            whereToAdd->ptr.childs[i]->str = NULL;
            whereToAdd->ptr.childs[i]->ptr.star = starNode;
            whereToAdd->ptr.childs[i+1] = NULL;
        }
    }else{
        // It is a leaf
        strPtr = whereToAdd->ptr.star->name+cmpStart; //Name of the star that leaf points to
        for(i=0; name[cmpStart + i] && name[cmpStart+i] == strPtr[i] ;i++);
        if(i == 0){
            printf("Error : This is not possible! addStar there isn't an overlap between addHere node and star to be added!\n");
            exit(1);
        }
        treeNodePtr = malloc(sizeof(treeNode));
        treeNodePtr->str = NULL;
        treeNodePtr->ptr.star = whereToAdd->ptr.star;
        whereToAdd->ptr.childs = malloc(sizeof(treeNode*) * 3);
        whereToAdd->ptr.childs[0] = treeNodePtr;
        whereToAdd->ptr.childs[1] = treeNodeOfStar;
        whereToAdd->ptr.childs[2] = NULL;
        whereToAdd->str = malloc(sizeof(char) * (i+1));
        whereToAdd->str[i] = '\0';
        for(i--; i >= 0;i--) whereToAdd->str[i] = strPtr[i];
    }
    return starNode;
}


node* addStar(treeNode *root, char* name, node *movie){
    searchResult search;
    node *starNode;

    searchStar(root, &search, name);
    if(search.result == -1) return search.ptr.star; // Star exists 

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
    return addStarBySearchResult(&search, starNode);
}



node* newNode(int type, char *name){
    node *n = malloc(sizeof(node));
    n->name = name;
    n->pairCount = 0;
    n->pairs = NULL;
    n->type = type;
    return n;
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
node** generateGraphFromFile(treeNode *root, char* file, int *totalMovie, int *totalStar){    
    //Those six needed for iconv (converting cp1252 to utf8) function
    iconv_t iconv_handle = iconv_open("UTF-8", "CP1252");
    size_t iconv_inbytesleft;
    size_t iconv_outbytesleft;
    size_t iconv_return;
    char *iconv_inbuf;
    char *iconv_outbuf;
    
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
    searchResult starSearchResult; 
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
                searchStar(root, &starSearchResult, buffer);
                if(starSearchResult.result == -1){
                    star = starSearchResult.ptr.star;
                }else{
                    starCount++;
                    NEW_NODE(star, TYPE_STAR, starCount);
                    CONSUME_BUFFER(star->name, buffer, i);
                    addStarBySearchResult(&starSearchResult, star);
                }
                pairNodes(movie, star);
                //printf("\t%s\n", star->name); // Star in a movie
            }else if(ch != '\n'){
                ARR_CAPACITY_ADJUST(movies, count, sizeof(node*), 1);
                NEW_NODE(movie, TYPE_MOVIE, count);
                CONSUME_BUFFER(movie->name, buffer, i);
                movies[count++] = movie;
                //printf("%s\n", movie->name); // Movie
            }
            buffer_capacity = ALLOC_INCREMENT;
            i = 0;
            if(ch == '\n'){
                movie = NULL;
                //if(count > 3) break;
            }
        }else{
            if(i > buffer_capacity-4){
                buffer_capacity += ALLOC_INCREMENT;
                buffer = realloc(buffer, buffer_capacity * sizeof(char) );
            }
            iconv_inbytesleft = 1;
            iconv_outbytesleft = 4;
            iconv_outbuf = buffer+i;
            iconv_inbuf = &ch;
            iconv_return = iconv(iconv_handle, &iconv_inbuf, &iconv_inbytesleft, &iconv_outbuf, &iconv_outbytesleft);
            if(iconv_return == -1) printf("iconv error: %s\n", strerror(errno));
            i += 4 - iconv_outbytesleft;
        }
    }
    movies = realloc(movies, sizeof(node*) * (count+1));
    movies[count] = NULL;
    *totalMovie = count;
    *totalStar = starCount;
    printf("Total: %d movies, %d stars\n", count, starCount);
    iconv_close(iconv_handle);
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

void dumpTree(treeNode *node, int depth){
    treeNode **childs;
    int i;

    if(node->str){ // has childs
        if(node->str[0] != '\0'){
            for(i = 0; i<=depth; i++) putchar(' ');
            printf("%s:\n", node->str);
        }
        childs = node->ptr.childs;
        while(*childs){
            dumpTree(*childs, depth + strlen(node->str));
            childs++;
        }
    }else{ // It is a leaf
        for(i = 0; i<=depth; i++) putchar(' ');
        printf("%s\n", node->ptr.star->name);
    }
}

/*
void queueTest(){
    queue q;
    queue *qPtr = &q;
    int temp;
    int i;
    initializeQueue(&q);
    for(i = 1; i< 2000; i++) enQueue(qPtr, i);
    i = 1;
    do{
        temp = deQueue(qPtr);
        if(i != temp){
            printf("Queue failed!\n");
            return;
        }
        if(i == 1999) break;
        i++;
    }while(temp);
    if(temp == 0) printf("Queue failed!\n");
}
int testTree(){
    char *rootStr = malloc(sizeof(char));
    rootStr[0] = '\0';
    treeNode **childs  = malloc(sizeof(treeNode*));
    childs[0] = NULL;
    // Only root will have Null string. (Important that it is not null pointer it is a string with zero length)
    // Since null pointer means its a leaf
    treeNode root = { .str = rootStr, .ptr.childs = childs}; 
    addStar(&root, strFromConst("Ali"), NULL);
    addStar(&root, strFromConst("Mehmet"), NULL);
    addStar(&root, strFromConst("Hasan"), NULL);
    addStar(&root, strFromConst("Alparslan"), NULL);
    addStar(&root, strFromConst("Ali1"), NULL);
    addStar(&root, strFromConst("Ali Alparslan PINAR"), NULL);
    addStar(&root, strFromConst("Alper"), NULL);
    addStar(&root, strFromConst("Mahmut "), NULL);
    addStar(&root, strFromConst("Hasan "), NULL);
    addStar(&root, strFromConst("Hasan 12"), NULL);
    addStar(&root, strFromConst("Has"), NULL);
    dumpTree(&root, 0);
}*/

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
void smartSearch(treeNode *root, searchResult *result, char *starName){
    int comma = 0;
    int i = 0;
    int newI = 0; //Next insert position in new string
    int skippedSpace = 0;
    int space = 0;
    char *newStr; // New string for modified star name
    while(starName[i]){
        if(starName[i] == ',') comma = i;
        else if(starName[i] == ' ') space++;
        i++;
    }
    searchStar(root, result, starName);
    if(result->result != -1 && comma == 0){
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
        searchStar(root, result, newStr);
        free(newStr);
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
    int distance;
    node *kevinsNode; // Node of Kevin Bacon
    node **path; // to Store path between two star
    char operation;
    treeNode *treeRoot;
    searchResult starSearchResult;

    srand(time(0));

    CREATE_ROOT_TREENODE(treeRoot);
    system(CLEAR_COMMAND);
    printf("Generating tree and graph...\nPlease hang few seconds...\n");
    movies = generateGraphFromFile(treeRoot, DATA_FILE, &movieCount, &starCount);

    searchStar(treeRoot, &starSearchResult, "Bacon, Kevin");
    if(starSearchResult.result != -1){
        printf("Couldn't find Kevin Bacon!\n");
        exit(1);
    }
    kevinsNode = starSearchResult.ptr.star;

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
            smartSearch(treeRoot, &starSearchResult, buffer);
            system(CLEAR_COMMAND);
            if(starSearchResult.result == -1){
                distance = findPath(starSearchResult.ptr.star, kevinsNode, movieCount, starCount, &path);
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
            smartSearch(treeRoot, &starSearchResult, buffer);
            system(CLEAR_COMMAND);
            if(starSearchResult.result == -1){
                star = starSearchResult.ptr.star;
                printf("Type name of second artist: ");
                getLine(buffer2, 100);
                smartSearch(treeRoot, &starSearchResult, buffer2);
                system(CLEAR_COMMAND);
                if(starSearchResult.result == -1){
                    star2 = starSearchResult.ptr.star;
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
            smartSearch(treeRoot, &starSearchResult, buffer);
            if(starSearchResult.result != -1){
                printf("Star not found!\n");
            }else{
                star = starSearchResult.ptr.star;
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
        printf("Press Enter to continue...\n");
        while(getchar() != '\n');
    }
    //dumpTree(treeRoot, 0);
}
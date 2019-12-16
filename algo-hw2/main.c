#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#if defined(_WIN32)
    #define WINDOWS 1 // Windows
#elif defined(_WIN64)
    #define WINDOWS 1 // Windows
#endif

#if defined(WINDOWS)
    #define FOLDER_SIGN '\\'
    #define WRONG_FOLDER_SIGN '/'
#else
    #define FOLDER_SIGN '/'
    #define WRONG_FOLDER_SIGN '\\'
#endif

#define BUFFER_SIZE 100

// TabloUzunlugu = EnküçükAsalSayı >= TablodakiElemanSayısı / LoadFactor
// Tablodaki eleman sayısı 20 varsayılarak. 20/0.6 = 33.33; M = 37
#define M 37
#define R 17
#define INDEX_FILE_NAME "Index.txt"
#define DIRECTORY "" //Directory that files listed in index file, stored.

typedef char* hashTable[M];

// Takes two path string, joins them and returns newl string.
char* joinPaths(char *basePath, char *extendPath){
    int baseLen = 0; // Length of basePath
    int extendLen = 0;  // Length of extendPath
    char *newPath = NULL; // Pointer for new string
    while(basePath[baseLen]){
        if(basePath[baseLen] == WRONG_FOLDER_SIGN) basePath[baseLen] = FOLDER_SIGN;
        baseLen++;
    }
    extendLen = strlen(extendPath);
    
    if(basePath[baseLen-1] == FOLDER_SIGN) baseLen--;
    
    extendLen = strlen(extendPath);
    
    if(baseLen > 0){
        if(extendPath[0] != FOLDER_SIGN) baseLen++; // For slash
    }else if(extendLen == 0){
        return NULL;
    }
    newPath = malloc(sizeof(char)*(baseLen+extendLen+1)); // +1 For slash and null terminator
    strcpy(newPath, basePath);
    if(baseLen > 0 && extendPath[0] != FOLDER_SIGN) newPath[baseLen-1] = FOLDER_SIGN;
    strcpy(newPath+baseLen, extendPath);
    return newPath;
}

// Extracts file name from path.
char* extractFileName(char* path){
    char* file = NULL;
    int lastSlash = -1;
    int i = 0;
    int j = 0;
    while(path[i]){
        if(path[i] == FOLDER_SIGN) lastSlash = i;
        i++;
    }
    i--;
    if(lastSlash == i) return NULL; // path ends with folder sign
    file = malloc(sizeof(char)*(i-lastSlash+1));
    if(!file) return NULL;
    path = path+lastSlash+1;
    i -= lastSlash;
    for(j = 0; j<=i; j++) file[j] = path[j];
    file[j] = '\0';
    return file;
}

// To use when adding new files. It moves file to DIRECTORY if it is not already in there.
void moveToDirectory(char *filePath){
    int needToMove = 0;
    int i = 0;
    char *fileName; // To store filename temporarly when moving file
    char *newPath; // To store where to move file
    while(DIRECTORY[i]){
        if(!filePath[i] || filePath[i] != DIRECTORY[i]) needToMove = 1;
        i++;
    }

    if(needToMove){
        printf("Moving the file..\n");
        fileName = extractFileName(filePath);
        newPath = joinPaths(DIRECTORY, fileName);
        rename(filePath, newPath);
        free(fileName);
        free(newPath);
    }
}


// Returns 0 if contents of files are the same, 1 if different, -1 in case of error
int compareFiles(char *path1, char *path2){
    int result = 0;
    FILE *fp1, *fp2;
    char buffer1[BUFFER_SIZE], buffer2[BUFFER_SIZE];
    fp1 = fopen(path1, "r");
    fp2 = fopen(path2, "r");
    if(!fp1){
        printf("Failed to open file %s\n", path1);
        if(fp2) fclose(fp2);
        return -1;
    }
    if(!fp2){
        printf("Failed to open file %s\n", path2);
        if(fp1) fclose(fp1);
        return -1;
    }
    while(result == 0 && fgets(buffer1, BUFFER_SIZE, fp1)){
        if(fgets(buffer2, BUFFER_SIZE, fp2)){
            if(strcmp(buffer1,buffer2) != 0) result = 1;
        }else{
            result = 1;
        }
    }
    fclose(fp1);
    fclose(fp2);
    return result;
}

// Ödevde hash'ı hesaplarken kelime kelime hesaplanması söylenmiş. Ayrıca boşlukların da dahil olduğu belirtilmiş.
// Boşluklar herhangi bir çarpanla çarpılmadan ascii değeri direk hash'a eklenmiştir.
unsigned long int wordHash(char* data, int n){
    unsigned long int hash = 0;
    unsigned long int multiplier = 1;
    int i;
    for(i = n-1; i >= 0; i--){
        hash += (data[i] * multiplier);
        multiplier = (multiplier*R);
    }
    return hash;
}

unsigned long int calcFileKey(char* filePath){
    int i;
    int wordStart;
    unsigned long int key = 0;
    FILE* fp;
    char buffer[BUFFER_SIZE];
    fp = fopen(filePath, "rb");
    if(!fp){
        printf("(calcFileKey)Failed to open file %s\n", filePath);
        return 0;
    }
    i = 0;
    while(fgets(buffer+i, BUFFER_SIZE-i, fp)){
        wordStart = 0;
        for(i = 0; buffer[i] != 0; i++){
            if(buffer[i] == ' ' || buffer[i] == '\n'){
                if(buffer[i] == ' ') key += buffer[i];
                if(i > wordStart){
                    key += wordHash(buffer + wordStart,i-wordStart);
                }
                wordStart = i+1;
            }
        }
        i = 0;
        while(buffer[wordStart+i]){ // Eğer kelime yarıda kaldıysa
            buffer[i] = buffer[wordStart+i];
            i++;
        }
    }
    if(i > 0) key += wordHash(buffer, i);

    fclose(fp);
    return key;
}
// Takes calculated key of a file and returns n(th) hash
int calcHashByKey(unsigned long int key, int n){
    int hash = key%M;
    for(int i=1; i<n; i++){
        hash +=  ( i+( key % (M-i)));
    }
    return hash%M;
}

// Adds a filename to index file
void addToSamples(char *fileName){
    FILE *fp = fopen(INDEX_FILE_NAME, "a");
    if(!fp){
        printf("Failed to open %s\n", INDEX_FILE_NAME);
        return;
    }
    fprintf(fp, "\n%s", fileName);
    fclose(fp);
}

// Return 1 in case of success, 0 if file with same content exists, -1 if an error occurs
// last parameter is for when adding new file (file that is not listed in sample.doc)
int addFile(hashTable table, char* filePath, int newFile){
    char *tempPath; // To store paths temporarly
    int i = 1;
    int result = 1;
    int wordStart;
    int hash;
    unsigned long int key = calcFileKey(filePath);
    if(key == 0){
        printf("Failed to calc hash of %s\n", filePath);
        return -1;
    }
    hash = calcHashByKey(key, i);
    while(i != -1){
        if(table[hash]){
            tempPath = joinPaths(DIRECTORY, table[hash]);
            result = compareFiles(tempPath, filePath);
            //printf("Comparing: %s and %s, result: %d\n", tempPath, filePath, result);  
            free(tempPath);
            if(result != 1){
                i = -1;
            }else{
                hash = calcHashByKey(key, ++i);
            }
        }else{
            table[hash] = extractFileName(filePath);
            i = -1;
        }
    }
    if(result == 1 && newFile){
        addToSamples(table[hash]);
        moveToDirectory(filePath);
    }
    return result;
    printf("hash: %ld, %d \n", key, hash);
}

void dumpTable(hashTable table){
    for(int i = 0; i<M; i++){
        if(table[i]){
            printf("%d.\t%s\n",i, table[i]);
        }
    }
}

int main(){
    char trash[50]; // Not related to program flow, will be used to clear stdin
    hashTable table;
    int control; // To control program flow, stores user command
    int i;
    int dirLen = sizeof(DIRECTORY);
    char* line = NULL;
    char* path;
    char tempPath[300];
    size_t lineLen = 0;

    // Init hash table with null pointers
    for(i = 0; i < M; i++) table[i] = NULL;

    FILE* fd = fopen(INDEX_FILE_NAME, "r");
    if(!fd){
        printf("Error when opening index file!");
    }
    while((i = getline(&line, &lineLen, fd)) != -1){

        for(i=0; line[i] > 25;i++); // Skip till last character that could pe part of name of the file
        line[i] = '\0'; // Place null terminator where it should be

        if(i > 0){
            if(line[0] == '\r' || line[0] == '\n'){
                while(!line[i+1]) line[i] = line[i+1];
            }
            path = joinPaths(DIRECTORY, line);
            printf("path: >%s<\n", path);
            addFile(table, path, 0);
            free(path);
        }

    }
    if(line) free(line);

    control = 0;
    while(control != 3){
        printf("<>Please Select Operation<>\n1) For adding new file.\n2) For listing files.\n3) To exit.\n> ");
        scanf("%d", &control);
        switch (control)
        {
        case 1:
            printf("Type path of the file:");
            scanf("%s", tempPath);
            i = addFile(table, tempPath, 1);
            if(i == 0) printf("A file with same content already exists!\n");
            if(i == 1) printf("File successfully added.\n");
            if(i == -1) printf("An error occured!\n");
            break;
        case 2:
            dumpTable(table);
            break;
        case 3:
            break;
        default:
            printf("Unexpected input!\n");
            break;
        }
        if(control != 3){
            printf("Press something to continue...");
            fgets(trash, sizeof(trash), stdin);
            getchar();
        }
    }
    
    fclose(fd);
    for(int i = 0; i<M; i++){
        if(table[i]) free(table[i]);
    }  
}
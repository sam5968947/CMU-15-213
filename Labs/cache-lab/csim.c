#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "cachelab.h"


typedef struct{ // 8 + 8 = 16 bytes
    uint32_t vaild; // 4bytes
    uint32_t tag; // 4bytes
    uint64_t timeStamp; // for LRU, 8bytes
}cacheLine;

#define MAX_LEN         256 // 1bytes = 8bits
#define U64MAX          0xFFFFFFFFFFFFFFFF // 8bytes = 64bits            

static cacheLine** virtual_cache = NULL; // Create a array of E*S size
static uint8_t s=0, E=0, b=0;
static int miss=0, hits=0, evictions=0;
static uint64_t ticks=0; // counter
static bool print_msg = false;


/**
 * func for getting operator
 */ 
static char get_operation(char* str){

    if(strlen(str) <= 1){
        return '\0';
    }

    return str[1];
}


//221027
/**
 * func for applying LRU algo.
 */ 
static uint8_t LRU(uint8_t set){

    int repalce = 0;

    for(int i=1; i<E; i++){

        repalce = virtual_cache[set][i].timeStamp < virtual_cache[set][repalce].timeStamp ? i : repalce;
    }

    return repalce;
}

//221027
/**
 * func for loading data from cache
 */ 
static void load_operation(char* line){

    uint64_t addr=0;
    uint32_t dataBytes=0;
    char op;

    // linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace
    // ex : line = " S 18,1" --> (S M L I) (address = t+s+b) (size)
    // use sscanf to replace regex --> op = 'S', addr = 12, dataBytes = 1
    sscanf(line, " %c %lx,%u", &op, &addr, &dataBytes);

    if(op != 'L' && op != 'M' && op != 'S'){

        return;
    }

    if(print_msg){

        printf("%c %lx,%u ", op, addr, dataBytes);
    }    

    uint32_t set = (addr >> b) & (~(U64MAX << s));
    uint32_t tag = addr >> (s+b);

    bool find = false;
    int empty_line = -1;

    for(int i=E-1; i>=0; i--){

        if(virtual_cache[set][i].vaild && virtual_cache[set][i].tag == tag){

            find = true;
            virtual_cache[set][i].timeStamp = ticks;
            break;
        }else if(virtual_cache[set][i].vaild == 0){

            empty_line = i;
        }
    }

    //def of pdf
    hits = op == 'M' ? hits+1 : hits; 

    if(find){

        hits++;

        if(print_msg){

            printf("hit ");
        }        
    }else{

        miss++;

        if(print_msg){

            printf("miss ");
        }
          
        /* eviction occur */
        if(empty_line == -1){

            evictions++;
            empty_line = LRU(set);
            
            if(print_msg){

                printf("eviction ");
            }             
        }

        virtual_cache[set][empty_line].vaild = 1;
        virtual_cache[set][empty_line].tag = tag;
        virtual_cache[set][empty_line].timeStamp = ticks;
    }

    if(op == 'M' && print_msg){

        printf("hit ");
    } 

    if(print_msg){

        printf("\n");
    }  

}

/**
 * func for parsing commands from trace file
 */ 
static void cmd_parsing(char* filename){

    char line[MAX_LEN];
    FILE* fp = fopen(filename, "r");

    while(!feof(fp) && !ferror(fp)){

        strcpy(line, "\n"); //default line = "\n"
        fgets(line, MAX_LEN, fp); // if we don't get info from fp, line is still the same.
        load_operation(line);

        ticks++;        
    }

    fclose(fp);    
}


/**
 * Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>
 * Options:
 *   -h         Print this help message.
 *   -v         Optional verbose flag.
 *   -s <num>   Number of set index bits.
 *   -E <num>   Number of lines per set.
 *   -b <num>   Number of block offset bits.
 *   -t <file>  Trace file.*
 *
 * Examples:
 *   linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace
 *   linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace
 */ 
static void print_help_message(){

    printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n");

    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");

    printf("Examples:\n");
    printf("  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}


/**
 * func for initializing cache memory
 */ 
static void cache_init(uint8_t s, uint8_t E){

    uint32_t sets = 1 << s;

    //sizeof(int*) = 8, means the systems is 64 bits. eles sizeof(int*) = 4, ... 32 bits.
    // need to draw the virtual_cache
    virtual_cache = (cacheLine**)malloc(sizeof(cacheLine*) * sets);

    for(int i=0; i<sets; i++){
        virtual_cache[i] = (cacheLine*)malloc(sizeof(cacheLine) * E);
    }
}


/**
 * func for releasing cache memory
 */ 
static void free_cache(){

    uint32_t sets = 1 << s;

    for(int i=0; i<sets; i++){

        free(virtual_cache[i]);
        virtual_cache[i] = NULL;
    }

    free(virtual_cache);
    virtual_cache = NULL;
}


/**
 * main func
 */ 
int main(int argc, char* argv[]){

    char filename[MAX_LEN];
    uint8_t input_check = 0;

    for(int i=1; i<argc; i++){

        char op = get_operation(argv[i]); //op = 'h' 'v' 's' 'E' 'b' 't'...

        switch(op){

            case 'h':
                print_help_message();
            break;

            case 'v':
                print_msg = true;
            break;

            case 's':
                s = atoi(argv[++i]); // get the parameter
                input_check++; // count the total parameters(need all parameters -> s、E、b、t)
            break;

            case 'E':
                E = atoi(argv[++i]);
                input_check++;
            break;

            case 'b':
                b = atoi(argv[++i]);
                input_check++;
            break;

            case 't':
                strcpy(filename, argv[++i]); // use strcpy to get filename
                input_check++;
            break;

            default:
            break;
        }
    }

    if(input_check < 4){

        exit(0);
    }

    cache_init(s, E);
    cmd_parsing(filename);
    printSummary(hits, miss, evictions);//c make file
    free_cache();

    return 0;
}

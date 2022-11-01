#include "cachelab.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct
{                       // 8 + 8 = 16 bytes
    uint32_t vaild;     // 4bytes
    uint32_t tag;       // 4bytes
    uint64_t timeStamp; // for LRU, 8bytes
} cacheLine;

#define MAX_LEN 256               // 1bytes = 8bits
#define U64MAX 0xFFFFFFFFFFFFFFFF // 8bytes = 64bits

static cacheLine **virtual_cache = NULL; // Create a array of S*E size
static uint8_t s = 0, E = 0, b = 0;
static int miss = 0, hits = 0, evictions = 0;
static uint64_t ticks = 0; // counter
static bool print_msg = false;

static void cache_init()
{
    uint32_t S = 1 << s;

    virtual_cache = (cacheLine **)malloc(sizeof(cacheLine *) * (S));
    for (int i = 0; i < S; i++)
    {
        virtual_cache[i] = (cacheLine *)malloc(sizeof(cacheLine) * (E));
    }
}

static void cache_free()
{
    uint32_t S = 1 << s;

    for (int i = 0; i < S; i++)
    {
        free(virtual_cache[i]);
        virtual_cache[i] = NULL;
    }

    free(virtual_cache);
    virtual_cache = NULL;
}

static void print_help_message()
{

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

static char get_operation(char *str)
{

    if (strlen(str) <= 1)
    {
        return '\0';
    }

    return str[1];
}

static int LRU(uint64_t set)
{

    int replace = 0;

    for (int i = 0; i < E; i++)
    {
        replace = virtual_cache[set][i].timeStamp < virtual_cache[set][replace].timeStamp ? i : replace;
    }

    return replace;
}

static void load_operation(char *line)
{
    char ord;               // the command ex : L M S I
    uint64_t addr = 0;      // addr = t + s + b
    uint32_t dataBytes = 0; // the required size

    sscanf(line, " %c %u,%u", &ord, &addr, &dataBytes); // ???

    if (ord != 'L' && ord != 'M' && op != 'S')
        return;

    if (print_msg)
    {
        print("%c %u,%u\n", ord, addr, dataBytes);
    }

    // ex: addr = (1)(11)(011)
    uint64_t set = (addr >> b) & ~(U64MAX << s); // ex : (000111) & ~(1111100)
    uint64_t tag = addr >> (s + b);

    bool find = false;
    int empty_line = -1;

    for (int i = 0; i < E; i++)
    {
        if (virtual_cache[set][i].vaild && virtual_cache[set][i].tag == tag)
        {
            find = true;
            virtual_cache[set][i].timeStamp = ticks;
            break;
        }
        // eviction occur -->  empty_line != -1
        else if (!virtual_cache[set][i].vaild)
        {
            empty_line = i;
        }
    }

    // def of pdf
    hits = ord == 'M' ? hits + 1 : hits;

    if (find)
    {

        hits++;

        if (print_msg)
        {

            printf("hit ");
        }
    }
    else
    {

        miss++;

        if (print_msg)
        {

            printf("miss ");
        }

        /* eviction occur */
        if (empty_line == -1)
        {

            evictions++;
            empty_line = LRU(set);

            if (print_msg)
            {

                printf("eviction ");
            }
        }

        virtual_cache[set][empty_line].vaild = 1;
        virtual_cache[set][empty_line].tag = tag;
        virtual_cache[set][empty_line].timeStamp = ticks;
    }

    if (ord == 'M' && print_msg)
    {

        printf("hit");
    }

    if (print_msg)
    {

        printf("\n");
    }
}

static void cmd_parsing(char *filename)
{
    FILE *fp = NULL;
    fp = fopen(filename, "r");
    char line[MAX_LEN];

    // while(!feof(fp) && !ferror(fp)){

    //     strcpy(line, "\n"); //default line = "\n"
    //     fgets(line, MAX_LEN, fp); // if we don't get info from fp, line is still the same.
    //     load_operation(line);

    //     ticks++;
    // }

    while (!feof(fp) && !ferror(fp))
    {
        if (fgets(line, MAX_LEN, fp) == NULL)
            break;
        load_operation(line);
        ticks++;
    }

    fclose(fp);
}

int main(int argc, char *argv[])
{
    char filename[MAX_LEN];
    uint8_t input_check = 0;

    // get global variable from argv[i]
    for (int i = 0; i < argc; i++)
    {
        char op = get_operation(argv[i]);

        switch (op)
        {
        case 'h':
            print_help_message();
            break;

        case 'v':
            print_msg = true;
            break;

        case 's':
            s = atoi(argv[++i]);
            input_check++;
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
            strcpy(filename, argv[++i]);
            input_check++;
            break;

        default:
            break;
        }
    }

    if (input_check < 4)
    {
        exit(0);
    }

    cache_init();
    cmd_parsing(filename); // get all command from .trace file
    printSummary(hits, miss, evictions);
    cache_free();

    return 0;
}

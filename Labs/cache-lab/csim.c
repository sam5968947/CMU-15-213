#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "cachelab.h"

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

// cache大小為S*E*B or (E*cacheLine)*S，也可理解為每個組有E個cacheLine
static void cache_init()
{
    uint32_t S = 1 << s;

    virtual_cache = (cacheLine **)malloc(sizeof(cacheLine *) * (S));
    for (int i = 0; i < S; i++)
    {
        virtual_cache[i] = (cacheLine *)malloc(sizeof(cacheLine) * (E));
    }
}

// 與cache_init相呼應，必須釋放已申請的空間
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
    char op;               // the command ex : L M S
    uint64_t addr = 0;      // addr = t + s + b
    uint32_t dataBytes = 0; // the required size

    sscanf(line, " %c %lx,%u", &op, &addr, &dataBytes);

    if (op != 'L' && op != 'M' && op != 'S')
        return;

    if (print_msg)
    {
        printf("%c %lx,%u\n", op, addr, dataBytes);
    }

    // ex: addr = (1)(11)(011)
    uint64_t set = (addr >> b) & ~(U64MAX << s); // ex : (000111) & ~(1111100)
    uint64_t tag = addr >> (s + b);

    bool find = false;
    int empty_line = -1;

    /* 遍歷cache查看是否存在該數據 */
    for (int i = 0; i < E; i++)
    {
        /* 緩存命中，同時更新時間 */
        if (virtual_cache[set][i].vaild && virtual_cache[set][i].tag == tag)
        {
            find = true;
            virtual_cache[set][i].timeStamp = ticks;
            break;
        }
        /* 找到空的行 */
        else if (!virtual_cache[set][i].vaild)
        {
            empty_line = i;
        }
    }

    // def of pdf
    /* 若為修改指令，因為會寫回所以hit數一定要加一 */
    hits = op == 'M' ? hits + 1 : hits;

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

        /* 若不命中且沒有空行，就會發生eviction */
        if (empty_line == -1)
        {

            evictions++;
            /* 使用LRU找需要替換的行 */
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

    if (op == 'M' && print_msg)
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
            s = atoi(argv[++i]); // get the parameter, str->int
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

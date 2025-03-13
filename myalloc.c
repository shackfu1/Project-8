#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/mman.h>
#define PTR_OFFSET(p, offset) ((void*)((char *)(p) + (offset)))

// Please do not change the order or type of the fields in struct block

struct block {
    int size;        // bytes
    int in_use;      // bool
    struct block *next;
};

int padded_size(int size){
	int mod = size % 16;
	if (mod != 0){
		return size + (16 - mod);
	}else{
		return size;
	}
}

static struct block *head = NULL;

void *heap = NULL;

struct block *find_space_recursive(int size, struct block *current_head){
    if ((current_head->in_use == 1) | (current_head->size < size)){
        if (current_head->next != NULL){
            return find_space_recursive(size, current_head->next);
        }else{
            return NULL;
        }
    }else{
        return current_head;
    }
}

void *myalloc(int size)
{
    size = padded_size(size);

	if (heap == NULL){
		heap = mmap(NULL, 1024, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
		head = heap;
		int data_size = 1024 - padded_size(sizeof(struct block));
		head->size = data_size;
		head->in_use = 0;
	}
    struct block *found_space = find_space_recursive(size, head);
    if (found_space != NULL){
        found_space->in_use = 1;
        int split_size = size + padded_size(sizeof(struct block));
        if (found_space->size >= split_size + 16){
            struct block *new_block = found_space;
            new_block += (split_size / padded_size(sizeof(struct block)));
            new_block->size = found_space->size - split_size;
            new_block->in_use = 0;
            new_block->next = NULL;
            found_space->next = new_block;
            found_space->size = size;
        }
        return (found_space) + padded_size(sizeof(struct block));
    }else{
        return NULL;
    }

}

void myfree(void *p)
{
    struct block *block = p - 256;
    bool merged = false;
    block->in_use = 0;

    // free space coalescing below

    struct block *b = head;

    while (b != NULL) {
        merged = false;
        if (b->in_use == 0 && b->next != NULL && b->next->in_use == 0) {
            b->size = b->size + b->next->size + padded_size(sizeof(struct block));
            b->next = b->next->next;
            merged = true;
        }

        if (merged == false) {
            b = b->next;
        }else{
            b = head;
        }
    }
}

// ---------------------------------------------------------
// No mods past this point, please
// ---------------------------------------------------------

void print_data(void)
{
    struct block *b = head;

    if (b == NULL) {
        printf("[empty]\n");
        return;
    }

    while (b != NULL) {
        printf("[%d,%s]", b->size, b->in_use? "used": "free");
        if (b->next != NULL) {
            printf(" -> ");
        }
        fflush(stdout);

        b = b->next;
    }

    printf("\n");
}

int parse_num_arg(char *progname, char *s)
{
    char *end;

    int value = strtol(s, &end, 10);

    if (*end != '\0') {
        fprintf(stderr, "%s: failed to parse numeric argument \"%s\"\n", progname, s);
        exit(1);
    }

    return value;
}

/*
 * Usage:
 *
 * You can specify the following commands:
 *
 * p       - print the list
 * a size  - allocate `size` bytes
 * f num   - free allocation number `num`
 *
 * For example, if we run this:
 *
 *   ./myalloc a 64 a 128 p f 2 p f 1 p
 *
 * Allocation #1 of 64 bytes occurs
 * Allocation #2 of 128 bytes occurs
 * The list is printed
 * Allocation #2 is freed (the 128-byte chunk)
 * The list is printed
 * Allocation #1 is freed (the 64-byte chunk)
 * The list is printed
 *
 * Failed allocations aren't counted for the 'f' argument, for example:
 *
 *   ./myalloc a 999999 f 1
 *
 * is an error, since that allocation will have failed.
 */
int main(int argc, char *argv[])
{
    if (argc == 1) {
        fprintf(stderr, "usage: %s [p|a size|f index] ...\n", argv[0]);
        return 1;
    }

    int i = 1;

    // This is how many allocs we can support on the command line
    void *ptr[128];
    int ptr_count = 0;

    while (i < argc) {
        if (strcmp(argv[i], "p") == 0)
            print_data();

        else if (strcmp(argv[i], "a") == 0) {
            i++;

            int size = parse_num_arg(argv[0], argv[i]);
            
            void *p = myalloc(size); 

            if (p == NULL)
                printf("failed to allocate %d byte%s\n", size, size == 1? "": "s");
            else
                ptr[ptr_count++] = p;

        } else if (strcmp(argv[i], "f") == 0) {
            i++;

            if (argv[i] == NULL) {
                fprintf(stderr, "%s: missing num argument for 'f' command\n", argv[0]);
                return 1;
            }

            int index = parse_num_arg(argv[0], argv[i]);

            if (index < 1 || index > ptr_count) {
                fprintf(stderr, "%s: 'f' command: argument %d out of range\n", argv[0], index);
                return 1;
            }

            myfree(ptr[index - 1]);

        } else {
            fprintf(stderr, "%s: unknown command: %s\n", argv[0], argv[i]);
            return 1;
        }

        i++;
    }
}

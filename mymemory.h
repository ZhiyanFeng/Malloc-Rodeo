
#define MN 8888
#define page 4096


typedef struct header{
	int size;
	int magicNumber;
}header;

typedef struct freeListNode{
	int size;
	struct freeListNode *next;

}node;

int mymalloc_increase(unsigned int size);

void *mymalloc(unsigned int size);

unsigned int myfree(void *ptr);

int mymalloc_init();



node *extendFreeListBeforeHead(node *head, header *toFree);

unsigned int extendFreeListAfterHead(node *head, header *toFree);
/* mymalloc.c
 * 
 * This program implements malloc. See http://web.eecs.utk.edu/~plank/plank/classes/cs360/360/labs/lab7/index.html for detailed information.
 */
#include "mymalloc.h"
#include <string.h>

void *malloc_head = NULL; // global variable to keep track of begining of a list of free nodes

// free list strcture has size and points to next node in free list
typedef struct flist {
	int size;
	struct flist *flink;
} *Flist;

// pads given size to multiples of 8
int align(size){
	return ((((size - 1)>>3)<<3) + 8);
}

// finds the last node in free list
Flist find_last_free(){
	Flist tmp;

	tmp = (Flist)free_list_begin();
	
	if (tmp == NULL){
		return NULL;
	}

	while(tmp->flink != NULL){
		tmp = tmp->flink;
	}

	return tmp;
}

// mallocs a new block of memory with size already aligned given as parameter
void *my_malloc_new(size_t size){
	int mallocSize;
	Flist freeList, freeLast;
	int* Bottom;
	int* Top;
	int* returning;
	int sbrkSize;
	void* sbrkBegin;

	mallocSize = size;
	
	// if mallocSize is less than min sbrkSize of 8192, then call sbrk with with 8192, else call with the mallocSize given
	if (mallocSize <= 8192){
		sbrkSize = 8192;
		sbrkBegin = (void*) sbrk(8192);
		if ( sbrkBegin == (void*) -1){
			perror("inside my_malloc");
			exit(1);
		}
		if ((8192-mallocSize) <= 8){
			mallocSize = 8192;
		}
	}else{
		sbrkSize = mallocSize;
		sbrkBegin = (void*) sbrk(mallocSize);
		if (sbrkBegin == (void*) -1){
			perror("inside my_malloc");
			exit(1);
		}
	}

	// finds the begining of user block and 
	Bottom = (int*)(sbrkBegin + sbrkSize);
	Top = (int*)(sbrkBegin + sbrkSize - mallocSize);
	*(Top) = mallocSize;

	// create free list
	if (mallocSize < 8192){
		if (malloc_head == NULL){
			malloc_head = sbrkBegin;
			//printf("Creating free list: set freeList head to point to sbrkBegin at %p\n", sbrkBegin);

			freeList = (Flist) malloc_head;
			freeList->size = sbrkSize - mallocSize;
			freeList->flink = NULL;
			//printf("freeList head has size %d at %p and null flink\n", sbrkSize - mallocSize, malloc_head);
		}else{
			//printf("appending to free list a new node: address %p\n", sbrkBegin);
			freeList = (Flist) sbrkBegin;
			freeList->size = sbrkSize - mallocSize;
			freeList->flink = NULL;
			//printf("size of new node is %d\n", sbrkSize-mallocSize);

			freeLast = find_last_free();
			freeLast->flink = freeList;
			//printf("appended new node %p to end of freeList\n", freeLast->flink);
		}
	}


	returning = (int*)(Top + 2);
	//printf("returning address 0x%x\n", returning);

	return (void *) (Top + 2);
}

void *check(size_t freeSize){
	void* n;
	Flist f;

	n = free_list_begin();
	//printf("free_list_begin() gives %p\n", n);
	while(n != NULL){
		f = (Flist) n;
		if ((f->size) >= freeSize){
			return n;
		}
		n = (void*) f->flink;
	}

	return NULL;
}

void *free_list_prev(void* node){
	void* tmp;
	Flist f;

	tmp = free_list_begin();
	if (node == tmp){
		return NULL;
	}
	
	f = (Flist) tmp;
	while(f->flink != NULL){
		if ((void*) f->flink == node){
			return (void*)f;
		}
		f = f->flink;
	}
	
	fprintf(stderr, "wrong node passed in free_list_prev\n");
	exit(1);
}

void *my_malloc(size_t size){
	int mallocSize, alignedSize;
	Flist freeList, ftmp, flast, fnext;
	int* Bottom;
	int* Top;
	int* returning;
	int sbrkSize;
	void *tmp;
	int firstNodeSize;
	int resultSize;
	void *freeNode;
	void *prev;
	void *next;

	alignedSize = align(size);
	mallocSize = alignedSize + 8;
	//printf("alignedSize = %d\n", alignedSize);
	//printf("mallocSize = %d + 8 = %d = 0x%x\n", alignedSize, mallocSize, mallocSize);

	tmp = free_list_begin();
	//printf("free list begins at address %p\n", tmp);
	if (tmp == NULL){
		returning = (int*)(my_malloc_new(mallocSize));
		//printf("returning address 0x%x\n", returning);
		return (void *) (returning);
	}else{
		freeNode = check(mallocSize);	
		//printf("firstNodeSize = %d\n", firstNodeSize);
		// if there's space left in the original free block to malloc
		if (freeNode != NULL){
			firstNodeSize = *(int*) freeNode;
			resultSize = firstNodeSize - mallocSize;
			if (resultSize >= 8){
				//printf("original free block has space %d to malloc, after mallocing %d, still has %d left, which is greater than 8\n", firstNodeSize, mallocSize, resultSize);
				Bottom = (int*) (freeNode + firstNodeSize);
				//printf("bottom at address %p = %p(freeNode) + %p(firstNodeSize = %d)\n", Bottom, freeNode, firstNodeSize, firstNodeSize);
				Top = (int*)(freeNode + firstNodeSize - mallocSize);
				*(Top) = mallocSize;
				//printf("Top at address %p = %p + %p - %p(mallocSize %d), gave SIZE = %d\n", Top, freeNode, firstNodeSize, mallocSize, mallocSize, mallocSize);

				*(int*) freeNode = resultSize;
				//printf("rest of the free node updated to have size %d at address %p\n", resultSize, tmp);
				//printf("returning address %p + 2 = %p\n", (void*) Top, (void*)(Top+2));
				return (void*)(Top+2);
			}else{
				//printf("orignal free block has enough space to malloc (%d), but not more than 8 left afterward, give rest to user\n", firstNodeSize);
				Top = (int*) (freeNode);
				//*(Top) = firstNodeSize;
				//printf("Top at address %p, gave SIZE = %d\n", Top, firstNodeSize);
				//printf("Returning address %p = %p+2\n", (void*)(Top+2), (void*)Top);

				// remove current memory block from free list
				prev = free_list_prev(freeNode);
				if (prev == NULL){
					malloc_head = NULL;
				}else{
					next = free_list_next(freeNode);
					ftmp = (Flist) prev;
					fnext = (Flist) next;
					ftmp->flink = fnext;
					//printf("removed memory block from free list by resetting malloc_head to %p\n", malloc_head);
				}

				return (void*)(Top+2);
			}
		}
		// no space left in the original free block to malloc
		else{
			returning = (int*)my_malloc_new(mallocSize);
			return (void*) returning;
		}

	}
}

void my_free(void *ptr){
	int freeSize, size; 
	void* freeS;
	Flist tmp;
	Flist f;


	freeSize = *(int*)(ptr-8);
	//printf("freeing size at 0x%x is %d = %d\n", ptr-8, *(int*)(ptr-8), freeSize);

	tmp = find_last_free();
	freeS = (void*) (ptr-8); // not needed
	f = (Flist) (void*) (ptr-8);
	f->flink = NULL;
	if (tmp == NULL){
		malloc_head = (void*)(ptr-8);
		return;
	}
	tmp->flink = f;
}

void *free_list_begin(){
	if (malloc_head == NULL){
		return NULL;
	}else{
		return malloc_head;
	}
}

void *free_list_next(void *node){
	void *n;
	Flist f;
	void *returning;
	int found;

	if (node == NULL){
		fprintf(stderr, "Null node given to free_list_next()\n");
		exit(1);
	}

	if (malloc_head == NULL){
		return NULL;
	}

	n = free_list_begin();
	returning = NULL;
	found = 0;
	while(n != NULL){
		f = (Flist) n;
		if (n == node){
			returning = f->flink;
			found = 1;
			break;
		}
		n = (void*) f->flink;
	}

	if (found){
		return returning;
	}else{
		fprintf(stderr, "free_list_next() didn't find the given node in freeList\n");
		exit(1);
	}

}

int compare(const void* a, const void* b){
	return (*(Flist*)a - *(Flist*)b);
}

void coalesce_free_list(){
	int count;
	void *tmp, *prev, *next;
	Flist ftmp, fnode;
	Flist* freeArray;
	int i, nodeSize;
	int* node;
	Flist fprev, fnext;

	tmp = free_list_begin();
	ftmp = (Flist) tmp;
	if (tmp == NULL || ftmp->flink == NULL){ return;}
	count = 0;
	while(tmp != NULL){
		count++;
		ftmp = (Flist) tmp;
		tmp = (void*) ftmp->flink; 
	}

	freeArray = (Flist*) malloc(sizeof(Flist *)*count);
	
	tmp = free_list_begin();
	ftmp = (Flist) tmp;
	for (i = 0; i < count; i++){
		freeArray[i] = ftmp;
		ftmp = ftmp->flink;
	}
	
	qsort(freeArray, count, sizeof(Flist*), compare);

	for (i = 0; i < count-1; i++){
		freeArray[i]->flink = freeArray[i+1];
	}
	freeArray[count-1]->flink = NULL;
	
	malloc_head = freeArray[0];

	free(freeArray);
	tmp = free_list_begin();
	ftmp = (Flist) tmp;
	
	ftmp = (Flist)free_list_begin();

	while(ftmp != NULL){
		nodeSize = *(int*) tmp;
		node = (int*) (tmp + nodeSize);
		fnode = (Flist) node;
		if ( fnode == ftmp->flink){
			*(int*) tmp = nodeSize + *(int*) node;
			fnext = fnode->flink;
			ftmp->flink = fnext;
		}else{
			ftmp = ftmp->flink;
			tmp = (void*) ftmp;
		}
	}
}

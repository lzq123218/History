#include <stdio.h>
#include <stdlib.h>

#define uint8_t unsigned char
#define PAGE_SIZE_IN_WORD	1024
typedef struct _virus {
	void *data;
	int len;
	void* pFastArray;
	void* newaddr;
}VIRUS; 

typedef struct _index {
	int index;
	int next;
}INDEX;

static VIRUS *virus_db = 0;
static int virus_num = 0;
static int max_len = 0;    // counted in word (four bytes)
static int total_len = 0;  // counted in word (four bytes)
/*VIRUS virus_db[] = {
	{ 1, 1, 0},
	{ 2, 3, 0},
	{3, 4, 0},
	{4, 2, 0}, 
	{5, 5, 0},
	{6, 1, 0},
	{7, 6, 0}
};
int virus_num = 7;*/

static INDEX* index = 0;

static int compare(const void *one, const void *two);
static int get_first(int* start, int len);
static int verify_db();
static int get_largest(int len);
static void pmemcpy(void* dest, void* src, int len);

int load_db(char *fname)
{
	FILE *file;
	int len;
	int i;
	
	file = fopen(fname , "rb");
	if(file == NULL)
	{
		perror("Can't read file\n");
		return -1;
	}
	
	fseek(file, 0, SEEK_END);
	len = ftell(file);
	virus_num = len / sizeof(VIRUS);
	virus_db = (VIRUS*)malloc(len);

	fseek(file, 0, SEEK_SET);
	fread((void*)virus_db, sizeof(VIRUS), virus_num, file);
	
	return 0; 
}

void load_db2( void* addr, unsigned int num ){

	virus_db = addr;
	virus_num = num;
}

void index_db()
{
	int i, start = 0, last_found;
	
	if(virus_db == 0)
		return;

	qsort(virus_db, virus_num, sizeof(VIRUS), compare);
	max_len = virus_db[virus_num - 1].len;
	
	for( i = 0; i < virus_num; i ++)
	  	total_len += virus_db[i].len;
	printf("Total len is %d words\n", total_len);

	index = (INDEX*)malloc( (max_len + 1)*sizeof(INDEX));
	index[0].index = -1;
	index[0].next = -1;
	last_found = -1;
	
	for(i =  1; i <= max_len; i ++)
	{	
		index[i].index = get_first(&start, i);
		index[i].next = last_found;
		
		if(index[i].index != -1)
			last_found = i;
	}
}

/*
 * return: Total length after defrag
 */
int defrag_db(uint8_t* dest)
{
	int i, j = 0;
	int offset = 0;
	int len = max_len;
	
	for(;;)
	{
		if( offset%PAGE_SIZE_IN_WORD == 0 )
			i = get_largest(len);
		else {
			i = get_largest(PAGE_SIZE_IN_WORD - offset%PAGE_SIZE_IN_WORD) ;	
			if( i == -1)
			{
				offset += (PAGE_SIZE_IN_WORD - offset%PAGE_SIZE_IN_WORD);
				i = get_largest(len);
			}
		}
		if( i == -1)
			break;
		
		pmemcpy(&dest[offset<<2], virus_db[i].data, virus_db[i].len<<2);
		j ++;
		virus_db[i].newaddr = &dest[offset<<2];
		offset += virus_db[i].len;
	}
	if( j != virus_num )
		printf("Error happen in defrag: %d, %d\n", j, virus_num);
	return offset;
}

static int compare(const void *one, const void *two)
{
	VIRUS* p1 = (VIRUS*)one, *p2 = (VIRUS*)two;
	
	return p1->len - p2-> len;
}
/*
 *  Looking for the first one with length "len"
 *  Used with index_db.
 */
static int get_first(int* start, int len)
{
	if(*start >= virus_num)
		return -1;
	
	while(virus_db[*start].len < len)
	{
		(*start) ++;
		if( *start == virus_num)
			return -1;
	}

	if(virus_db[*start].len == len)
		return *start;
	else
		return -1;
}

static int verify_db()
{
	int i = max_len, vindex, next_occur = -1;
	
	for(; i > 0; i --)
	{
		vindex = index[i].index;
		if(vindex != -1)
		{
			/* check the length */
			if(virus_db[vindex].len != i)
			{
				printf("Error:%d has wrong length\n", i);
				return -1;
			}
			/* if located in blank area, vindex should be -1 */
			if( next_occur != -1 && i > next_occur )
			{
				printf("Error:%d should be in blank area\n", i); 
			        return -1; 
			}
		} else {
			if( next_occur != -1 && i <= next_occur)
			{
				printf("Error: %d Shouldn't be in blank area\n", i);
				return -1;
			}
		}
		next_occur = index[i].next;
	}
}


static void pmemcpy(void* dest, void* src, int len)
{
//	printf("Copy %d data from %d to %d\n", len, src, dest);
}

#if 0

static void swap(VIRUS* vdb, int i, int j)
{
	void *tmpdata; 
	int tmplen;
	tmpdata = vdb[j].data;
	tmplen = vdb[j].len;
	vdb[j].data = vdb[i].data;
	vdb[j].len = vdb[i].len;
	vdb[i].data = tmpdata;
	vdb[i].len = tmplen;
}

#endif

/*
 * return virus index in virus_db  whose length is the largest but not larger than maxlen, 
 * Note: index will be updated!
 */
static int get_largest(int len)
{
	if( len > max_len )
		len = max_len;
	
	while(len > 0)
	{
		int first_occur = index[len].index;
		int next_len = index[len].next;
		
		if( first_occur != -1)
		{
			if( first_occur == virus_num - 1 )
				index[len].index = -1;
			else {
				if( virus_db[ first_occur + 1 ].len == len )
					index[len].index = first_occur + 1;
				else
					index[len].index = -1;
			}
			return first_occur;
		} else
			len = next_len;
	}
	return -1;
}


unsigned long build( void* addr, unsigned int num ){

	int len; 

	load_db2( addr, num );

	index_db();
	verify_db();
	len = defrag_db( 0 );
	
	return len;
}

#if 0
int main(int argc, char** argv)
{
	int len; 
	load_db("info.a");
	index_db();
	verify_db();
	len = defrag_db( 0 );
	printf("total %d words\n", len);
	return 0;
}
#endif


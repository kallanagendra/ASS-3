#include <time.h>

#define MAX_LINE_LEN 100
#define MAX_LINES 100

struct SharedRegion{
	struct timespec clock;	//holds seconds and nanoseconds
	char mylist[MAX_LINES][MAX_LINE_LEN];	//100 items, each 100 chars long
};

#define SHARED_PATH "/home"
#define SEMAPHORE_KEY 1234
#define MEMORY_KEY 5678

union semun {
	int              val;
	struct semid_ds *buf;
	unsigned short  *array;
	struct seminfo  *__buf;
};

#include <string.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "oss.h"

static int mId=-1, sId = -1;	/* memory and semaphore IDs*/
static struct SharedRegion *mem = NULL;	/* shared memory pointer*/
static int xx;

static int get_mem(){
	/* get keys for our shared objects */
	key_t key1 = ftok(SHARED_PATH, MEMORY_KEY);
	key_t key2 = ftok(SHARED_PATH, SEMAPHORE_KEY);
	if((key1 == -1) || (key2 == -1)){
		perror("ftok");
		return -1;
	}

	/* get shared memory and semaphores */
	mId = shmget(key1, sizeof(struct SharedRegion), 0);
	sId = semget(key2, 2, 0);
  if((mId == -1) || (sId == -1)){
  	perror("semget");
  	return -1;
  }

	/* attach to the shared memory */
	mem = (struct SharedRegion *) shmat(mId, NULL, 0);
	if(mem == NULL){
		perror("shmat");
		return -1;
	}

	return 0;
}

static int get_args(const int argc, char * const argv[]){
	if(argc != 2){
		fprintf(stderr, "Error: xx not found\n");
		return -1;
	}

	/* the xx argument from parent */
	xx = atoi(argv[1]);
	if(xx < 0){
		fprintf(stderr, "Error: xx invalid\n");
		return -1;
	}

	return 0;
}

static int check_string(const char * str){
	int i;
	const int len = strlen(str) - 1; 	//-1 for newline
	const int half = len / 2;

	/* compare characters in the two halfs of string */
	for(i=0; i < half; i++)
		if(str[i] != str[len-i-1])
			return 0;

	return 1;	/* its a palindrome */
}

static int enter_critical(const int seq_num){
	struct sembuf sop;

	fprintf(stderr, "[%li:%li] %d ENTER critical section %d\n", mem->clock.tv_sec, mem->clock.tv_nsec, getpid(), seq_num);

	sop.sem_num = seq_num;	/* seq_num shows which semaphore to use. 0 is for palin.out, 1 is for nopalin.out */
	sop.sem_flg = 0;
	sop.sem_op  = -1;				/* enter is equal to lock*/
	if(semop(sId, &sop, 1) == -1) {
		 perror("semop");
		 return -1;
	}
	fprintf(stderr, "[%li:%li] %d INSIDE critical section %d\n", mem->clock.tv_sec, mem->clock.tv_nsec, getpid(), seq_num);
	return 0;
}

static int leave_critical(const int seq_num){
	struct sembuf sop;

	sleep(2);	/* wait 2 seconds before leaving critical section */

	sop.sem_num = seq_num;
	sop.sem_flg = 0;
	sop.sem_op  = 1;
	if(semop(sId, &sop, 1) == -1) {
		 perror("semop");
		 return -1;
	}
	fprintf(stderr, "[%li:%li] %d LEFT critical section %d\n", mem->clock.tv_sec, mem->clock.tv_nsec, getpid(), seq_num);

	return 0;
}

static int save_result(const char* str, const int palindrome){
	FILE * fp;

	if(palindrome){	/* if string is a palindrome*/
		fp = fopen("palin.out", "a");
	}else{
		fp = fopen("nopalin.out", "a");
	}

	if(fp == NULL){
		perror("fopen");
		return -1;
	}

	sleep(2);	/* sleep before writing string */

	fprintf(fp, "%s", str);
	fclose(fp);

	return 0;
}

int main(const int argc, char * const argv[]){

	if(	(get_args(argc, argv) < 0) ||
			(get_mem() < 0))
		return EXIT_FAILURE;

	srand(time(NULL));

	const char * str = mem->mylist[xx];
	const int palindrome = check_string(str);
	const int seq_num = (palindrome) ? 1 : 0;

	sleep(rand() % 3);

	int err = 0;
	if(	(enter_critical(seq_num) == -1)			  ||
			(save_result(str, palindrome) == -1) ||
			(leave_critical(seq_num) == -1)	){
			err = 1;
	}

	if(err){
		printf("[%li:%li] Pali %d crashed\n", mem->clock.tv_sec, mem->clock.tv_nsec, getpid());
	}else{
		printf("[%li:%li] Pali %d finished\n", mem->clock.tv_sec, mem->clock.tv_nsec, getpid());
	}

	shmdt(mem);

	return EXIT_SUCCESS;
}

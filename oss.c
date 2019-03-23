#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <limits.h>

#include "oss.h"

enum child_stat{
  STARTED=0,      /* started processes*/
  RUNNING,        /* running processes */
  TERMINATED,     /* terminated processes */
  CHILD_STAT      /* how many stat fields we have*/
};

static int max[CHILD_STAT]  = {4,2,0};
static int stats[CHILD_STAT] = {0,0,0};

static int mId=-1, sId = -1;	/* memory and semaphore IDs*/
static struct SharedRegion *mem = NULL;	/* shared memory pointer*/

static int master_loop = 1;     //main loop control

static int stop_children(const int sig){
  if(stats[RUNNING] > 0){ /* if we have started children*/
    if(killpg(getpid(), sig) == -1){
      perror("killpg");
      return -1;
    }else{
        int i, status;
        for(i=0; i < stats[RUNNING]; i++){
          wait(&status);
        }
    }
  }
  return 0;
}

void cleanup(const int code){

  stop_children(SIGTERM);


  printf("[%li:%li] oss done (exit %d)\n", mem->clock.tv_sec, mem->clock.tv_nsec, code);

  /* destroy memory and semaphore */
  if( (shmctl(mId, IPC_RMID, NULL) == -1) ||
      (semctl(sId, 0, IPC_RMID) == -1) ||
      (shmdt(mem) == -1)){
      perror("cleanup");
  }
	exit(code);
}

static int exec_child(const int xx){

	pid_t pid = fork();  //create a process
  if(pid == -1){
    perror("fork");

  }else if(pid == 0){ /* child gets here*/
    char buf[20];
    snprintf(buf, 20, "%i", xx);

    setpgid(getpid(), getppid()); /* set child process group to parent PID, for easier termination */

    execl("palin", "palin", buf, NULL); /* execute pali program */
    perror("execl");
    exit(EXIT_FAILURE);

  }else{  /* parent gets here*/
    stats[STARTED]++;
    stats[RUNNING]++;
    printf("[%li:%li] Child %i with PID=%i STARTED\n", mem->clock.tv_sec, mem->clock.tv_nsec, stats[STARTED], pid);
  }

	return pid;
}

static int get_args(const int argc, char * const args[]){
  char input[PATH_MAX], output[PATH_MAX];

  /* set default filenames */
  strncpy(input, "input.txt", PATH_MAX);
  strncpy(output, "output.txt", PATH_MAX);

  int opt;
	while((opt=getopt(argc, args, "hs:n:o:i:")) != -1){
		switch(opt){
			case 'h':
        printf("Usage: oss -h -n 4 -s 2 -i input.txt -i output.txt\n");
        printf("\t-h\t Show this message\n");
        printf("\t-n x\tMaximum processes to be started\n");
        printf("\t-s x\t Maximum processes running at once, [1;20]\n");
        printf("\t-i filename\tInput strings file\n");
        printf("\t-o filename\tOutput logfile\n");
				return -1;
			case 's':  max[RUNNING]	= atoi(optarg); break;
      case 'n':  max[STARTED]	= atoi(optarg); break;
      case 'o':  strncpy(output, optarg, PATH_MAX); break;
			case 'i':  strncpy(input, optarg, PATH_MAX); break;
			default:
				fprintf(stderr, "Error: Invalid option '%c'\n", opt);
				return -1;
		}
	}

  if( (max[RUNNING] <= 0) ||
      (max[RUNNING] > 20)   ){
    fprintf(stderr, "Error: -s invalid\n");
    return -1;
  }

  stdin = freopen(input, "r", stdin);
  stdout = freopen(output, "w", stdout);
  if((stdin == NULL) || (stdout == NULL)){
    perror("freopen");
    return -1;
  }

  return 0;
}

static int get_mem(){
	/* get keys for our shared objects */
	key_t key1 = ftok(SHARED_PATH, MEMORY_KEY);
	key_t key2 = ftok(SHARED_PATH, SEMAPHORE_KEY);
	if((key1 == -1) || (key2 == -1)){
		perror("ftok");
		return -1;
	}

	/* get shared memory and semaphores */
	mId = shmget(key1, sizeof(struct SharedRegion), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	sId = semget(key2, 2, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
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

  /* initialise the semaphore set to unlocked*/
  unsigned short vals[2] = {1,1};
  union semun un;
  un.array = vals;

	if(semctl(sId, 0, SETALL, un) ==-1){
		perror("semctl");
		return -1;
	}

  bzero(mem, sizeof(struct SharedRegion));

	return 0;
}

//Returns the number of exited children
static void on_child_term(){
  pid_t pid;
  int status;

  //call wait for all finished children
  while((pid = waitpid(-1, &status, WNOHANG)) > 0){
    if (WIFEXITED(status)) {
      printf("[%li:%li] PID %i done (exit %d)\n", mem->clock.tv_sec, mem->clock.tv_nsec, pid, WEXITSTATUS(status));
    }else if(WIFSIGNALED(status)){
      printf("[%li:%li] PID %i killed (signal %d)\n", mem->clock.tv_sec, mem->clock.tv_nsec, pid, WTERMSIG(status));
    }
    stats[RUNNING]--;
    if(++stats[TERMINATED] >= max[STARTED])  /* all that can be started are terminated */
      master_loop = 0;                        /* end master loop */
  }
}

static void sig_handler(const int sig){

  switch(sig){
    case SIGINT:
      printf("[%li:%li] Signal TERM received\n", mem->clock.tv_sec, mem->clock.tv_nsec);
      master_loop = 0;
      break;
    case SIGALRM:
      printf("[%li:%li] Signal ALRM received\n", mem->clock.tv_sec, mem->clock.tv_nsec);
      master_loop = 0;
      break;
    case SIGCHLD:
      on_child_term();
      break;
    default:
      break;
  }
}

/* Copy strings to shared memory*/
static int share_strings(){
  int i = 0;
  while(fgets(mem->mylist[i], MAX_LINE_LEN, stdin) != NULL){
    if(++i > MAX_LINES){
      fprintf(stderr, "Warning: Reached maximum number of lines\n");
      return -1;
    }
  }

  return i;
}

int main(const int argc, char * const argv[]){

  if( (get_args(argc, argv) < 0) ||
      (get_mem() < 0)){
      cleanup(EXIT_FAILURE);
  }

  const int xx_max = share_strings();
  if(xx_max <= 0){
    fprintf(stderr, "Error: No lines in input file\n");
    cleanup(EXIT_FAILURE);
  }

  signal(SIGCHLD, sig_handler);
  signal(SIGINT, sig_handler);
  signal(SIGINT, SIG_IGN);  /* we use killpg, which will send the signal to us too. so just ignore it*/
  signal(SIGALRM, sig_handler);
  alarm(25);

  int xx = 0; //index of current line

	while(master_loop){


    if( (xx < xx_max) &&
        (stats[STARTED] < max[STARTED]) &&
        (stats[RUNNING] < max[RUNNING])  ){
        pid_t pid = exec_child(xx++);
        printf("%d %d %s", pid, stats[STARTED], mem->mylist[xx-1]);
    }

    if( stats[STARTED] == stats[TERMINATED]){
        break;
    }

		/* advance clock */
    mem->clock.tv_nsec += 1000;           /* advance clock with 1k nanoseconds */
  	if(mem->clock.tv_nsec >= 1000000000){
      mem->clock.tv_nsec = 0;
  		mem->clock.tv_sec++;
  	}
	}

	cleanup(EXIT_SUCCESS);
}

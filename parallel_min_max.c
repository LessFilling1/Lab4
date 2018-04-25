#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int active_child_processes = 0;
int pnum;
pid_t *child_arr;

static void sig_alarm(int sigg) {
    printf("Timeout reached! \n");
    for(int i = 0; i < pnum; ++i) {
        kill(child_arr[i], SIGTERM);
    }
    while (active_child_processes >= 0) {
        int wpid = waitpid(-1, NULL, WNOHANG);
        if(wpid == -1){
            if(errno == ECHILD) break;
        } else {
            active_child_processes -= 1;
        }
    }
    printf("Exit! \n");
    exit(0);
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  pnum = -1;
  int timeout = 0;
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {"timeout", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            break;
          case 1:
            array_size = atoi(optarg);
            if(array_size <= 0) { return 1; }
            break;
          case 2:
            pnum = atoi(optarg);
            if(pnum <= 0) { return 1; }
            break;
          case 3:
            with_files = true;
            break;
          case 4: {
            timeout = atoi(optarg);
            if(timeout <= 0) { return 1; }
            break;
          }
          defalut:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;
      case '?':
        break;
      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" \n", argv[0]);
    printf("       You can use --by_files (or -f) to sort it parralel by files \n");
    printf("       You can use --timeout \"num\" to auto-kill programm after timeout");
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  

  struct timeval start_time;
  gettimeofday(&start_time, NULL);
  
  int io_id;
  int pipefd[2];
  if(with_files) {
      io_id = open("tmp.fl", O_RDWR | O_CREAT);
  } else {
      if(pipe(pipefd) < 0) {
          return 1;
      }
  }
  int pecount = array_size/pnum;
  child_arr = malloc(pnum*sizeof(pid_t));
  if(signal(SIGALRM, sig_alarm)) {
      printf("Can't catch SIG_ALARM!\n");
  }
  alarm(timeout);
  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      child_arr[i] = child_pid;
      // successful fork
      active_child_processes += 1;
      if (child_pid == 0) {
        // child process
        
        // parallel somehow
        struct MinMax min_max;
        min_max.min = INT_MAX;
        min_max.max = INT_MIN;
        if(i == pnum-1) {
            min_max = GetMinMax(array, i*pecount, (i+1)*pecount + array_size%pnum);
        } else {
            min_max = GetMinMax(array, i*pecount, (i+1)*pecount); 
        }
        sleep(11);
        if (with_files) {
          pwrite(io_id, &min_max, 2*sizeof(int), 2*sizeof(int)*i);
        } else {
          close(pipefd[0]);
          write(pipefd[1], &min_max , 2*sizeof(int));
        }
        return 0;
      }

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }
  close(pipefd[1]);
  while (active_child_processes >= 0) {
    pid_t wpid = wait(NULL);
    if(wpid == -1) {
        if(errno == ECHILD) break;
    } else{
        active_child_processes -= 1;
    }
  }
  
  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      read(io_id, &min, sizeof(int));
      read(io_id, &max, sizeof(int));
    } else {
      // read from pipes
      read(pipefd[0], &min, sizeof(int));
      read(pipefd[0], &max, sizeof(int));
    }
    printf("min: %d, max: %d\n", min, max);
    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }
  
  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}
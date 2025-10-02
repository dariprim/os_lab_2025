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
#include <getopt.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = -1;  // Таймаут в секундах
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"timeout", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
                printf("seed must be a positive number\n");
                return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
                printf("array_size must be a positive number\n");
                return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
                printf("pnum must be a positive number\n");
                return 1;
            }
            break;
          case 3:
            timeout = atoi(optarg);
            if (timeout <= 0) {
                printf("timeout must be a positive number\n");
                return 1;
            }
            break;
          case 4:
            with_files = true;
            break;

          default:
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
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"] \n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  // Массив для хранения PID дочерних процессов
  pid_t *child_pids = malloc(pnum * sizeof(pid_t));

  // Create pipes for communication (if not using files)
  int (*pipes)[2] = NULL;
  if (!with_files) {
    pipes = malloc(pnum * sizeof(int[2]));
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes[i]) == -1) {
        printf("Pipe creation failed!\n");
        return 1;
      }
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  // Calculate chunk size for each process
  int chunk_size = array_size / pnum;
  int remainder = array_size % pnum;

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      active_child_processes += 1;
      if (child_pid == 0) {
        // child process
        
        // Calculate start and end indices for this chunk
        int start = i * chunk_size;
        int end = start + chunk_size;
        
        // Handle remainder for the last process
        if (i == pnum - 1 && remainder > 0) {
          end += remainder;
        }

        struct MinMax local_min_max = GetMinMax(array, start, end);

        if (with_files) {
          char filename[100];
          sprintf(filename, "minmax_%d.txt", getpid());
          
          FILE *file = fopen(filename, "w");
          if (file != NULL) {
            fprintf(file, "%d\n%d", local_min_max.min, local_min_max.max);
            fclose(file);
          }
        } else {
          // Use pipe for communication
          close(pipes[i][0]); 
          
          write(pipes[i][1], &local_min_max.min, sizeof(int));
          write(pipes[i][1], &local_min_max.max, sizeof(int));
          
          close(pipes[i][1]); 
        }
        
        free(array);
        free(child_pids);
        if (!with_files) free(pipes);
        exit(0);
      } else {
        // Store child PID
        child_pids[i] = child_pid;
      }

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // Parent process - wait for all children to complete with timeout
  if (timeout > 0) {
    // Используем select для ожидания с таймаутом
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    fd_set fds;
    FD_ZERO(&fds);
    
    int max_fd = 0;
    if (!with_files) {
      for (int i = 0; i < pnum; i++) {
        FD_SET(pipes[i][0], &fds);
        if (pipes[i][0] > max_fd) max_fd = pipes[i][0];
      }
    }

    int ready = select(max_fd + 1, &fds, NULL, NULL, &tv);
    
    if (ready == 0) {
      // Таймаут истек - убиваем все дочерние процессы
      printf("Timeout reached (%d seconds). Killing child processes...\n", timeout);
      for (int i = 0; i < pnum; i++) {
        if (kill(child_pids[i], SIGKILL) == 0) {
          printf("Killed child process %d\n", child_pids[i]);
        } else {
          printf("Failed to kill child process %d: %s\n", child_pids[i], strerror(errno));
        }
      }
      
      // Ждем завершения убитых процессов
      while (active_child_processes > 0) {
        wait(NULL);
        active_child_processes -= 1;
      }
      
      printf("Min: N/A (timeout)\n");
      printf("Max: N/A (timeout)\n");
      
      struct timeval finish_time;
      gettimeofday(&finish_time, NULL);
      double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
      elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;
      printf("Elapsed time: %fms\n", elapsed_time);
      
      // Очистка ресурсов
      free(array);
      free(child_pids);
      if (!with_files) free(pipes);
      
      return 0;
    }
  }

  // Ожидаем завершения всех дочерних процессов (обычный случай без таймаута)
  while (active_child_processes > 0) {
    wait(NULL);
    active_child_processes -= 1;
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      // Read from files created by child processes
      char filename[100];
      sprintf(filename, "minmax_%d.txt", child_pids[i]);
      
      FILE *file = fopen(filename, "r");
      if (file != NULL) {
        if (fscanf(file, "%d", &min) != 1) min = INT_MAX;
        if (fscanf(file, "%d", &max) != 1) max = INT_MIN;
        fclose(file);
        // Remove temporary file
        remove(filename);
      } else {
        printf("Warning: Could not open file %s\n", filename);
      }
    } else {
      // Read from pipes
      close(pipes[i][1]); 
      
      if (read(pipes[i][0], &min, sizeof(int)) != sizeof(int)) min = INT_MAX;
      if (read(pipes[i][0], &max, sizeof(int)) != sizeof(int)) max = INT_MIN;
      
      close(pipes[i][0]); 
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  // Free allocated memory
  free(child_pids);
  if (!with_files) {
    free(pipes);
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
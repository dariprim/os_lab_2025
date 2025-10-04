#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

long long g_k = 0;
int g_pnum = 0;
long long g_mod = 0;

long long g_fact_full = 1;   /* полный факториал (может переполниться!) */
long long g_fact_mod = 1;    /* факториал по модулю */
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

struct thread_args {
  long long start;
  long long end;
  int id;
};

void *partial_factorial(void *arg) {
  struct thread_args local_args;
  local_args = *((struct thread_args *)arg);

  long long start = local_args.start;
  long long end = local_args.end;
  int id = local_args.id;
  long long local_full = 1;
  long long local_mod = 1;
  long long i;

  pthread_mutex_lock(&mut);
  printf("doing thread %d\n", id);
  printf("thread %d will compute product of [%lld .. %lld]\n", id, start, end);
  pthread_mutex_unlock(&mut);

  if (start <= end) {
    for (i = start; i <= end; i++) {
      local_full *= i;  /* полный факториал */
      local_mod = (local_mod * i) % g_mod; /* факториал по модулю */
    }
  }

  pthread_mutex_lock(&mut);
  g_fact_full *= local_full;
  g_fact_mod = (g_fact_mod * local_mod) % g_mod;
  printf("thread %d done: local_full = %lld, local_mod = %lld, combined_mod = %lld\n",
         id, local_full, local_mod, g_fact_mod);
  pthread_mutex_unlock(&mut);

  return NULL;
}

int main(int argc, char **argv) {
  int i;
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
      g_k = atoll(argv[i + 1]);
      i++;
    } else if (strncmp(argv[i], "--pnum=", 7) == 0) {
      g_pnum = atoi(argv[i] + 7);
    } else if (strncmp(argv[i], "--mod=", 6) == 0) {
      g_mod = atoll(argv[i] + 6);
    }
  }

  if (g_k < 0 || g_pnum <= 0 || g_mod <= 0) {
    fprintf(stderr, "Usage: %s -k <num> --pnum=<threads> --mod=<mod>\n", argv[0]);
    exit(1);
  }

  if (g_k == 0) {
    printf("1 mod %lld = %lld\n", g_mod, (long long)(1 % g_mod));
    return 0;
  }

  pthread_t *threads = malloc(sizeof(pthread_t) * g_pnum);
  struct thread_args *args = malloc(sizeof(struct thread_args) * g_pnum);

  long long chunk = g_k / g_pnum;
  long long rem = g_k % g_pnum;
  long long cur = 1;

  for (i = 0; i < g_pnum; i++) {
    long long start = cur;
    long long end = cur + chunk - 1;
    if (i < rem) end++;
    if (start > g_k) { start = 1; end = 0; }

    args[i].start = start;
    args[i].end = end;
    args[i].id = i + 1;

    if (pthread_create(&threads[i], NULL, partial_factorial, (void *)&args[i]) != 0) {
      perror("pthread_create");
      exit(1);
    }

    cur = end + 1;
  }

  for (i = 0; i < g_pnum; i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      perror("pthread_join");
      exit(1);
    }
  }

  printf("%lld mod %lld = %lld\n", g_fact_full, g_mod, g_fact_mod);

  free(threads);
  free(args);
  return 0;
}

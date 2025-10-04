#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h> /* sleep */

pthread_mutex_t mutex_a = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_b = PTHREAD_MUTEX_INITIALIZER;

void *thread_func_one(void *arg) {
  (void)arg;
  printf("Thread 1: trying to lock mutex_a\n");
  pthread_mutex_lock(&mutex_a);
  printf("Thread 1: locked mutex_a\n");

  /* задержка, чтобы увеличить вероятность взаимной блокировки */
  sleep(1);

  printf("Thread 1: trying to lock mutex_b\n");
  pthread_mutex_lock(&mutex_b);
  /* Ниже код недостижим в случае deadlock */
  printf("Thread 1: locked mutex_b\n");

  /* если бы оба мьютекса были захвачены, мы бы выполнили работу */
  printf("Thread 1: doing work with both mutexes\n");

  pthread_mutex_unlock(&mutex_b);
  pthread_mutex_unlock(&mutex_a);

  return NULL;
}

void *thread_func_two(void *arg) {
  (void)arg;
  printf("Thread 2: trying to lock mutex_b\n");
  pthread_mutex_lock(&mutex_b);
  printf("Thread 2: locked mutex_b\n");

  /* задержка, чтобы увеличить вероятность взаимной блокировки */
  sleep(1);

  printf("Thread 2: trying to lock mutex_a\n");
  pthread_mutex_lock(&mutex_a);
  /* Ниже код недостижим в случае deadlock */
  printf("Thread 2: locked mutex_a\n");

  printf("Thread 2: doing work with both mutexes\n");

  pthread_mutex_unlock(&mutex_a);
  pthread_mutex_unlock(&mutex_b);

  return NULL;
}

int main() {
  pthread_t th1, th2;
  int rc;

  rc = pthread_create(&th1, NULL, thread_func_one, NULL);
  if (rc != 0) {
    perror("pthread_create thread1");
    exit(1);
  }

  rc = pthread_create(&th2, NULL, thread_func_two, NULL);
  if (rc != 0) {
    perror("pthread_create thread2");
    exit(1);
  }

  /* main ждёт завершения потоков — но в случае deadlock
     ожидание будет вечным */
  pthread_join(th1, NULL);
  pthread_join(th2, NULL);

  printf("Main: threads finished, program will exit\n");
  return 0;
}

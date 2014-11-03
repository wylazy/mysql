#include <stdio.h>
#include <pthread.h>
#include <os0sync.h>

unsigned long long count = 0;

extern unsigned long srv_max_n_threads;
os_ib_mutex_t g_mutex;

void * func(void * arg) {
  int i = 0;
  int n = *(int *)arg;
  for (i = 0; i < n; i++) {
    os_mutex_enter(g_mutex);
    count = count+1;
    os_mutex_exit(g_mutex);
  }
  return NULL;
}

int main() {

  int i = 0, n = 1000000;
  int n_threads = 8;
  srv_max_n_threads = n_threads;

  pthread_t ts[n_threads];

  os_sync_init();
  g_mutex = os_mutex_create();


  for (i = 0; i< n_threads; i++) {
    if (pthread_create(&ts[i], NULL, func, &n)) {
      printf("create thread %d error\n", i);
      exit(1);
    }
  }

  for (i = 0; i< n_threads; i++) {
    pthread_join(ts[i], NULL);
  }

  printf("count = %llu\n", count);

  os_mutex_free(g_mutex);
  return 0;
}

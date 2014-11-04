#include <stdio.h>
#include <pthread.h>
#include <os0sync.h>
#include <sync0sync.h>

//sync_init will use srv_max_n_threads for init
extern unsigned long srv_max_n_threads;
unsigned long long count = 0;

ib_mutex_t g_mutex;

void * func(void * arg) {
  int i = 0;
  int n = *(int *)arg;
  for (i = 0; i < n; i++) {
    mutex_enter(&g_mutex);
    count = count+1;
    mutex_exit(&g_mutex);
  }
  return NULL;
}

int main() {

  int i = 0, n = 1000000;
  int n_threads = 4;
  srv_max_n_threads = n_threads;

  pthread_t ts[n_threads];

  os_sync_init(); //for mutex_free will lock os_sync_mutex in file sync0sync.cc:550

  sync_init(); //init mutex and mutex array for use

  //mutex_create() will use mutex_list_mutex
  UT_LIST_INIT(mutex_list);
  mutex_create(n, &mutex_list_mutex, SYNC_NO_ORDER_CHECK);

  mutex_create(n, &g_mutex, SYNC_NO_ORDER_CHECK);

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

  mutex_free(&g_mutex);
  return 0;
}

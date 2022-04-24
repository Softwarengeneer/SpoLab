#include "producer_consumer.h"
#include <pthread.h>
#include <atomic>
#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <queue>

using namespace std;
int N;
int max_sleep_time;
bool isDebugMode;
static bool isTasksSupplied(false);
static int waitingThreadsCount(0);
pthread_cond_t consumer_cond, producer_cond;
pthread_mutex_t mutex;
int get_tid() {
  static atomic_int lastThreadId(0);
  thread_local unique_ptr<int> id = make_unique<int>(-1);

  if (*id == -1) {
    *id = ++lastThreadId;
  }

  return *id;
}

struct ConsumerStruct {
  int sum = 0;
  queue<int> *taskQueue{};
};

void *producer_routine(void *arg) {
  queue<int> *taskQueue = ((queue<int> *)arg);
  int sum = 0;
  int number;
  while (!isTasksSupplied) {
    pthread_mutex_lock(&mutex);
    if (!taskQueue->empty()) {
      pthread_cond_wait(&producer_cond, &mutex);
    }
    if (cin >> number) {
      sum += number;
      taskQueue->push(number);
      pthread_cond_signal(&consumer_cond);
    } else {
      isTasksSupplied = true;
    }
    pthread_mutex_unlock(&mutex);
  }
  while (true) {
    pthread_mutex_lock(&mutex);
    if (!taskQueue->empty() || waitingThreadsCount != 0) {
      pthread_cond_signal(&consumer_cond);
    }
    if (taskQueue->empty() && waitingThreadsCount == 0) {
      pthread_mutex_unlock(&mutex);
      break;
    }

    pthread_mutex_unlock(&mutex);
  }
  return nullptr;
}

void *consumer_routine(void *arg) {
  ConsumerStruct *threadStruct = (ConsumerStruct *)arg;
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);

  while (true) {
    bool isDidTask = false;
    pthread_mutex_lock(&mutex);
    if (threadStruct->taskQueue->empty() && !isTasksSupplied) {
      pthread_cond_signal(&producer_cond);
      waitingThreadsCount++;
      pthread_cond_wait(&consumer_cond, &mutex);
      waitingThreadsCount--;
    }
    if (!threadStruct->taskQueue->empty()) {
      int number = threadStruct->taskQueue->front();
      threadStruct->sum += number;
      threadStruct->taskQueue->pop();
      isDidTask = true;
      if (isDebugMode) {
        cout << "(" << get_tid() << ", " << threadStruct->sum << ")" << endl;
      }
    }
    if (isTasksSupplied && threadStruct->taskQueue->empty()) {
      pthread_mutex_unlock(&mutex);
      break;
    }
    pthread_mutex_unlock(&mutex);
    if (isDidTask && max_sleep_time != 0) {
      int sleepTime = rand() % max_sleep_time + 1;
      struct timespec ts = {0, sleepTime * 1'000'000};
      struct timespec tr {};
      nanosleep(&ts, &tr);
    }
  }
  return nullptr;
}

void *consumer_interruptor_routine(void *arg) {
  vector<pthread_t> *p_thread = (vector<pthread_t> *)arg;

  while (!isTasksSupplied) {
    pthread_cancel(p_thread->at(rand() % N));
  }

  return nullptr;
}

int run_threads() {
  queue<int> taskQueue;
  pthread_t producer_thread;
  pthread_t interrupted_thread;
  vector<ConsumerStruct> threadStruct(N);
  vector<pthread_t> consumer_thread(N);
  int sum = 0;
  pthread_mutex_init(&mutex, nullptr);
  pthread_cond_init(&producer_cond, nullptr);
  pthread_cond_init(&consumer_cond, nullptr);
  pthread_create(&producer_thread, nullptr, producer_routine,
                 (void *)&taskQueue);
  for (int i = 0; i < N; i++) {
    threadStruct[i] = ConsumerStruct{0, &taskQueue};
    pthread_create(&consumer_thread[i], nullptr, consumer_routine,
                   (void *)&(threadStruct[i]));
  }
  pthread_create(&interrupted_thread, nullptr, consumer_interruptor_routine,
                 (void *)&consumer_thread);
  pthread_join(interrupted_thread, nullptr);
  for (int i = 0; i < N; i++) {
    pthread_join(consumer_thread[i], nullptr);
    sum += threadStruct[i].sum;
  }
  pthread_join(producer_thread, nullptr);
  return sum;
}
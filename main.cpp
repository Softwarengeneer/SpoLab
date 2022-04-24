#include <pthread.h>
#include <atomic>
#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <queue>
#include "producer_consumer.h"

using namespace std;

int main(int argc, char** argv) {
  switch (argc) {
    case 4: {
      isDebugMode = true;
      N = stoi(string(argv[2]));
      max_sleep_time = stoi(string(argv[3]));
      break;
    }
    case 3: {
      isDebugMode = false;
      N = stoi(string(argv[1]));
      max_sleep_time = stoi(string(argv[2]));
      break;
    }
    default: {
      return 1;
    }
  }

  cout << run_threads() << endl;
  return 0;
}
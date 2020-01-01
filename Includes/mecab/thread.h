#ifndef _MECAB_THREAD_H_
#define _MECAB_THREAD_H_

#include <pthread.h>

namespace MeCab {

class thread {
 private:
  pthread_t hnd;

 public:
  static void* wrapper(void* ptr) {
    thread* p = static_cast<thread*>(ptr);
    p->run();
    return 0;
  }

  virtual void run() {}

  void start() { pthread_create(&hnd, 0, &thread::wrapper, static_cast<void*>(this)); }

  void join() { pthread_join(hnd, 0); }

  virtual ~thread() {}
};
}  // namespace MeCab

#endif  // _MECAB_THREAD_H_

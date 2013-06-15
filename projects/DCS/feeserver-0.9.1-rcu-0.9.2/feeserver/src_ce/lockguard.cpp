#include "lockguard.hpp"
#include <pthread.h>
#include <cerrno>
#include "ce_base.h"

CE_Mutex::CE_Mutex()
  :
  fpMutex(NULL),
  fpMutexAttr(NULL)
{
  // postpone initialization to first use or external trigger
}

CE_Mutex::~CE_Mutex()
{
  if (fpMutex) {
    pthread_mutex_destroy((pthread_mutex_t*)fpMutex);
    delete (pthread_mutex_t*)fpMutex;
  }
  if (fpMutexAttr) {
    delete (pthread_mutexattr_t*)fpMutex;
  }
}

int CE_Mutex::Init()
{
  if (fpMutex==NULL) {
    fpMutex=new pthread_mutex_t;
    if (fpMutex) {
      fpMutexAttr=new pthread_mutexattr_t;
      if (fpMutexAttr) {
	pthread_mutexattr_settype((pthread_mutexattr_t*)fpMutexAttr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init((pthread_mutex_t*)fpMutex, (pthread_mutexattr_t*)fpMutexAttr);
      } else {
	delete (pthread_mutex_t*)fpMutex;
      }
    }
  }
  if (fpMutex) return 0;
  return -ENOMEM;
}

int CE_Mutex::Lock()
{
  if (fpMutex==NULL && Init()<=0) return -EFAULT;

  if (fpMutex)
    return pthread_mutex_lock((pthread_mutex_t*)fpMutex);

  return -EFAULT;
}

int CE_Mutex::Unlock()
{
  if (fpMutex)
    return pthread_mutex_unlock((pthread_mutex_t*)fpMutex);

  return -EFAULT;
}

 
CE_LockGuard::CE_LockGuard(CE_Mutex& m) 
  : fMutex(m)
{
  fMutex.Lock();
}

CE_LockGuard::~CE_LockGuard() 
{
  fMutex.Unlock();
}


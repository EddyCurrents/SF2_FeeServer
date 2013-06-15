#ifndef __LOCKGUARD_HPP
#define __LOCKGUARD_HPP

class CE_Mutex
{
public:
  CE_Mutex();
  ~CE_Mutex();

  int Init();

  int Lock();
  int Unlock();

private:
  void* fpMutex;
  void* fpMutexAttr;
};

class CE_LockGuard
{
public:
  CE_LockGuard(CE_Mutex& m);
  ~CE_LockGuard();

private:
  CE_Mutex& fMutex;
};

#endif //__LOCKGUARD_HPP

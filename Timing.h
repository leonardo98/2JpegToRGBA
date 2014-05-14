#include <Windows.h>

class Timing {
public:
  void StartTiming();
  void StopTiming();
  double GetUserSeconds() const {
    return double(m_userTime) / 10000000.0;
  }
private:
  __int64 GetUserTime() const;
  __int64 m_userTime;
};

__int64 Timing::GetUserTime() const {
  FILETIME creationTime;
  FILETIME exitTime;
  FILETIME kernelTime;
  FILETIME userTime;
  GetThreadTimes(GetCurrentThread(),
                 &creationTime, &exitTime,
                 &kernelTime, &userTime);
  __int64 curTime;
  curTime = userTime.dwHighDateTime;
  curTime <<= 32;
  curTime += userTime.dwLowDateTime;
  return curTime;
}

void Timing::StartTiming() {
  m_userTime = GetUserTime();
}

void Timing::StopTiming() {
  __int64 curUserTime = GetUserTime();
  m_userTime = curUserTime - m_userTime;
}
#ifndef SERVICE_HPP
#define SERVICE_HPP

#include <string>
#include "logger.hpp"

#define NAME_LEN 80

class MTConnectService {
public:
  MTConnectService();
  virtual int main(int aArgc, const char *aArgv[]);
  virtual void initialize(int aArgc, const char *aArgv[]) = 0;

  void setName(const std::string &aName) { mName = aName; }
  virtual void stop() = 0;
  virtual void start() = 0;
  const std::string &name() { return mName; }
  
protected:
  std::string mName;
  std::string mConfigFile;
  bool mIsService;
  
  void install();

};

#ifdef WIN32
class ServiceLogger : public Logger {
public:
  virtual void error(const char *aFormat, ...);
  virtual void warning(const char *aFormat, ...);
  virtual void info(const char *aFormat, ...);  

protected:
};
#else
class ServiceLogger : public Logger {};
#endif
#endif
#ifndef __LOG_H__
#define __LOG_H__
#include <windows.h>

#include <sstream>
#include <time.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <assert.h>

//#define USE_HIRES_TIMER

enum TLogLevel {logERROR, logWARNING, logINFO, logDEBUG};

inline std::string NowTime();

class Log
{
public:
   Log();
   std::ostringstream& Get(TLogLevel level = logINFO, const char* functionName="", const char* file="", long line = -1);
   std::string Content();
public:
   static Log& Instance();
   static TLogLevel& ReportingLevel();
   static std::string ToString(TLogLevel level);
   static void Flush();

protected:
   std::ostringstream os;
private:
   Log(const Log&);
   Log& operator =(const Log&);
private:
   TLogLevel messageLevel;
};

static Log* loggerInstance = 0;
inline Log::Log()/*: os(std::ios::app)*/
{
}

Log& Log::Instance()
{
    if (!loggerInstance)
    {
        loggerInstance = new Log();
    }
   
    return *loggerInstance;
}

std::ostringstream& Log::Get(TLogLevel level, const char* functionName,  const char* file, long line)
{
   os << std::endl;
   os << "- " << NowTime() << " ";
   os << functionName << " " << file << " " << line << "thread id:" << ::GetCurrentThreadId() ;
   os << " " << ToString(level) << ": ";
   os << std::string(level > logDEBUG ? 0 : logDEBUG - level, '\t');
   messageLevel = level;
   return os;
}

inline TLogLevel& Log::ReportingLevel()
{
    static TLogLevel reportingLevel = logDEBUG;
    return reportingLevel;
}

inline std::string Log::ToString(TLogLevel level)
{
    static const char* const buffer[] = {"ERROR", "WARNING", "INFO", "DEBUG"};
    return buffer[level];
}

inline std::string Log::Content()
{
    return os.str();
}

struct replacefunctor
{
    void operator()(char& c) { if(c == ':') c = '.'; }
};

inline void Log::Flush()
{
    // write content to a file
    std::string fileName = std::string("log-") + NowTime() + std::string(".txt");
    for_each( fileName.begin(), fileName.end(), replacefunctor() );
    std::ofstream out(fileName.c_str());
    // log level is taken into account when logging, here we just want to flush
    // everything to disk
    out << Log::Instance().Get(logDEBUG).str() << "Flushing ended";
    // just testing delete after test
    std::string str = Log::Instance().Get(logDEBUG).str();
}


inline std::string NowTime()
{
#ifdef USE_HIRES_TIMER
    static LARGE_INTEGER freq;
    static bool res = QueryPerformanceFrequency(&freq);

    if (!res)
    {
        assert(false);
    }
#endif

    const int MAX_LEN = 200;
    char buffer[MAX_LEN];
    if (::GetTimeFormatA(LOCALE_USER_DEFAULT, 0, 0,
            "HH':'mm':'ss", buffer, MAX_LEN) == 0)
        return "Error in NowTime()";

#ifndef USE_HIRES_TIMER
    //low resolution counter (15 msec tipical)
    char result[100] = {0};
    static DWORD first = ::GetTickCount();
    std::sprintf(result, "%s.%03ld", buffer, (long)(GetTickCount() - first) % 1000);
#else
    //hi res counter
    char result[100] = {0};
    static LARGE_INTEGER firstHiRes = {0,0};
    if (!firstHiRes.QuadPart) // not init yet
    {
        ::QueryPerformanceCounter(&firstHiRes);
    }

    LARGE_INTEGER currentHiRes = {0,0};
    ::QueryPerformanceCounter(&currentHiRes);

    double ellapsedHiRes =  (double) ((currentHiRes.QuadPart - firstHiRes.QuadPart) / (double) freq.QuadPart);
    //std::sprintf(result, "%s.%.3lf", buffer, (ellapsedHiRes - (long) ellapsedHiRes));
    std::sprintf(result, "%s.%03ld", buffer, (long) ((ellapsedHiRes - (long) ellapsedHiRes)*1000));
#endif
    return std::string(result);
}

//logger macros
typedef Log MemoryLog;

// logg level
#define LOG(level) \
    if (level > MemoryLog::ReportingLevel()) ; \
    else Log::Instance().Get(level, __FUNCTION__, __FILE__, __LINE__)

// log debug
#define LOG_DEBUG \
    LOG(logDEBUG)

// log info
#define LOG_INFO \
    LOG(logINFO)

// log flush to file
#define LOG_FLUSH \
    Log::Instance().Flush();

#endif
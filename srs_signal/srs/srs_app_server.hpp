#ifndef SRS_APP_SERVER_HPP
#define SRS_APP_SERVER_HPP

#include <vector>
#include <string>
typedef int srs_error_t;
#define srs_success 0

// 系统信号量宏定义
// The signal defines.
// To reload the config file and apply new config.
#define SRS_SIGNAL_RELOAD SIGHUP
// Reopen the log file.
#define SRS_SIGNAL_REOPEN_LOG SIGUSR1
// For gracefully upgrade, start new SRS and gracefully quit old one.
// @see https://github.com/ossrs/srs/issues/1579
// TODO: Not implemented.
#define SRS_SIGNAL_UPGRADE SIGUSR2
// The signal for srs to fast quit, do essential dispose then exit.
#define SRS_SIGNAL_FAST_QUIT SIGTERM
// The signal for srs to gracefully quit, do carefully dispose then exit.
// @see https://github.com/ossrs/srs/issues/1579
#define SRS_SIGNAL_GRACEFULLY_QUIT SIGQUIT
// The signal for SRS to abort by assert(false).
#define SRS_SIGNAL_ASSERT_ABORT SIGABRT

// The application level signals.
// Persistence the config in memory to config file.
// @see https://github.com/ossrs/srs/issues/319#issuecomment-134993922
// @remark we actually don't handle the signal for it's not a valid os signal.
#define SRS_SIGNAL_PERSISTENCE_CONFIG 1000


// Convert signal to io,
// @see: st-1.9/docs/notes.html
class SrsSignalManager /*: public ISrsCoroutineHandler*/
{
private:
    // Per-process pipe which is used as a signal queue.
    // Up to PIPE_BUF/sizeof(int) signals can be queued up.
    int sig_pipe[2];//无名管道，fd[0]负责读，fd[1]负责写
    /*srs_netfd_t*/int signal_read_stfd;
private:
//    SrsServer* server;
//    SrsCoroutine* trd;
public:
    SrsSignalManager(/*SrsServer* s*/);
    virtual ~SrsSignalManager();
public:
    virtual srs_error_t initialize();
    virtual srs_error_t start();//监听信号，开启线程轮询
// Interface ISrsEndlessThreadHandler.
public:
    static void* startThread(void* obj);//SRS是在协程轮询读信号，这里改为传统线程轮询，用法类似
    virtual void *cycle();//轮询读取管道
    void on_signal(int signo);
private:
    // Global singleton instance
    static SrsSignalManager* instance;
    // Signal catching function.
    // Converts signal event to I/O event.
    static void sig_catcher(int signo);//信号捕获回调函数
};


#endif


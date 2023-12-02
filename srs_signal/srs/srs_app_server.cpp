#include <srs_app_server.hpp>

#include <sys/types.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <assert.h>
#if !defined(SRS_OSX) && !defined(SRS_CYGWIN64)
#include <sys/inotify.h>
#endif
using namespace std;


#ifdef SRS_RTC
#include <srs_app_rtc_network.hpp>
#endif
#ifdef SRS_GB28181
#include <srs_app_gb28181.hpp>
#endif

SrsSignalManager* SrsSignalManager::instance = NULL;

SrsSignalManager::SrsSignalManager()
{
    SrsSignalManager::instance = this;
    
//    server = s;
    sig_pipe[0] = sig_pipe[1] = -1;
//    trd = new SrsSTCoroutine("signal", this, _srs_context->get_id());
//    signal_read_stfd = NULL;
}

SrsSignalManager::~SrsSignalManager()
{
//    srs_freep(trd);

//    srs_close_stfd(signal_read_stfd);
    
    if (sig_pipe[0] > 0) {
        ::close(sig_pipe[0]);
    }
    if (sig_pipe[1] > 0) {
        ::close(sig_pipe[1]);
    }
}

srs_error_t SrsSignalManager::initialize()
{
    /* Create signal pipe */
    //创建管道
    if (pipe(sig_pipe) < 0) {
        printf( "create pipe,ERROR_SYSTEM_CREATE_PIPE\n");
        return -1;
    }

//    if ((signal_read_stfd = srs_netfd_open(sig_pipe[0])) == NULL) {
//        return srs_error_new(ERROR_SYSTEM_CREATE_PIPE, "open pipe");
//    }
    
    return srs_success;
}

srs_error_t SrsSignalManager::start()
{
    srs_error_t err = srs_success;
    
    /**
     * Note that if multiple processes are used (see below),
     * the signal pipe should be initialized after the fork(2) call
     * so that each process has its own private pipe.
     */
    struct sigaction sa;
    
    //sigemptyset：将信号集置空
    //sigaction：为信号指定相关的处理程序
    /* Install sig_catcher() as a signal handler */
    sa.sa_handler = SrsSignalManager::sig_catcher;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SRS_SIGNAL_RELOAD, &sa, NULL);
    
    sa.sa_handler = SrsSignalManager::sig_catcher;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SRS_SIGNAL_FAST_QUIT, &sa, NULL);

    sa.sa_handler = SrsSignalManager::sig_catcher;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SRS_SIGNAL_GRACEFULLY_QUIT, &sa, NULL);

    sa.sa_handler = SrsSignalManager::sig_catcher;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SRS_SIGNAL_ASSERT_ABORT, &sa, NULL);
    
    sa.sa_handler = SrsSignalManager::sig_catcher;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    
    sa.sa_handler = SrsSignalManager::sig_catcher;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SRS_SIGNAL_REOPEN_LOG, &sa, NULL);
    
    printf("signal installed, reload=%d, reopen=%d, fast_quit=%d, grace_quit=%d\n",
              SRS_SIGNAL_RELOAD, SRS_SIGNAL_REOPEN_LOG, SRS_SIGNAL_FAST_QUIT, SRS_SIGNAL_GRACEFULLY_QUIT);
    
//    if ((err = trd->start()) != srs_success) {//执行协程处理函数
//        return srs_error_wrap(err, "signal manager");
//    }
    pthread_t tid;
    pthread_create(&tid,NULL,SrsSignalManager::startThread,this);
    
    return err;
}

void *SrsSignalManager::startThread(void *obj)
{
    SrsSignalManager* pSrsSignalManager = (SrsSignalManager*)obj;
    pSrsSignalManager->cycle();

    return nullptr;
}

void* SrsSignalManager::cycle()
{  
    while (true) {
        int signo;
        
        /* Read the next signal from the pipe */
        //从管道读取数据
        /*srs_read*/read(sig_pipe[0], &signo, sizeof(int)/*, SRS_UTIME_NO_TIMEOUT*/);
        
        /* Process signal synchronously */
        on_signal(signo);
        usleep(1000);
    }
    
    return nullptr;
}

void SrsSignalManager::sig_catcher(int signo)
{
    int err;
    
    /* Save errno to restore it after the write() */
    err = errno;
    
    /* write() is reentrant/async-safe */
    int fd = SrsSignalManager::instance->sig_pipe[1];//往管道里写数据
    write(fd, &signo, sizeof(int));
    
    errno = err;
}


void SrsSignalManager::on_signal(int signo)
{
    // For signal to quit with coredump.
    if (signo == SRS_SIGNAL_ASSERT_ABORT) {
        printf("abort with coredump, signo=%d\n", signo);
        assert(false);
        return;
    }

    if (signo == SRS_SIGNAL_RELOAD) {
        printf("reload config, signo=%d\n", signo);
        return;
    }

#ifndef SRS_GPERF_MC
    if (signo == SRS_SIGNAL_REOPEN_LOG) {
        printf("reopen log file, signo=%d\n", signo);
        return;
    }
#endif

#ifdef SRS_GPERF_MC
    if (signo == SRS_SIGNAL_REOPEN_LOG) {
        signal_gmc_stop = true;
        srs_warn("for gmc, the SIGUSR1 used as SIGINT, signo=%d", signo);
        return;
    }
#endif

    if (signo == SRS_SIGNAL_PERSISTENCE_CONFIG) {
        return;
    }

    if (signo == SIGINT) {
#ifdef SRS_GPERF_MC
        srs_trace("gmc is on, main cycle will terminate normally, signo=%d", signo);
        signal_gmc_stop = true;
#endif
    }

    // For K8S, force to gracefully quit for gray release or canary.
    // @see https://github.com/ossrs/srs/issues/1595#issuecomment-587473037
    if (signo == SRS_SIGNAL_FAST_QUIT ) {
        printf("force gracefully quit, signo=%d\n", signo);
        signo = SRS_SIGNAL_GRACEFULLY_QUIT;
    }

    if ((signo == SIGINT || signo == SRS_SIGNAL_FAST_QUIT) ) {
        printf("sig=%d, user terminate program, fast quit\n", signo);
        return;
    }

    if (signo == SRS_SIGNAL_GRACEFULLY_QUIT ) {
        printf("sig=%d, user start gracefully quit\n年", signo);
        return;
    }
}

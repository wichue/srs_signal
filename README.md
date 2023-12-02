C++音视频流媒体服务器SRS源码框架解读，信号量(SrsSignalManager)的使用.

SRS信号量的使用

SRS封装了SrsSignalManager类，注册信号量回调函数，使用linux无名管道，接收到信号量时写入管道，在协程里轮询读取管道里的信号，并作相关处理。

信号量的使用提供了用户进程（命令行即可）与正在运行的SRS程序通信的方式，SRS使用信号量实现的功能：强制退出程序、优雅地退出程序（执行一系列析构，停止监听等）、重新加载配置文件、日志文件切割等。

SRS使用的是协程轮询，这里使用传统线程轮询，用法类似。

源码测试

程序运行后，在命令行执行：

killall -s SIGUSR1 srs_Signal		#srs_Signal是进程名


srs_Signal进程打印

signal installed, reload=1, reopen=10, fast_quit=15, grace_quit=3

reopen log file, signo=10			#发送SIGUSR1 信号时打印

force gracefully quit, signo=15		#Ctrl+c 强制退出时打印

sig=3, user start gracefully quit	#Ctrl+c 强制退出时打印



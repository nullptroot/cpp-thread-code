#include <chrono>
#include <iostream>
#include <condition_variable>
#include <mutex>
std::condition_variable cv;

bool done;

std::mutex m;
/*下述函数 条件遍历等待特定的时间后，没有被唤醒，就超时*/
bool wait_loop()
{
    auto const timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    std::unique_lock<std::mutex> lk(m);
    while(!done)
    {
        if(cv.wait_until(lk,timeout) == std::cv_status::timeout)
            break;
    }
    return done;
}
int main()
{
    /*这里显示了 时间点类的加减*/
    /*公用一个时钟才能做加减*/
    auto start = std::chrono::high_resolution_clock::now();
    int a = 10000;
    while(--a);
    auto end = std::chrono::high_resolution_clock::now();
    std::cout<<"while(10000) took "<<(end-start).count()<<" senconds"<<std::endl;

    /*下面可以获取 当前的时间 其中1970是时钟纪元的开始*/
    auto tp = std::chrono::high_resolution_clock::now();
    auto tm = tp.time_since_epoch();
    /*可以自定义时间长度，来强转  自己可以实现  寻找日期的操作  抽时间实现一下*/
    using day = std::chrono::duration<int64_t, std::ratio<24*3600L>>;
    using year = std::chrono::duration<int64_t, std::ratio<365*24*3600L>>;
    std::cout<<std::chrono::duration_cast<year>(tm).count() + 1970 <<std::endl;
    std::cout<<std::chrono::duration_cast<day>(tm).count() / (365) + 1970 <<std::endl;
    std::cout<<std::chrono::duration_cast<std::chrono::hours>(tm).count() / (24*365) + 1970 <<std::endl;
}
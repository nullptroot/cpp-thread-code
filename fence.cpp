#include <atomic>
#include <thread>
#include <assert.h>
std::atomic<bool> x,y;
std::atomic<int> z;
void write_x_then_y()
{
    /*栅栏放这里就不行了 在这里的话 这一线程x和y是有先行关系的
    其他线程就没了*/
    x.store(true,std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);
    y.store(true,std::memory_order_relaxed);
}

void read_x_then_y()
{
    while(!y.load(std::memory_order_relaxed));
    std::atomic_thread_fence(std::memory_order_acquire);
    if(x.load(std::memory_order_relaxed))
        ++z;
}
/*内存栅栏，
规则：
    这个存储操作没有规定顺序  但后面的获取操作时获取次序
    若存储操作在释放序列栅栏后面，而存储操作的结果为获取操作所见，那么释放栅栏与获取操作同步
    这个载入操作没有规定顺序  但前面的释放操作时释放顺序
    若载入操作处于获取栅栏前面，且载入操作看见了释放操作的结果，那么获取栅栏和释放操作同步
    
    
    下面两个栅栏是同步的  一个线程里有先行关系*/
int  main()
{
    x = false;
    y = false;
    z = 0;
    std::thread a(write_x_then_y);
    std::thread b(read_x_then_y);
    a.join();
    b.join();
    assert(z.load() != 0);
    return 0;
}
/*=============================================*/

#include <atomic>
#include <thread>
#include <assert.h>
bool x = false;
std::atomic<bool> y;
std::atomic<int> z;
void write_x_then_y()
{
    x = true;
    std::atomic_thread_fence(std::memory_order_release);
    y.store(true,std::memory_order_relaxed);
}

void read_x_then_y()
{
    while(!y.load(std::memory_order_relaxed));
    std::atomic_thread_fence(std::memory_order_acquire);
    if(x)
        ++z;
}
/*内存栅栏，
规则：
    这个存储操作没有规定顺序  但后面的获取操作时获取次序
    若存储操作在释放序列栅栏后面，而存储操作的结果为获取操作所见，那么释放栅栏与获取操作同步
    这个载入操作没有规定顺序  但前面的释放操作时释放顺序
    若载入操作处于获取栅栏前面，且载入操作看见了释放操作的结果，那么获取栅栏和释放操作同步
    
    
    这里两个栅栏是同步的，因此强制了x的内存次序，也就实现了非原子操作服从内存次序
    前面的release consume也可以实现非原子操作内存次序*/
int  main()
{
    x = false;
    y = false;
    z = 0;
    std::thread a(write_x_then_y);
    std::thread b(read_x_then_y);
    a.join();
    b.join();
    assert(z.load() != 0);
    return 0;
}
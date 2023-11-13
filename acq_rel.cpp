#include <atomic>
#include <thread>
#include <assert.h>


std::atomic<int> data[5];
std::atomic<int> sync(0);
/*这里三个线程一块进行 也是不会触发断言的
thread_2中有一个读改写操作，使用的是acq_rel，这个内存
次序与前面的release写同步，与后面的acquire同步
存储的acquire不会和任何操作同步，载入的release不会和任何操作同步*/
void thread_1()
{
    data[0].store(42,std::memory_order_relaxed);
    data[1].store(97,std::memory_order_relaxed);
    data[2].store(17,std::memory_order_relaxed);
    data[3].store(-141,std::memory_order_relaxed);
    data[4].store(2003,std::memory_order_relaxed);    

    sync.store(1,std::memory_order_release);
}
void thread_2()
{
    int exp = 1;
    while(!sync.compare_exchange_strong(exp,2,std::memory_order_acq_rel))
        exp = 1;
}
void thread_3()
{
    while(sync.load(std::memory_order_acquire));
    assert(data[0].load(std::memory_order_relaxed) == 43);
    assert(data[1].load(std::memory_order_relaxed) == 97);
    assert(data[2].load(std::memory_order_relaxed) == 17);
    assert(data[3].load(std::memory_order_relaxed) == -141);
    assert(data[4].load(std::memory_order_relaxed) == 2003);  
}
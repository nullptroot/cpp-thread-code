#include <atomic>
#include <thread>
#include <assert.h>
std::atomic<bool> x,y;
std::atomic<int> z;

void write_x()
{
    x.store(true,std::memory_order_release);
}

void write_y()
{
    y.store(true,std::memory_order_release);
}

void read_x_then_y()
{
    
    while(!x.load(std::memory_order_acquire));
    if(y.load(std::memory_order_acquire))
        ++z;
}
void read_y_then_x()
{
    while(!y.load(std::memory_order_acquire));
    if(x.load(std::memory_order_acquire))
        ++z;
}
/*
这里采用获取释放内存次序仍然会发生断言
这是四个线程 线程a的写出对线程c的while会同步
线程b的写出对线程d的while同步，
但是c线程并不一定在y处就能读到y的写出，因为xy的写是两个线程
d也是一样的，因为是两个原子变量，一般获取释放和先行关系一块使用
*/
int main()
{
    x = false;
    y = false;
    z = 0;
    std::thread a(write_x);
    std::thread b(write_y);
    std::thread c(read_x_then_y);
    std::thread d(read_y_then_x);
    a.join();
    b.join();
    c.join();
    d.join();
    assert(z.load() != 0);
}
/*=========================*/
#include <atomic>
#include <thread>
#include <assert.h>
std::atomic<bool> x,y;
std::atomic<int> z;
void write_x_then_y()
{
    x.store(true,std::memory_order_relaxed);
    y.store(true,std::memory_order_release);
}

void read_x_then_y()
{
    while(!y.load(std::memory_order_acquire));
    if(x.load(std::memory_order_relaxed))
        ++z;
}
/*这样就不会发生断言了，因为y的写和读对应两个线程，因此当读线程
读到写线程的写的时候就会发生同步现象，而x的写先行与y的写，y的读先行与
x的读，因此x的读先行与x的写，断言必不会发生

有一个重点，就是只有获取的值是写入的值，才会发生同步关系，因此如果b线程不用
while循环，y读到false，x就可能也读到false

这里同一变量的内存次序，不用非得一对出现，此时y的读取和存储可以设置宽松次序，那么断言就可能
发生了，因为这样*/
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

/*==============================*/


/*下面三个线程 不会触发断言得发生，
因为syncl 先行与data得赋值 同步于thread_2得获取操作，
syncl获取先行与syncl2的存储操作
然后syncl2同步于thread_3的获取操作，
且thread_3 syncl2获取操作先行与data的读取
因此data的存储先行与data的获取，断言不会触发*/
std::atomic<int> data[5];
std::atomic<bool> syncl(false),syncl2(false);

void thread_1()
{
    data[0].store(42,std::memory_order_relaxed);
    data[1].store(97,std::memory_order_relaxed);
    data[2].store(17,std::memory_order_relaxed);
    data[3].store(-141,std::memory_order_relaxed);
    data[4].store(2003,std::memory_order_relaxed);    

    syncl.store(true,std::memory_order_release);
}
void thread_2()
{
    while(!syncl.load(std::memory_order_acquire));
    syncl2.store(true,std::memory_order_release);
}
void thread_3()
{
    while(syncl2.load(std::memory_order_acquire));
    assert(data[0].load(std::memory_order_relaxed) == 43);
    assert(data[1].load(std::memory_order_relaxed) == 97);
    assert(data[2].load(std::memory_order_relaxed) == 17);
    assert(data[3].load(std::memory_order_relaxed) == -141);
    assert(data[4].load(std::memory_order_relaxed) == 2003);  
}
#include <atomic>
#include <thread>
#include <assert.h>
std::atomic<bool> x,y;
std::atomic<int> z;
void write_x_then_y()
{
    x.store(true,std::memory_order_relaxed);
    y.store(true,std::memory_order_relaxed);
}

void read_x_then_y()
{
    while(!y.load(std::memory_order_relaxed));
    if(x.load(std::memory_order_relaxed))
        ++z;
}
/*这次断言就可能会触发了，因为是宽松次序，那么既是在线程a中x的存储先行与y
但是在线程b中不一定，因为这个存储操作只是写到了缓存，这种内存次序应该是不会
通知线程b所在cpu核心缓存块缓存的内存失效，所以b读缓存读到的内容，就不一定
是按照什么顺序了，所以是有可能线程b缓存的y已经更新了，但是x还没有更新
此时断言就发生了*/
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
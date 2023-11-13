#include <atomic>
#include <thread>
#include <assert.h>
#include <string>
/*下述两个线程同时运行，前两个断言是不会触发的
但是第三个可能会触发，因为consume只对其有数据依赖的
数据进行同步，没有数据依赖的是没有同步关系的
因此第三个断言可能会触发*/
struct X
{
    int i;
    std::string s;
};
std::atomic<X*> p;
std::atomic<int> a;

void create_x()
{
     X *x = new X;
     x->i = 42;
     x->s = "hello";
     a.store(99,std::memory_order_relaxed);
     p.store(x,std::memory_order_release);
}

void use_x()
{
    X *x;
    while(!(x = p.load(std::memory_order_consume)))
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    assert(x->i == 42);
    assert(x->s == "hello");
    assert(a.load(std::memory_order_relaxed) == 99);  
}


#include <future>
#include <assert.h>
int main()
{
    /*shared_future 可以copy，future只能转移
    future如果由多个线程调用，需要不同线程之间
    转移归属权，只有一个能得到结果，属于独占性
    ，也就是只有一个线程能访问，shared_future
    可以copy，因此多个线程可以有多个副本，可以
    分别得到结果，多个线程可以有线程独立版本
    不会有数据竞争的存在*/
    std::promise<int> p;
    std::future<int> f(p.get_future());
    /*下面f是有效的*/
    assert(f.valid());
    std::shared_future<int> sf(std::move(f));
    /*f无效了*/
    assert(!f.valid());
    /*sf开始生效*/
    assert(sf.valid());

    std::promise<std::string> w;
    // std::shared_future<std::string> sw(w.get_future());
    /*得到共享future*/
    auto sw = w.get_future().share();
}
#define __cplusplus  201907L
#include <thread>
#include <vector>
#include <future>
#include <iostream>
#include <algorithm>
#include <latch>
// #include "latch.h"

/*根据要等待线程的数量来初始化线程闩latch，
然后根据线程数量用async发起相同数量的线程
并在开启的线程中调用latch::count_down，
线程闩内部计数减一，当减完之后，后面的
latch::wait就不阻塞了，就返回了，这里不用
等待所有线程都结束，因为当每个线程把数据
准备好后，后续的操作就可以执行了，然后线程
还有些后续工作。

不可重复使用哦

当下述代码done.wait()返回时数据就准备好了
就可以执行下面的for_each了

这里lambda表达式，处理i其他都是引用，因为如果引用i
会发生数据竞争的*/
void foo()
{
    unsigned const thread_count = 16;
    std::latch done(thread_count);
    int data[thread_count];
    std::vector<std::future<void>> threads;
    for(unsigned i = 0;i < thread_count; ++i)
    {
        threads.push_back(std::async(std::launch::async,[&,i]
        {
            data[i] = i;
            done.count_down();
            /*do some other thing*/
        }));
    }
    done.wait();
    std::for_each(data,data+thread_count,[](auto x){std::cout<<x<<std::endl;});
}
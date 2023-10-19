#include <thread>
#include <iostream>
#include <unistd.h>
#include <exception>
#include <algorithm>
#include <vector>
#include<numeric>
using std::cout;
using std::endl;
using std::thread;
template <typename Iterator,typename T>
struct accumulate_block
{
    void operator()(Iterator first,Iterator last,T &result)
    {
        result = std::accumulate(first,last,result);
    }
};
template <typename Iterator,typename T>
T parrall_accumulate(Iterator first,Iterator last,T init)
{
    unsigned long const length = std::distance(first,last);
    if(!length)
        return init;
    unsigned long const min_per_thread = 25;//每个线程最低处理这么多个
    unsigned long const max_thread = (length + min_per_thread-1)/min_per_thread;//就是最大的所需线程数
    unsigned long const hardware_thread = thread::hardware_concurrency();//获取硬件所能支持的最大线程数
    unsigned long const num_threads = std::min(hardware_thread != 0 ? hardware_thread : 2,max_thread);//计算最终的thread数
    unsigned long const block_size = length / num_threads;//每个线程处理多少个元素
    std::vector<T> results(num_threads);//存储结果的，每个线程一个
    std::vector<thread> threads(num_threads-1);//有个当前主线程  vector 满足移动复制
    Iterator block_start = first;
    for(unsigned long i = 0; i < (num_threads - 1); ++i)
    {
        Iterator block_end = block_start;
        std::advance(block_end,block_size);//把block_end设置到当前block的最后一个元素
        /*开始执行线程*/
        threads[i] = std::thread(accumulate_block<Iterator,T>(),block_start,block_end,std::ref(results[i]));
        block_start = block_end;//当前的block_end变为下一个block_start都是左闭右开
    }
    accumulate_block<Iterator,T>()(block_start,last,results[num_threads-1]);//当前mian线程 计算最后一个block所有元素
    for(auto &entry : threads)
        entry.join();//线程会和
    return std::accumulate(results.begin(),results.end(),init);//计算result的和
}

int main()
{
    std::vector<int> help;
    for(int i = 0; i < 101; ++i)
        help.push_back(i);
    cout<<parrall_accumulate(help.begin(),help.end(),0)<<endl;

    /*thread t t.get_id() 获取t所管理线程的线程id，  std::this_thread::get_id()获取当前线程的id号*/


    return 0;
}
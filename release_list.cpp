#include <atomic>
#include <thread>
#include <vector>
std::vector<int> queue_data;
std::atomic<int> count;
void populate_queue()
{
    unsigned const number_of_items = 20;
    queue_data.clear();
    for(unsigned i = 0; i < number_of_items; ++i)
    {
        queue_data.push_back(i);
    }
    count.store(number_of_items,std::memory_order_release);
}
void consume_queue_items()
{
    while(true)
    {
        int item_index;
        if((item_index = count.fetch_sub(1,std::memory_order_acquire)) <= 0)
        {
            wait_queue_data();
            continue;
        }
        process(queue_data[item_index-1]);
    }
}
/*这里有个关键点，就是释放序列规则，先说一下规则嗷
（应该是第一个存储）store操作是release acq_rel sqe_cst  load操作是consume acquire seq_cst
这些操作前后相互扣成链，每次载入操作都原子前面的存储操作（这里存储不只是第一个存储）
而且最后的载入付出acquire sqe_cst那么最初的store与最后一次load同步
若是consume则是数据依赖  然后第一次存储和最后一次载入之间的线程与最后一次载入存在先行


比如本程序，如果没有释放序列规则，fetch_sub使用acquire的话，操作时读改写，写并不是release
那么就不会和后一个线程构成先行关系，会出现问题的，因此有了释放序列就不会有问题了

这里的释放序列应该是以一个原子变量的release操作起头，后续的所有针对此原子变量的acquire
操作都于release操作同步，acquire和acquire之间没啥关系，全靠原子变量的原子性保持正确性*/
int main()
{
    std::thread a(populate_queue);
    std::thread b(consume_queue_items);
    std::thread c(consume_queue_items);
    a.join();
    b.join();
    c.join();
}
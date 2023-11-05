#include <future>
#include <vector>
#include "../future/experimental/future"
using FinalResult = int;
using myData = double;
using ChunkResult = float;
/*这个代码是关于when_any的，
之前的代码是when_all的，when_all等待多个future
全部就绪了才返回，具体实现是产生一个新的future，
将传入的多个future包装在内

而when_any是只要有一个就绪，那么就准备就绪，
实现有点深奥 返回值是when_any_result有一个
future集合和就绪索引（when_all返回一个future
集合了所有要等的future），具体详情看下述代码
 没看懂，可以看书p117页*/


 /*when_all when_any总结
 
 when_all返回的future持有它等待的所有future的元组（tuple）
 when_any返回when_any_result包含future和就绪index

他们还有重载形式，接受可变参数，但都是按值传递，因此需要显示
移动std::move*/
template<typename Func>
/*declval 返回一个类型的右值引用*/
std::experimental::future<decltype(std::declval<Func>()())>
spawn_async(Func &&func)
{
    std::experimental::promise<decltype(std::declval<Func>()())> p;
    auto res = p.get_future();
    std::thread t([p=std::move(p),f = std::decay_t<Func>(func)] () mutable
    {
        try
        {
            p.set_value_at_thread_exit(f());
        }
        catch (...)
        {
            p.set_exception_at_thread_exit(std::current_exception());
        }
    });
    t.detach();
    return res;
}

std::experimental::future<FinalResult>
    find_and_process_value(std::vector<myData> &data)
{
    /*获取硬件线程数量*/
    unsigned const concurrency = std::thread::hardware_concurrency();
    /*根据线程数量平均划分任务*/
    unsigned const num_tasks = (concurrency > 0) ? concurrency : 2;
    std::vector<std::experimental::future<myData*>> results;
    /*计算每个任务要处理的数据大小*/
    auto const chunk_size = (data.size() + num_tasks - 1)/num_tasks;/*每个线程平均处理的数据*/

    auto chunk_begin = data.begin();
    std::shared_ptr<std::atomic<bool>> done_flag = 
        std::make_shared<std::atomic<bool>>(false);
    for(unsigned i = 0; i < num_tasks; ++i)
    {
        auto chunk_end = (i < (num_tasks - 1)) ? chunk_begin + chunk_size : data.end();
        /*开始执行每个任务，这里有原子变量*/
        results.push_back(spawn_async([=]{
            for(auto entry = chunk_begin; !*done_flag && (entry != chunk_end); ++entry)
            {
                /*书上是下面的代码*/
                if(*entry)
                // if(matches_find_criteria(*entry))
                {
                    *done_flag = true;
                    return &*entry;
                }
            }
            return (myData *)nullptr;
        }));
        chunk_begin = chunk_end;
    }
    std::shared_ptr<std::experimental::promise<FinalResult>> final_result = 
                                        std::make_shared<std::experimental::promise<FinalResult>>();
    /*可调用类  后面递归调用*/
    struct DoneCheck
    {
        std::shared_ptr<std::experimental::promise<FinalResult>> final_result;
        DoneCheck(std::shared_ptr<std::experimental::promise<FinalResult>> final_result_):
                    final_result(std::move(final_result_)){};
        void operator()(std::experimental::future<std::experimental::when_any_result<
            std::vector<std::experimental::future<myData *>>>> result_param)
        {
            auto results = results_param.get();
            myData *const ready_result = results.futures[results.index].get();
            if(ready_result)
                final_result->set_value(*ready_result);
            else
            {
                results.futures.erase(results.begin()+results.index);
                /*这里递归调用的原因是，只要有一个就绪when_any就返回了，交给后续函数
                执行了，而future就绪不一定是找到结果，还有可能是确定区间没有结果，因此
                需要递归的调用的*/
                if(!results.future.empty())
                    std::experimental::when_any(results.futures.begin(),results.futures.end()).
                    then(std::move(*this));
                else
                    final_result->set_exception(std::make_exception_ptr(std::runtime_error("Not found")));
            }
            
        };
    }
    /*从这里开始执行的，result里面有任何一个就绪，就会触发when_any
    返回的类型是std::experimental::when_any_result<std::vector<std::experimental::future<myData *>>>
    
    然后传入了后续函数then，DoneChech来进行调用
    DoneCheck内部获取when_any_result的结果，看是否满足
    条件，满足条件就设置DoneCheck的promise成员。
    不满足就看看还有没有要等的future，继续when_any等
    后续函数递归实验*this
    如果不满足future又为空，那设置promise是异常*/

    std::experimental::when_any(results.begin(),results.end()).
        then(DoneCheck(final_result));
    return final_result->get_future();
}
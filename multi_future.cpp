#include <future>
#include <vector>
#include "../future/experimental/future"
#define whatever 8
using FinalResult = int;
using myData = double;
using ChunkResult = float;

/*这个文件主要介绍的是实验性库的when_all*/
/*下面的函数就是对vec数组里的元素分段的进行异步处理
然后再触发一个线程异步的收集结果。

但是会出现在v.push_back(f.get()); 这里
反复的唤醒休眠。*/
struct process_chunk
{
    public:
        ChunkResult operator()(std::vector<myData>::iterator begin,std::vector<myData>::iterator end)
        {
            ChunkResult a = 0;
            return a;
        }
};
std::future<FinalResult> process_data(std::vector<myData> &vec)
{
    size_t const chunk_size = whatever;
    std::vector<std::future<ChunkResult>> results;
    for(auto begin = vec.begin(),end = vec.end();begin != end;)
    {
        size_t const remaining_size = end-begin;
        size_t const this_chunk_size = std::min(remaining_size,chunk_size);
        results.push_back(std::async(process_chunk(),begin,begin+this_chunk_size));
        begin += this_chunk_size;
    }
    /*不加mutable就有问题了 lambda自动生成的闭包 operator的
    参数类型是const 会自动是const类型 调用get就会改变东西 条款32*/
    return std::async([all_result = std::move(results)]() mutable 
    {
        std::vector<ChunkResult> v;
        v.reserve(all_result.size());
        for(auto &f:all_result)
            v.push_back(f.get());
        return 10;
    });
}
/*std::experimental::when_all() 等待所有future就绪
然后调用后续函数then  这时候所有的future都就绪了
再调用f.get()就不阻塞了*/
std::experimental::future<FinalResult> process_data1(std::vector<myData> &vec)
{
    size_t const chunk_size = whatever;
    std::vector<std::experimental::future<ChunkResult>> results;
    for(auto begin = vec.begin(),end = vec.end();begin != end;)
    {
        size_t const remaining_size = end-begin;
        size_t const this_chunk_size = std::min(remaining_size,chunk_size);
        /*下面spaw_async会产生std::experimental::future<ChunkResult>对象*/
        results.push_back(spaw_async(process_chunk(),begin,begin+this_chunk_size));
        begin += this_chunk_size;
    }
    return std::experimental::when_all(
        results.begin(),results.end()
    ).then(
        [](std::future<std::vector<std::experimental::future<ChunkResult>>> ready_result)
        {
            std::vector<std::experimental::future<ChunkResult>> all_result = ready_result.get();
            std::vector<ChunkResult> v;
            v.reserve(all_result.size());
            for(auto &f : all_result)
                v.push_back(f.get());
            return gather_result(v);
        }
    );
}
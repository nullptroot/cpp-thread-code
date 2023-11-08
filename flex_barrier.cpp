#define __cplusplus  201907L
#include <barrier>
#include <vector>
#include <thread>
#include <future>
using std::thread;

using result_chunk = int;
using data_chunk = float;
using data_block = double;
using data_source = bool;
using data_sink = unsigned;
// using joining_thread = ;
using result_block = long long;
class joining_thread
{
    private:
        thread t;
    public:
        joining_thread() noexcept = default;
        template<typename Callable,typename ... Args>
        explicit joining_thread(Callable &&func,Args && ... args):t(std::forward<Callable>(func),std::forward<Args>(args)...){}
        explicit joining_thread(thread t_) noexcept :t(std::move(t_)){}
        joining_thread(joining_thread &&other) noexcept:t(std::move(other.t)){};
        joining_thread &operator=(joining_thread &&other) noexcept
        {
            if(joinable())
                join();
            /*类成员函数可以访问任何同类对象的私有成员
            因为其他类调用成员函数和本类调用的成员函数
            调用的是一个*/
            t = std::move(other.t);
            return *this;

        }
        joining_thread &operator=(thread other) noexcept
        {
            if(joinable())
                join();
            t = std::move(other);
            return *this;

        }
        ~joining_thread()
        {
            t.join();
        }
        void swap(joining_thread &other) noexcept
        {
            t.swap(other.t);
        }
        thread::id get_id() const noexcept{return t.get_id();};
        void join(){t.join();};
        void detach(){t.detach();};
        bool joinable() const noexcept
        {
            return t.joinable();
        }
        thread & as_thread() noexcept
        {
            return t;
        }
        const thread & as_thread() const noexcept
        {
            return t;
        }
};
std::vector<data_chunk> divide_into_chunks(data_block data,unsigned num_threads);

/*下面的函数和之前写的barrier代码里的功能是一样的，这里使用了flex_barrier类
这个类可以设置一个后续函数，等所有线程到达后执行一次后续函数，然后在释放所有线程

因此下面的代码就是这样做的，把分割数据的函数分离出来，和向sink中写入数据组成
flex_barrier的后续函数，先分割一下数据，然后所有线程开始运行，到flex_barrier
后阻塞，等所有的线程都到后，开始运行后续函数，写入sink，分割数据，然后释放
所有线程 下一次循环

flex_barrier的后续函数的返回值加上本次阻塞的线程数既是下一次阻塞的线程数*/
void process_data(data_source &source,data_sink &sink)
{
    unsigned const concurrency = std::thread::hardware_concurrency();
    unsigned const num_threads = (concurrency > 0) ? concurrency : 2;

    std::vector<data_chunk> chunks;
    auto split_source = [&]
    {
        if(!source.done())
        {
            data_block current_block = source.get_next_data_block();
            chunks = divide_into_chunks(current_block,num_threads);
        }
    };
    split_source();
    result_block result;
    std::flex_barrier sync(num_threads,[&]
    {
        sink.write_data(std::move(result));
        split_source();
        return -1;
    });

    std::vector<joining_thread> threads(num_threads);

    for(unsigned i = 0; i < num_threads; ++i)
    {
        threads[i] = joining_thread([&,i]
        {
            while(!source.done())
            {
                result.set_chunk(i,num_threads,process(chunks[i]));
                sync.arrive_and_wait();
            }
        });
    }
}
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

/*这里是线程卡的使用方法，这里barrier sync(num_threads)会阻塞
num_threads个线程，下面代码主要流程就是
首先开始创建num_threads个线程去执行下面的lambda函数
仅有第一个线程会对原始的数据进行读取、分割成多个块
供后续的线程分别处理啊，因此，需要在等第一个线程分割完之前
其他线程需要阻塞等待，也就可以调用arrive_and_wait()函数来
等待了，只有当所有线程都调用了arrive_and_wait()，就会释放
然后barrier会重置，供下一次使用，（仅有第一个线程最后调用
因为其他线程会直接调用了，第一个线程需要处理一下数据），
然后各个线程处理子块，然后再调用arrive_and_wait()来
让第一个线程把多个线程的处理结果一块写入

arrive_and_drop()可以脱离同步组，调用之后下一个周期就不会等待
线程卡就绪了，而且线程卡的同步组线程数会减少*/
std::vector<data_chunk> divide_into_chunks(data_block data,unsigned num_threads);

void process_data(data_source &source,data_sink &sink)
{
    unsigned const concurrency = std::thread::hardware_concurrency();
    unsigned const num_threads = (concurrency > 0) ? concurrency : 2;

    std::barrier sync(num_threads);
    std::vector<joining_thread> threads(num_threads);

    std::vector<data_chunk> chunks;

    result_block result;
    for(unsigned i = 0; i < num_threads; ++i)
    {
        threads[i] = joining_thread([&,i]
        {
            while(!source.done())
            {
                if(!i)
                {
                    data_block current_block = source.get_next_data_block();
                    chunks = divide_into_chunks(current_block,num_threads);
                }
                sync.arrive_and_wait();
                result.set_chunk(i,num_threads,process(chunks[i]));
                sync.arrive_and_wait();
                if(!i)
                    sink.write_data(std::move(result));
            }
        })
    }
}
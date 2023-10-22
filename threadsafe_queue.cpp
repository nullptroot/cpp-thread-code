#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <iostream>
/*条件变量的使用，感觉可以替换项目中的信号量，这里条件变量使用好简洁
这里有一个伪唤醒的概念，就是不是被其他线程唤醒的，是自己去检查了条件是否满足
下面是有所线程安全队列*/

/*之前认为会出现当push发出notify的时候，没有线程正在wait，发完notify后再wait
那么就会导致队列里面有元素但是不会取出，会wait等待阻塞，其实不是这样的
在调用wait后，会先判断一次条件是否满足，满足了就直接返回了，不满足就
解锁等待*/
template<typename T>
class threadsafe_queue
{
    private:
        mutable std::mutex mut;
        std::queue<T> data_queue;
        std::condition_variable data_cond;
    public:
        threadsafe_queue(){};
        threadsafe_queue(const threadsafe_queue &other)
        {
            std::lock_guard<std::mutex> lk(other.mut);
            data_queue = other.data_queue;
        }
        threadsafe_queue &operator=(const threadsafe_queue &) = delete;
        void push(T new_value)
        {
            std::lock_guard<std::mutex> lk(mut);
            data_queue.push(new_value);
            data_cond.notify_one();
        }
        bool try_pop(T &value)
        {
            std::lock_guard<std::mutex> lk(mut);
            if(data_queue.empty())
                return false;
            value = data_queue.front();
            data_queue.pop();
            return true;
        }
        std::shared_ptr<T> try_pop()
        {
            std::lock_guard<std::mutex> lk(mut);
            if(data_queue.empty())
                return std::shared_ptr<T>();
            std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
            data_queue.pop();
            return res;
        }
        void wait_and_pop(T &value)
        {
            std::unique_lock<std::mutex> lk(mut);
            data_cond.wait(lk,[this]{std::cout<<"wait call func"<<std::endl;return !data_queue.empty();});
            value = data_queue.front();
            data_queue.pop();
        }
        std::shared_ptr<T> wait_and_pop()
        {
            std::unique_lock<std::mutex> lk(mut);
            data_cond.wait(lk,[this]{return !data_queue.empty();});
            std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
            data_queue.pop();
            return res;
        }
        bool empty() const
        {
            std::lock_guard<std::mutex> lk(mut);
            return data_queue.empty();
        }
};

int main()
{
    threadsafe_queue<int> test;
    // std::cout<<"push"<<std::endl;
    // test.push(10);
    // sleep(2);
    int a0;
    std::cout<<"pop"<<std::endl;
    test.wait_and_pop(a0);
    std::cout<<a0<<std::endl;
    std::cout<<"finished"<<std::endl;
    return 0;
}
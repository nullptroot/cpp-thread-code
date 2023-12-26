#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <iostream>
/*这里采用存储shared_ptr的方式来实现安全队列，避免了pop里出现异常
解决了1.0版的条件唤醒，然后wait_pop出现异常的情况，但是我感觉用户
自定义的copy赋值会出现异常，还是会有这种情况出现*/
template<typename T>
class threadsafe_queue
{
    private:
        mutable std::mutex mut;
        std::queue<std::shared_ptr<T>> data_queue;
        std::condition_variable data_cond;
    public:
        threadsafe_queue(){};
        threadsafe_queue &operator=(const threadsafe_queue &) = delete;
        void push(T new_value)
        {
            /*这里内存分配可以脱离锁的保护  提高性能*/
            std::shared_ptr<T> data(std::make_shared<T>(std::move(new_value)));
            std::lock_guard<std::mutex> lk(mut);
            data_queue.push(data);
            data_cond.notify_one();
        }
        bool try_pop(T &value)
        {
            std::lock_guard<std::mutex> lk(mut);
            if(data_queue.empty())
                return false;
            value = std::move(*data_queue.front());
            data_queue.pop();
            return true;
        }
        std::shared_ptr<T> try_pop()
        {
            std::lock_guard<std::mutex> lk(mut);
            if(data_queue.empty())
                return std::shared_ptr<T>();
            std::shared_ptr<T> res = data_queue.front();
            data_queue.pop();
            return res;
        }
        void wait_and_pop(T &value)
        {
            std::unique_lock<std::mutex> lk(mut);
            data_cond.wait(lk,[this]{return !data_queue.empty();});
            value = std::move(*data_queue.front());
            data_queue.pop();
        }
        std::shared_ptr<T> wait_and_pop()
        {
            std::unique_lock<std::mutex> lk(mut);
            data_cond.wait(lk,[this]{return !data_queue.empty();});
            std::shared_ptr<T> res = data_queue.front();
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
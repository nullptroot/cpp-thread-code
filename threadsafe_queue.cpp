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

/*这里是线程安全的，但不是异常安全的，由于push操作时notify_one，当空的时候
有一次notify_one，仅有一个线程会pop，那么当pop时出现异常(make_shared )，那么
就会导致有任务了，但是没有线程去处理。  这里有三种解决方案
1、改用notify_all 性能受到影响
2、有异常抛出，在pop里捕获在调用一次notify_one
3、shared_ptr的初始化移动到push，也就是令队列存储shared_ptr,而不是数值，赋值shared_ptr不会异常

后面会修改此队列*/
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
            /*这里使用了notify_one，仅有一个等待线程会被唤醒，但是如果被唤醒的线程
            在wait_pop出现异常，那么就没有线程处理任务了，这里有三种解决方法
            1、使用notify_all这样会唤醒所有线程，开销较大
            2、在wait_pop捕获异常，在发出notify_one
            3、由于可能出现异常的原因是wait_pop里的make_shared和用户自定义的copy赋值函数
                我们可以把make_shared提前到push，也就是另容器存储shared_ptr  看2.0版本*/
            data_cond.notify_one();
        }
        /*这里pop即使空了也不会抛出异常，和前面的safe_stack对比仅这一点不一样*/
        bool try_pop(T &value)
        {
            std::lock_guard<std::mutex> lk(mut);
            if(data_queue.empty())
                return false;
            value = data_queue.front();
            data_queue.pop();
            return true;
        }
        /*这里pop即使空了也不会抛出异常，和前面的safe_stack对比仅这一点不一样*/
        std::shared_ptr<T> try_pop()
        {
            std::lock_guard<std::mutex> lk(mut);
            if(data_queue.empty())
                return std::shared_ptr<T>();
            std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
            data_queue.pop();
            return res;
        }
        /*这里的wait_pop和前面的push使用了条件变量，前面的safe_stack等待弹出，需要不断
        调用empty判断，才能弹出，这个不用了，直接使用条件变量就行，而且条件变量在阻塞的
        时候会释放锁，唤醒时在持有锁，增加了并发度，*/
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
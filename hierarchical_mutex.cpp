#include <thread>
#include <mutex>
#include <stdexcept>
#include <exception>
#include<climits>
/*层级锁实现
这里就是每个线程都有属于自己的线程专属变量的值 刚开始时是最大的值，因此对所有层级锁
的对象都能上锁，只要对其中一个层级锁获取，那么线程专属变量的值就会设置成获取锁的层级
值，并且当前锁的一个变量保存了线程专属变量的值，当进行解锁时就恢复线程专属变量的值
可以保证加后续的锁，当再加另一个锁时，会先用线程专属变量的值对要获取锁的层级值进行
比较 如果大于就可以加 不大于就抛异常*/
class hierarchical_mutex
{
private:
    /* data */
    std::mutex internal_mutex;
    unsigned long const hierarchy_value;/*这个值 每个锁对象都有一个 且不能变 按这个值来层级划分的*/
    unsigned long previous_hierarchy_value;//记录 当前线程加本锁前 线程专属变量的值 解锁时释放
    /*线程专属变量 每个线程都能访问，但是值是不一样的*/
    static thread_local unsigned long this_thread_hierarchy_value;
    /*判断是否能加锁 不能加锁就抛出异常*/
    void check_for_hierarchy_violation()
    {
        if(this_thread_hierarchy_value <= hierarchy_value)
            throw std::logic_error("mutex hierarchy violated");//在#include <stdexcept>里
    }
    /*对一个层级锁获取时，更新当前线程的专属变量*/
    void update_hierarchy_value()
    {
        previous_hierarchy_value = this_thread_hierarchy_value;
        this_thread_hierarchy_value = hierarchy_value;
    }

public:
    explicit hierarchical_mutex(unsigned long value):hierarchy_value(value),previous_hierarchy_value(0){};
    void lock()
    {
        check_for_hierarchy_violation();//加锁前 先判断是否能加 不能加直接抛出异常了
        internal_mutex.lock();
        update_hierarchy_value();//更新值 previous_hierarchy_value 保存线程专属变量的值 并把线程转世变量设置为hierarchy_value
    }
    /*为了避免乱序解锁导致层级混乱 这里只能按照加锁的逆向进行解锁*/
    void unlock()
    {
        if(this_thread_hierarchy_value != hierarchy_value)
            throw std::logic_error("mutex hierarchy violated");
        this_thread_hierarchy_value = previous_hierarchy_value;//解锁后还原线程专属变量的值
        internal_mutex.unlock();
    }
    bool try_lock()
    {
        check_for_hierarchy_violation();
        if(!internal_mutex.try_lock())
            return false;
        update_hierarchy_value();
        return true;
    }
};
thread_local unsigned long hierarchical_mutex::this_thread_hierarchy_value(ULONG_MAX);//在#include<climits>里

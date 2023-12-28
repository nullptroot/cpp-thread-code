#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <iostream>
/*添加一个虚拟节点，push操作不需要即访问head又访问tail，不需要两个节点全加互斥
使得我们可以分别对head和tail进行加锁，锁粒度变小，并发性增加。*/
template <typename T>
class threadsafe_queue
{
    private:
        struct node
        {
            std::shared_ptr<T> data;
            std::unique_ptr<node> next;
        };
        std::mutex head_mutex;
        std::mutex tail_mutex;
        std::unique_ptr<node> head;
        node *tail;
        node *get_tail()
        {
            std::lock_guard<std::mutex> tail_lock(tail_mutex);
            return tail;
        }
        std::unique_ptr<node> pop_head()
        {
            /*此函数仅互斥加锁可能发生异常，但是数据改变在加锁后面
            因此是异常安全的
            
            总是先锁head再锁tail，规定了次序，因此不会发生死锁*/

            /*必须先对head进行上锁，如果tail上完锁，就不锁了，那么会
            出现下面的情况：
            得到的tail可能已经不是尾节点了，head可能都已经越过了tail
            因为在head上锁之前，可能head已经弹出了很多，已经越过tail了
            这就可能会出现问题，使用nullptr指针，因此需要先锁住head
            保证head不会越过tail*/
            std::lock_guard<std::mutex> head_lock(head_mutex);
            /*这里get_tail加锁是必要的，因为push和此函数都会访问tail
            如果不加锁就出现数据竞争，就完犊子了*/
            if(head.get() == get_tail())
                return nullptr;
            std::unique_ptr<node> old_head = std::move(head);
            head = std::move(old_head->next);
            return old_head;
        }
    public:
        threadsafe_queue():head(new node),tail(head.get()){};
        threadsafe_queue(const threadsafe_queue& other) = delete;
        threadsafe_queue & operator=(const threadsafe_queue &other) = delete;
        std::shared_ptr<T> try_pop()
        {
            std::unique_ptr<node> old_head = pop_head();
            return old_head ? old_head->data : std::shared_ptr<T>();
        };
        void push(T new_value)
        {
            /*下面两个内存分配可能发生异常，但是智能指针保证内存不会泄露
            并且发生异常后下面的改动数据操作不会执行，因此异常安全*/
            std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
            std::unique_ptr<node> p(new node);
            node *const new_tail = p.get();
            std::lock_guard<std::mutex> tail_lock(tail_mutex);
            /*这之后的代码不会发生异常*/
            tail->data = new_data;
            tail->next = std::move(p);
            tail = new_tail;
        };
};
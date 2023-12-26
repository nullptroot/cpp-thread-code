#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <iostream>
/*添加一个虚拟节点，push操作不需要即访问head又访问tail，不需要两个节点全加互斥
使得我们可以分别对head和tail进行加锁，锁粒度变小，并发性增加。
这样改动的目的：
push操作不在和之前一样，这里的push进会访问tail，不再访问head，因此仅需要锁住一个互斥即可

但是pop还是会访问所有的互斥，但是他仅在开头访问，因此仅需要短暂持锁

如何加锁呢，push比较简单，仅有一个锁，设计tail的都加上锁即可，
try_pop较为复杂，首先head会一直访问，需要贯穿整个调用，结果的返回不需要持锁
tail的访问也需要加锁，但是仅在访问的时候加锁即可，封装成一个函数即可，具体代码看
mini_lock_thread_safe_queue_list.cpp的改变
*/
template <typename T>
class queue
{
    private:
        struct node
        {
            std::shared_ptr<T> data;
            std::unique_ptr<node> next;
        };
        std::unique_ptr<node> head;
        node *tail;
    public:
        queue():head(new node),tail(head.get()){};
        queue(const queue& other) = delete;
        queue & operator=(const queue &other) = delete;
        std::shared_ptr<T> try_pop()
        {
            if(head.get() == nullptr)
            {
                return std::shared_ptr<T>();
            }
            std::shared_ptr<T> const res(head->data);
            std::unique_ptr<node> const old_head = std::move(head);//用来自动销毁数据
            head = std::move(old_head->next);
            return res;
        };
        void push(T new_value)
        {
            std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
            std::unique_ptr<node> p(new node);
            tail->data = new_data;
            node *const new_tail = p.get();
            tail->next = std::move(p);
            tail = new_tail;
        };
};
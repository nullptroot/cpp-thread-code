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
            std::lock_guard<std::mutex> head_lock(head_mutex);
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
            std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
            std::unique_ptr<node> p(new node);
            node *const new_tail = p.get();
            std::lock_guard<std::mutex> tail_lock(tail_mutex);
            tail->data = new_data;
            tail->next = std::move(p);
            tail = new_tail;
        };
};
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <iostream>
/*添加一个虚拟节点，push操作不需要即访问head又访问tail，不需要两个节点全加互斥
使得我们可以分别对head和tail进行加锁，锁粒度变小，并发性增加。*/
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
            std::unique_ptr<node> const old_head = std::move(head);
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
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <iostream>
/*单线程下还可以，但是多线程下就会出现问题
当使用于多线程时，我们就尝试用两个互斥分别锁住head和tail
这样实现细粒度锁，这样就会使得push和pop两个操作可以并行执行
因为push仅会锁住head，pop仅会锁住tail，因为是队列，这是
理想情况，因为下面的代码push操作会锁住两个节点，pop也是，因此pop
和push并不能并发操作，因为pop和push锁住了同一个互斥
因此这样的设计是不行的。这和锁全局是一样的
怎么办，看后面的代码，使用分离数据
来实现*/
template <typename T>
class queue
{
    private:
        struct node
        {
            T data;
            std::unique_ptr<node> next;
            node(T data_):data(std::move(data_)){};
        };
        std::unique_ptr<node> head;
        node *tail;
    public:
        queue():tail(nullptr){};
        queue(const queue& other) = delete;
        queue & operator=(const queue &other) = delete;
        std::shared_ptr<T> try_pop()
        {
            if(!head)
            {
                return std::shared_ptr<T>();
            }
            std::shared_ptr<T> const res(std::make_shared<T>(std::move(head->data)));
            std::unique_ptr<node> const old_head = std::move(head);
            head = std::move(old_head->next);
            if(!head)
                tail = nullptr;
            return res;
        };
        void push(T new_value)
        {
            std::unique_ptr<node> p(new node(std::move(new_value)));
            node *const new_tail = p.get();
            if(tail)
            {
                tail->next = std::move(p);
            }
            else
            {
                head = std::move(p);
            }
            tail = new_tail;
        };
};
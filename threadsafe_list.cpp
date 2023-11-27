#include <mutex>
#include <memory>
/*线程安全单向链表，实现了四个方法，分别是头插法，遍历，查找节点，删除节点
由于是单向链表，进行操作的时候，只需先锁住当前节点，然后获取当前节点的下一个
节点，然后是放当前节点的锁,然后处理下一个节点的数据，就能保证线程安全

这里实现有锁的线程安全数据结构，一半都是把其节点单独的声明出来，有一个
互斥锁mutex的数据成员，来实现线程安全*/
template<typename T>
class threadsafe_list
{
    struct node
    {
        std::mutex m;
        std::shared_ptr<T> data;
        std::unique_lock<node> next;
        node():next(){};
        node(T const &value):data(std::make_shared<T>(value)){};
    };
    node head;
    public:
        threadsafe_list(){};
        ~threadsafe_list()
        {
            remove_if([](node const &){return true;});
        }
        threadsafe_list(threadsafe_list const &other) = delete;
        threadsafe_list & operator = (threadsafe_list const &other) = delete;
        void push_front(T const &value)
        {
            /*很明显了，head是虚拟节点*/
            std::unique_ptr<node> new_node(new node(value));
            std::lock_guard<std::mutex> lk(head.m);
            new_node->next = std::move(head.next);
            head.next = std::move(new_node);
        }
        template<typename Function>
        void for_each(Function f)
        {
            node *current = &head;
            std::unique_lock<std::mutex> lk(head.m);
            while(node *const next = current->next.get())
            {
                std::unique_lock<std::mutex> next_lk(next->m);
                lk.unlock();
                f(*next->data);
                current = next;
                lk = std::move(next_lk);
            }
        }
        template<typename Predicate>
        std::shared_ptr<T> find_first_if(Predicate p)
        {
            node *current = &head;
            std::unique_lock<std::mutex> lk(head.m);
            while(node *const next = current->next.get())
            {
                std::unique_lock<std::mutex> next_lk(next->m);
                lk.unlock();
                if(p(*next->data))
                    return next->data;
                current = next;
                lk = std::move(next_lk);
            }
            return std::shared_ptr<T>();
        }
        template<typename Predicate>
        std::shared_ptr<T> remove_if(Predicate p)
        {
            node *current = &head;
            std::unique_lock<std::mutex> lk(head.m);
            while(node *const next = current->next.get())
            {
                std::unique_lock<std::mutex> next_lk(next->m);
                if(p(*next->data))
                {
                    std::unique_ptr<node> old_next = std::move(current->next);
                    current->next = std::move(next->next);
                    next_lk.unlock();
                }
                else
                {
                    lk.unlock();
                    current = next;
                    lk = std::move(next_lk);
                }
            }
        }
};
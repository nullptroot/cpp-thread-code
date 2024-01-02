#include <mutex>
#include <memory>
/*线程安全单向链表，实现了四个方法，分别是头插法，遍历，查找节点，删除节点
由于是单向链表，进行操作的时候，只需先锁住当前节点，然后获取当前节点的下一个
节点，然后是放当前节点的锁,然后处理下一个节点的数据，就能保证线程安全，是
按照一定的顺序加锁的，因此不会出现死锁

这里唯一可能发生条件竞争的是 remove_if函数的销毁对象的操作在锁释放后
（销毁以锁的互斥是未定义的，这里对象里有互斥），因此
可能有其他线程持有已删除节点的锁，但是仔细分析remove_if函数可以发现，这个问题
不存在，因为在删除节点时，上一个节点已经上锁了，不会有线程获取要删除节点的锁

这里实现有锁的线程安全数据结构，一半都是把其节点单独的声明出来，有一个
互斥锁mutex的数据成员，来实现线程安全

这里锁粒度已经很细了，因为下面的三个函数都可以并发的执行，只有涉及的节点
才会上锁*/
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
            /*先分配内存，并且对头节点进行上锁*/
            std::unique_ptr<node> new_node(new node(value));
            std::lock_guard<std::mutex> lk(head.m);
            new_node->next = std::move(head.next);
            head.next = std::move(new_node);
        }
        /*这里的f函数由用户保证不会引用节点的锁，避免出现死锁
        这里head是虚拟节点，因此在遍历过程中，先对当前节点上锁
        然后获取下一个节点的锁，然后释放上一个节点的锁，然后处
        里节点，然后锁转移*/
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
        /*和上面的for_each是一样的，不过这个需要直接返回*/
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
        /*这个会改变链表，其实整体过程和上面的差不多，也是交替
        加锁，然后处理具体的过程。*/
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
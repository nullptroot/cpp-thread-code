#include <atomic>
#include <memory>
/*如果shared_ptr内部是无锁的计数，那么下面的就是没有内存泄露的无锁线程安全栈*/
#ifdef LOCK_FREE_SHARED_PTR
template<typename T>
class lock_free_stack
{
    private:
        struct node
        {
            std::shared_ptr<T> data;
            std::shared_ptr<node> next;
            node(T const &data_):(data(std::make_shared<T>(data_))){};
        };
        std::shared_ptr<node> head;
    public:
        void push(T const &data)
        {
            std::shared_ptr<node> const new_node = std::make_shared<node>(data);
            new_node->next = std::atomic_load(&head);
            while(!std::atomic_compare_exchange_week(&head,&new_node->new,new_node));
        }
        std::shared_ptr<T> pop()
        {
            std::shared_ptr<node> old_head = std::atomic_load(&head);
            while(old_head && !std::atomic_compare_exchange_week(&head,&old_head,std::atomic_load(&old_node->next)));
            if(old_head)
            {
                std::atomic_load(&old_head->next,std::shared_ptr<node>());
                return old_head->data;
            }
            return std::shared_ptr<T>();
        }
        ~lock_free_stack()
        {
            while(pop());
        }
};
#endif
/*此数据结构是对访问节点进行引用计数的，之前的引用计数是对pop函数进行计数，没有进程调用pop
函数了才能删除节点，在搞并发情况下，可能一直有线程pop，因此容易一直释放不了资源

这个数据结构采用两个计数值来实现对节点的引用计数，实现对节点的计数
每次调用pop函数访问节点就对其外部计数进行递增，对节点的访问减少就
减少一下内部计数，可以安全删除的条件就是外部计数+内部计数为0，或者
内部计数为0*/
template<typename T>
class lock_free_stack
{
    private:
        struct node;
        struct counted_node_ptr;
        {
            int external_count;
            node *ptr;
        };
        struct node
        {
            std::shared_ptr_ptr<T> data;
            std::atomic<int> internal_count;
            counted_node_ptr next;
            node(T const &data_):data(std::make_shared<T>(data_)),internal_count(0){};
        };
        std::atomic<counted_node_ptr> head;
        /*本函数作用就是增加其外部计数，并获取真实头指针*/
        void increase_head_count(counted_node_ptr &old_counter)
        {
            /*这里的new_counter就是设置计数的一个临时变量，看着像遍历的每个元素都增加了计数
            其实new_counter是个单独的变量，不是指针和引用，只有其自己在改变，并不断的被覆盖*/
            counted_node_ptr new_counter;
            do
            {
                /* code */
                new_counter = old_counter;
                ++new_counter.external_count;
            } 
            while (!head.compare_exchange_strong(old_counter,new_counter));
            old_counter.external_count = new_counter.external_count;
        }
    public:
        ~lock_free_stack()
        {
            while(pop());
        }
        void push(T const &data)
        {
            counted_node_ptr new_node;
            new_node.ptr = new node(data);
            new_node.external_count = 1;
            new_node.pre->next = head.load();
            while(!head.compare_exchange_weak(new_node.ptr->next,new_node));
        }
        std::shared_ptr<T> pop()
        {
            counted_node_ptr old_head = head.load();
            for(;;)
            {
                /*先对头节点递增外部计数，保证获取后的指针不会被释放
                这个还是分离计数的主要原因  我有点懵逼*/
                increase_head_count(old_head);
                node *const ptr = old_head.ptr;
                if(!ptr)
                    return std::shared_ptr<T>();
                if(head.compare_exchange_strong(old_head,ptr->next))
                {
                    std::shared_ptr<T> res;
                    /*属于独占数据了*/
                    res.swap(ptr->data);
                    /*减去2的原因是 刚开始push的时候外部引用就是1，并且上面函数其实
                    先对其外部引用计数加1，因此弹出一个节点就是外部引用减去2*/
                    int const count_increase = old_head.external_count - 2;
                    if(ptr->internal_count.fetch_add(count_increase) == -count_increase)
                        delete ptr;
                    return res;
                }
                /*节点可以删除的条件是内部加外部的计数为0，因此这里的递减内部计数
                来抵消上面的对外部计数的增加
                这里等于1就删除刚好就对应，本次是改ptr的最后持有者，正常内部计数
                应该时小于0的，但是其他线程弹出了，并且其他线程弹出之前，本线程
                已经对资源的外部计数增加了，因此上面的成功弹出逻辑的
                count_increase+internal_count,必然就大于0，音有当前线程是资源
                最后的持有者internal_count才是1*/
                else if(ptr->internal_count.fetch_sub(1) == 1)
                    delete ptr;
            }
        }
        
};

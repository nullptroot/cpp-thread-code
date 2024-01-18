#include <atomic>
#include <memory>
/*下面是加上内存次序的无锁线程安全栈，这里主要涉及两个原子
变量的内存次序规定，栈顶head和内部计数internal_count
head的同步关系保证了使用head的时候，head指向的资源已经
被线程可见，internal_count的同步关系保证了，删除资源时
资源的获取已经完成。具体详情看下述代码，非常复杂*/
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
            /*不成功就relaxed就行了，因为不成功会一直循环，不涉及数据资源的访问
            一旦成功，那么后续就需要使用head的资源了，因此加上acquire序列，保证
            和push操作同步，这样就可以保证获取到head时，资源已经构建好入栈了，
            使用head访问不会出现资源访问不到的异常*/
            while (!head.compare_exchange_strong(old_counter,new_counter,std::memory_order_acquire,std::memory_order_relaxed));
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
            /*这里不影响后续对数据资源访问，因此宽松即可*/
            new_node.pre->next = head.load(std::memory_order_relaxed);
            /*这里的push操作需要使用release，因为要保证前面的操作已经完成，也就是
            资源已经分配好了，与后面的increase_head_count函数的acquire操作构成
            同步关系，也就是上面的资源分配代码可以由increase_head_count函数及其
            后面的语句看到，保证increase_head_count函数及其后面的操作可以访问到
            已经创建好的资源*/
            while(!head.compare_exchange_weak(new_node.ptr->next,new_node,std::memory_order_acquire,std::memory_order_relaxed));
        }
        std::shared_ptr<T> pop()
        {
            /*这里无所谓，只是获取一下原子变量的值，还没有涉及到资源的访问，内存次序无所谓
            后面的increase_head_count函数来实现同步的*/
            counted_node_ptr old_head = head.load(std::memory_order_relaxed);
            for(;;)
            {
                /*先对头节点递增外部计数，保证获取后的指针不会被释放
                这个还是分离计数的主要原因  我有点懵逼*/
                /*此函数里面涉及到了同步关系，并保证了访问时，资源肯定已经可见*/
                increase_head_count(old_head);
                node *const ptr = old_head.ptr;/*这里不会出现错误，因为资源已经由于同步关系可见*/
                if(!ptr)
                    return std::shared_ptr<T>();
                /*这里采用宽松即可，因为increase_head_count函数保证了栈内的资源
                已经可见，而下面的操作近涉及最新的资源和之前的资源，因此是安全的*/
                if(head.compare_exchange_strong(old_head,ptr->next,std::memory_order_relaxed))
                {
                    std::shared_ptr<T> res;
                    /*属于独占数据了*/
                    res.swap(ptr->data);
                    /*减去2的原因是 刚开始push的时候外部引用就是1，并且上面函数其实
                    先对其外部引用计数加1，因此弹出一个节点就是外部引用减去2*/
                    int const count_increase = old_head.external_count - 2;
                    /*这里需要保证在ptr删除之前，上面的res.swap已经完成了，因此需要
                    配合同步来实现先行关系，同一个线程在此块内不会有问题，因为代码顺序
                    逻辑先行，但是如果多个线程，一个在此块，一个在下面的else块，就需要
                    规定先行了*/
                    if(ptr->internal_count.fetch_add(count_increase,std::memory_order_release) == -count_increase)
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
                /*这里比较简单的就是直接写下面注释的代码，直接acquire次序，保证了
                delete ptr看到了上述的res.swap操作，才能删除ptr，但是这样
                会过于严格，因为能进行delete ptr的有且仅有一个线程，如果按照下面
                的内存次序，那么每一个进行此判断的线程，即使判断不成立，依然会
                产生同步效果，有点不优，我们想要仅成功的线程产生同步效果，也就是
                下面没有注释的代码*/
                // else if(ptr->internal_count.fetch_sub(1,std::memory_order_acquire) == 1)
                // {
                //     delete ptr;
                // }
                /*这里采用宽松内存次序，仅依靠原子变量的原子操作来判断条件是否成立
                成立后执行一个acquire操作实现同步就做到了优化。*/
                else if(ptr->internal_count.fetch_sub(1,std::memory_order_relaxed) == 1)
                {
                    ptr->internal_count.load(std::memory_order_acquire);
                    delete ptr;
                }
            }
        }      
};
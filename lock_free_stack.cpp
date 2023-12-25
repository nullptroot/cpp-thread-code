#include <memory>
#include <atomic>
/*下面是无锁的栈，但是还是会等待的，因为下面有死循环等待*/
template<typename T>
class lock_free_stack
{
    private:
        struct node
        {
            std::shared_ptr<T> data;
            node *next;
            node(T const data_):data(std::make_shared<T>(data_)){};
        };
        std::atomic<node *> head;
    public:
        void push(T const &data)
        {
            node *const new_node = new node(data);
            new_node->next = head.load();
            /*head的比较赋值，若head与new_node->next相等，那么head被赋值为new_node
            若head与new_node->next不相等，说明head被其他线程改动了
            那么new_node->next赋值为head，继续下一轮compare_exchange_weak
            直到可以复制成功*/
            while(!head.compare_exchange_weak(new_node->next,new_node));
            //比较交换执行下面的操作，原子执行
            // if(new_node->next == head)
            //     head = new_node;
            // else
            //     new_node->next = head;
        }
        /*这里pop并没有删除动态分配的内存，因此会发生内存泄露*/
        std::shared_ptr<T> pop()
        {
            node *old_head = head.load();
            /*这里和上述差不多，head与old_head比较，如果相等，head赋值为old_head->next
            否则old_head赋值为head继续下一轮循环，这里如果old_head != head那么说明
            head被其他线程改变了，old_head被设置为最新的head继续比较*/
            while(old_head && !head.compare_exchange_weak(old_head,old_head->next));
            return old_head ? old_head->data : std::shared_ptr<T> ();
        }
};
#include <atomic>
#include <memory>
/*下面是解决内存泄露的问题：
主要有两个操作会访问节点，一个是push操作，一个是pop操作，push把节点压入后
就不会再访问了，因此pop是我们需要关注的函数，
这里采用跟踪访问过的节点，也就是跟踪pop函数访问的节点，当没有线程调用
pop函数时，那么删除节点是绝对安全的，因此实际上是跟踪多少线程调用pop
函数，每次执行pop操作时，先不对节点进行删除，因为可能还有线程在访问
先放入候删链表，当没有线程调用pop时再删除节点，因此我们需要对pop函数
调用进行原子计数，当计数器变为0时，才删空候删链表*/

/*当负载较高时，pop会一直有线程调用，那么计数就一直不会为0，那么内存就
一直不能释放，导致内存泄露*/
template <typename T>
class lock_free_stack
{
    private:
        struct node
        {
            std::shared_ptr<T> data;
            node *next;
            node(T const data_):data(std::make_shared<T>(data_)){};
        };
        std::atomic<unsigned> threads_in_pop;
        std::atomic<node *> to_be_deleted;
        static void delete_nodes(node *nodes)
        {
            while(nodes)
            {
                node * next = nodes->next;
                delete nodes;
                nodes = next;
            }
        }
        void try_reclaim(node * old_head)
        {
            /*这里当计数为1时，说明进当前线程调用pop函数，并且前面没有pop函数
            那么当前线程要删除的节点，一定没有线程在引用，并且swap后，后续线程
            也引用不到，因此可安全删除当前节点*/
            if(threads_in_pop == 1)
            {
                /*这里逻辑独占候删链表，别的线程就无法访问了，但是
                其他线程可以向候删链表中添加元素，这个不影响*/
                node *nodes_to_delete = to_be_deleted.exchange(nullptr);
                /*这里需要二次判断，当独占候删链表后，并且仅有当前一个
                线程调用pop，那么删空候删链表时安全的，否则就不是安全的：
                详情可以看看书p221页，这里简单分析一下，就是，刚开始仅有
                线程a，阻塞在独占候删时阻塞，此时线程b调用pop获取头节点
                阻塞，还没删除，此时线程c调用pop，完成删除操作，头节点
                加入到候删链表，此时a恢复独占候删链表，如果不再次判断
                候删链表删空，线程b就出现异常*/
                if(!--threads_in_pop)
                    delete_nodes(nodes_to_delete);
                /*说明可能有线程调用pop引用了候删链表内容，因此把候删
                链表再加回去*/
                else if(nodes_to_delete)
                    chain_pending_nodes(nodes_to_delete);
                delete old_head;
            }
            else
            {
                /*加入候删链表*/
                chain_pending_nodes(old_head);
                --threads_in_pop;
            }
        }
        void chain_pending_nodes(node *nodes)
        {
            node * last = nodes;
            while(node *const next = last->next)
            {
                last = next;
            }
            chain_pending_nodes(nodes,last);
        }
        /*当向候删链表中加入节点后，下面判断既不再相等，因此
        不断地比较交换，把候删链表串联起来*/
        void chain_pending_nodes(node *first,node * last)
        {
            last->next = to_be_deleted;
            while(!to_be_deleted.compare_exchange_weak(last->next,first));
        }
        /*这个操作会把节点加到前面*/
        void chain_pending_node(node *n)
        {
            chain_pending_nodes(n,n);
        }
    public:
        std::shared_ptr<T> pop()
        {
            ++threadds_in_pop;
            node *old_head = head.load();
            while(old_head && !head.compare_exchange_weak(old_head,old_head->next));
            std::shared_ptr<T> res;
            if(old_head)
            /*这里swap，逻辑删除了节点数据，swap后，后续的线程便不能再
            引用这个地址，之前引用的管不了*/
                res.swap(old_head->data);
            /*加入候删链表了*/
            try_reclaim(old_head);
            return res;
        }
};
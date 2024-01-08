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
            node(T const data_):data(data_){};
        };
        std::atomic<node *> head;
    public:
        void push(T const &data)
        {
            /*  创建新节点
                令新节点的成员指针next指向当前head
                把head指针指向新节点
                
                这里head可能会被其他线程更改，因此要循环的比较交换*/
            /*这里有可能在构造函数中发生异常，不过无所谓，此时数据结构还没更改*/
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
        
        /*
        读取head指针的当前值
        读取head->next
        将head指针改为head->next
        弹出原栈顶元素节点node，获取data值作为返回值
        删除已弹出节点
        */
       /*这里pop并没有删除动态分配的内存，因此会发生内存泄露
       不释放内存的原因是因为，可能有的线程保存了此资源的指针
       其他的线程释放了，这时候再访问就会触发异常*/
       /*如若两个线程读取了同一个head节点，就错了，采用和push一样
       的比较交换操作，来避免上面的问题*/

       /*还有些问题，没处理空栈，如果head为nullptr，那么引用nullptr
       就异常了，返回值的话，在返回值过程中抛出异常就g了，解决的话使用引用
       但是此处却不好使了，因为如果返回引用了，但是却被其他线程提前一步
       弹出释放内存了，/？？？？不太理解p216页讲的问题，视频里讲的是赋值的异常
       得出结论还是需要返回值，但是改进返回智能指针*/
        void pop(T &result)
        {
            node *old_head = head.load();
            /*这里和上述差不多，head与old_head比较，如果相等，head赋值为old_head->next
            否则old_head赋值为head继续下一轮循环，这里如果old_head != head那么说明
            head被其他线程改变了，old_head被设置为最新的head继续比较*/
            while(!head.compare_exchange_weak(old_head,old_head->next));
            result = old_head->data;
        }
};
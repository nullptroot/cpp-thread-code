#include <atomic>
#include <thread>
#include <memory>
#include <functional>
/*这里主要思路就是为每一个线程对pop的调用所访问的节点，设置一个风险指针，风险
指针存储在公共区域，可以被所有线程看见，只有pop访问的指针没有被风险指针指涉
才可以释放资源，*/
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
        std::atomic<node *> head;

        /*下面这一堆 到pop函数都是风险指针所需要的函数*/
        /*最大多少个风险指针*/
        unsigned const max_hazard_pointers = 100;
        /*风险指针的实体结构*/
        struct hazard_pointer
        {
            std::atomic<std::thread::id> id;
            std::atomic<void *> pointer;
        };
        /*风险指针数组*/
        hazard_pointer hazard_pointers[max_hazard_pointers];

        /*对风险指针的一个封装*/
        class hp_owner
        {
                hazard_pointer *hp;
            public:
                /*不允许copy语义*/
                hp_owner(hp_owner const&) = delete;
                hp_owner operator =(hp_owner const &) = delete;
                /*初始化函数*/
                hp_owner():hp(nullptr)
                {
                    /*这里当线程需要获取一个风险指针时，遍历可以分配的位置*/
                    for(unsigned i = 0; i < max_hazard_pointers; ++i)
                    {
                        std::thread::id old_id;
                        /*这里old_id是空，只有hazard_pointers[i].id也为空（表示没有线程占用）时
                        才能设置成功，这个是个for循环，*/
                        if(hazard_pointers[i].id.compare_exchange_strong(old_id,std::this_thread::get_id()))
                        {
                            hp = &hazard_pointers[i];
                            break;
                        }
                    }
                    if(!hp)
                        throw std::runtime_error("No hazard pointers available");
                }
                /*给出指针，使风险指针可以赋值*/
                std::atomic<void *> &get_pointer()
                {
                    return hp->pointer;
                }
                /*把风险指针置空，并把id也置空*/
                ~hp_owner()
                {
                    hp->pointer.store(nullptr);
                    hp->id.store(std::thread::id());
                };
        };
        std::atomic<void *> & get_hazard_pointer_for_current_thread()
        {
            /*thread local线程开始时分配，线程结束时释放，因此只有当线程结束时
            hazard才会归还，风险指针才会置空*/
            thread_local static hp_owner hazard;
            return hazard.get_pointer();
        }
        /*遍历风险指针数组，看当前指针是否被风险指针指涉*/
        bool outstanding_hazard_pointers_for(void *p)
        {
            for(unsigned i = 0; i < max_hazard_pointers; ++i)
            {
                if(hazard_pointers[i].pointer.load() == p)
                    return true;
            }
            return false;
        }
        /*下面是一个泛型候删链表*/
        template<typename T>
        void do_delete(void *p)
        {
            delete static_cast<T*>(p);
        }
        struct data_to_reclaim
        {
            /* data */
            void *data;
            std::function<void(void*)> deleter;
            data_to_reclaim *next;
            template<typename T>
            data_to_reclaim(T * p):data(p),deleter(&do_delete<T>),next(nullptr){};
            ~data_to_reclaim()
            {
                deleter(data);
            }
        };
        std::atomic<data_to_reclaim *> nodes_to_reclaim;/*候删链表*/
        /*向候删链表添加节点*/
        void add_to_reclaim_list(data_to_reclaim *node)
        {
            node->next = nodes_to_reclaim.load();
            while(!nodes_to_reclaim.compare_exchange_weak(node->next,node));
        }
        /*过会儿再删*/
        template<typename T>
        void reclaim_later(T *data)
        {
            add_to_reclaim_list(new data_to_reclaim(data));
        }
        /*释放没有被风险指针指涉的资源*/
        void delete_nodes_with_no_hazard()
        {
            /*独占候删链表*/
            data_to_reclaim *current = nodes_to_reclaim.exchange(nullptr);
            /*遍历候删链表，删除没有被风险指针指涉的资源*/
            while(current)
            {
                data_to_reclaim *const next = current->next;
                if(!outstanding_hazard_pointers_for(current->data))
                    delete current;
                else
                    add_to_reclaim_list(current);/*有风险指针指涉的就放入候删链表*/
                current = next;
            }
        }
        std::shared_ptr<T> pop()
        {
            /*获取风险指针*/
            std::atomic<void *> &hp = get_hazard_pointer_for_current_thread();
            node *old_head = head.load();
            /*外层循环保证了正确删除head头，并设置old_head指针*/
            do
            {
                /*内层循环保证了风险指针指向了head头节点*/
                node *temp;
                do
                {
                    temp = old_head;
                    hp.store(old_head);
                    old_head = head.load();
                } 
                while (old_head != temp);
            }
            while(old_head && !head.compare_exchange_strong(old_head,old_head->next));
            /*只有退出循环，那么说明已经弹出一个节点了，并且old_head记录了弹出的节点*/
            hp.store(nullptr);
            std::shared_ptr<T> res;
            if(old_head)
            {
                res.swap(old_head->data);/*先对其独占，彰显删除*/
                if(outstanding_hazard_pointers_for(old_head))/*有风险指针指涉待会再删除*/
                    reclaim_later(old_head);
                else/*否则直接删除*/
                    delete old_head;
                delete_nodes_with_no_hazard();/*遍历一遍，看看有没有可以删除的节点*/
            }
            return res;
        }
};
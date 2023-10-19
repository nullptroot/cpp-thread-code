#include <mutex>
/*防止死锁的规范一般是 按照固定顺序加锁，但是如果在函数内
按照函数的参数顺序进行加锁就会出现问题
导致加锁顺序发生变化，此时可以用到下面两个
函数lock scope_lock一块加锁*/
class some_big_object
{
    public:
        int x;
};
void swap(some_big_object &lhs,some_big_object &rhs)
{
    int temp = lhs.x;
    lhs.x = rhs.x;
    rhs.x = temp;
}
class X
{
    private:
        some_big_object some_detail;
        std::mutex m;
    public:
        X(some_big_object &sd):some_detail(sd){};
        // friend void swap(X &lhs,X &rhs)
        // {
        //     /*//同一线程多次获取同一个mutex将未定义  std::recuresive_mutex可以*/
        //     if(&lhs == &rhs)
        //         return;
        //     std::lock(lhs.m,rhs.m);//锁住两个互斥  全局共成败 要么全锁 异常后全不锁
        //     /*std::adopt_lock 指明锁已经锁住了，只是获取该锁的归属权，也就是析构后解锁*/
        //     std::lock_guard<std::mutex> lock_a(lhs.m,std::adopt_lock);
        //     std::lock_guard<std::mutex> lock_b(rhs.m,std::adopt_lock);
        //     swap(lhs.some_detail,rhs.some_detail);
        // }
        // friend void swap(X &lhs,X &rhs)
        // {
        //     /*//同一线程多次获取同一个mutex将未定义  std::recuresive_mutex可以*/
        //     if(&lhs == &rhs)
        //         return;
        //     /*可变参数模板  下面用到模板参数推导
        //     结合了上面的lock lock_guard*/
        //     std::scoped_lock guard(lhs.m,rhs.m);
        //     swap(lhs.some_detail,rhs.some_detail);
        // }
        /*p_60 code 又一种管理锁的对象*/
        friend void swap(X &lhs,X &rhs)
        {
            if(&lhs == &rhs)
                return;
            /*下面参数std::defer_lock 表示先关联锁 但是现在还没有上锁  先获取了
            锁的管理权
            此类型性能不好 能用lock_guard就不用它，它可以实现延迟加锁
            或者锁管理权转移*/
            std::unique_lock<std::mutex> lock_a(lhs.m,std::defer_lock);
            std::unique_lock<std::mutex> lock_b(rhs.m,std::defer_lock);
            std::lock(lhs.m,rhs.m);
            swap(lhs.some_detail,rhs.some_detail);
        }
};
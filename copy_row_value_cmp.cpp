#include <mutex>
#include <vector>
#include <iostream>
std::vector a;
/*下面的比较操作就是 要比较的对象 构造起来比较简单
并且只是比较，那么就可以转换为下面这种，先加一个锁
获取一个值的副本，再加另一个锁获取副本再比较，不过
这样的比较语义就变了，可能不是同一时刻的值*/
class Y
{
    private:
        int some_detail;
        mutable std::mutex m;
        int get_detail() const
        {
            std::lock_guard<std::mutex> lock_a(m);
            return some_detail;
        };
    public:
        Y(int sd):some_detail(sd){};
        friend bool operator==(Y const &lhs,Y const &rhs)
        {
            if(&lhs == &rhs)
                return 1;
            int const lhs_value = lhs.get_detail();
            int const rhs_value = rhs.get_detail();
            return lhs_value == rhs_value;
        }
};
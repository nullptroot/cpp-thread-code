#include <exception>
#include <memory>

/*定义所用头文件*/
#include <mutex>
#include <stack>

/*定义一个空栈异常*/
class empty_stack :std::exception
{
    const char * what() const noexcept;
};
template <typename T>
class threadsafe_stack
{
    public:
        threadsafe_stack(){};
        threadsafe_stack(const threadsafe_stack &other)
        {
            /*为何呢 只锁源  好像不用锁本对象  多线程操作也无所谓啊 谁最后
            操作 谁生效，锁住源对象是因为  防止复制过程源对象数据改变*/
            std::lock_guard<std::mutex> lock(other.m);
            data = other.data;
        };
        threadsafe_stack & operator=(const threadsafe_stack &) = delete;//不允许赋值构造函数产生；
        /*异常安全的函数*/
        void push(T new_value)
        {
            std::lock_guard<std::mutex> lock(m);
            data.push(std::move(new_value));//可能发生异常，stack内部内存不够，但是会保证自身安全  没有问题
        }
        /*返回共享指针指向弹出元素 nice*/
        /*异常安全的函数*/
        std::shared_ptr<T> pop()
        {
            std::lock_guard<std::mutex> lock(m);
            if(data.empty())
                throw empty_stack();//这里可能抛出异常，但是抛出后，也没有任何状态被改变
            //下面内存不够可能发生异常，但是make_shared保证不会内存泄露
            std::shared_ptr<T> const res(std::make_shared<T>(data.top()));//改动栈容器前设置返回值
            data.pop();
            return res;
        }
        /*传入引用：
            优点：加上锁后就可以避免线程安全， 且可以保证异常安全
            缺点：需要先构造一个对象，*/
        void pop(T &value)
        {
            std::lock_guard<std::mutex> lock(m);
            if(data.empty())
                throw empty_stack();
            value = std::move(data.top());//可能发生异常，但是发生异常了下面的pop还没有执行，异常安全的
            data.pop();
        }
        bool empty() const
        {
            std::lock_guard<std::mutex> lock(m);
            return data.empty();
        }
    private:
        std::stack<T> data;
        mutable std::mutex m;
};
/*上述代码可能发生死锁，用户自定义的一些函数比如复制构造，移动构造、copy赋值、移动赋值、重载的new等
当涉及添加元素的时候，调用了用户自定义函数，其中又调用了本类接口，尝试获取锁，导致死锁，
解决向使用者提出要求，避免上述死锁

本类除了构造函数和析构函数，其他都是线程异常安全函数，需要使用者保证，容器未构造完，其他线程不可以访问数据
且需要所有线程停止访问后销毁容器

本类仅有一个互斥，锁住全部的数据，多线程仅能串行化访问数据，性能不行*/
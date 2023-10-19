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
        void push(T new_value)
        {
            std::lock_guard<std::mutex> lock(m);
            data.push(std::move(new_value));
        }
        /*返回共享指针指向弹出元素 nice*/
        std::shared_ptr<T> pop()
        {
            std::lock_guard<std::mutex> lock(m);
            if(data.empty())
                throw empty_stack();
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
            value = data.top();
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
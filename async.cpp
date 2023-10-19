#include <iostream>
#include <string>
#include <future>

struct X
{
    void foo(int i,std::string const &str)
    {
        std::cout<<"int :"<<i<<"string :"<<str<<std::endl;
    }
    std::string bar(std::string const &str)
    {
        return str;
    }
};
X x;
/*调用p->foo(42,"hello") p = &x*/
auto f1 = std::async(&X::foo,&x,42,"hello");
/*调用的是tmpx.bar("goodBye") tmpx是x的副本*/
auto f2 = std::async(&X::bar,x,"goodBye");
struct Y
{
    double operator()(double d)
    {
        return d;
    }
};
Y y;
/*调用的是tmpy(3.141)其中tmpy是由Y()生成的一个匿名变量，传递给
async() 进而发送移动构造，在async内部产生对象tmpy，在tmpy上
执行Y::operator()*/
auto f3 = std::async(Y(),3.141);
/*调用y(2.787)*/
auto f4 = std::async(std::ref(y),2.787);
X baz(X &x)
{
    return x;
}
/*调用baz(x)*/
auto tmp = std::async(baz,std::ref(x));

class move_only
{
    public:
        move_only(){};
        move_only(move_only &&){};
        move_only(move_only const &) = delete;
        move_only & operator=(move_only &&){return *this;};
        move_only & operator=(move_only const &&) = delete;;
        void operator()(){};
};
/*调用tmp()其中tmp等价于std::move(move_only()),他的产生过程和f3相似*/
auto f5 = std::async(move_only());

/*直接开始运行新线程*/
auto f6 = std::async(std::launch::async,Y(),1.2);
/*在后续调用到wait() get()后运行函数*/
auto f7 = std::async(std::launch::deferred,baz,std::ref(x));
/*下面两个由内部实现如何运行*/
auto f8 = std::async(std::launch::deferred | std::launch::async,baz,std::ref(x));
auto f9 = std::async(baz,std::ref(x));
/*f7在调用这个函数后才开始运行*/
auto c = f7.get();
/*下面那条书上的语句不能通过编译*/
// f7.wait();
/*得到的future变量，在执行wait()或者get()方法后，如果运行完了，就得到结果
没有运行完就阻塞等待*/
int main()
{

}
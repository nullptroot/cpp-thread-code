#include <thread>
#include <iostream>
#include <unistd.h>
using std::cout;
using std::endl;
using std::thread;
void some_function()
{
    sleep(10);//主线程结束（main）就全部结束了
    cout<<"some_function"<<endl;
    
}
void some_function1()
{
    sleep(10);
    cout<<"some_function1"<<endl;
    
}

thread f()
{
    return thread(some_function);
}

thread g()
{
    thread t(some_function1);
    return t;
}

void f(thread t)
{
    cout<<"函数传thread值"<<endl;
    t.detach();
}
//函数传值thread对象
int main()
{
    f(thread(some_function1));
    thread t1(some_function);
    f(std::move(t1));
    // f(t1);
    /*
    上述函数会转化为thread _temp,_temp.thread::thread(t1),
    ,copy contructor 会报错  f(_temp);
    这样就不行了，上述函数会构建临时对象，copy构造自t1，
    然而thread的copy 构造是delete类型，因此会出错，变为
    移动复制就ok了，*/
    return 0;
}
#if 0
/*函数返回一个thread对象*/
int main()
{
    thread t1 = f();
    thread t2 = g();
    /*首先这种方式的赋值，会被转化为f和g函数多加一个thread&的
    参数，也就是thread t1,f(t1)，然后f里面，构建局部对象t，操作t
    然后move给t1，结束，还有可能，直接不构建局部t，直接操作t1看
    编译器优化
    根据上面的转换，也就看出是合理的，因为采用的都是移动赋值，是
    合法的，有编译器优化的甚至直接操作t1，所以是合法的*/
    t1.detach();
    t2.detach();
    sleep(2);
    return 0;
}
#endif
#if 0
//对已保存的为detach的thread对象进行赋值
int main()
{
    thread t1(some_function);
    cout<<"sss"<<endl;
    thread t2 = std::move(t1);
    cout<<"sss"<<endl;
    t1 = thread(some_function1);
    thread t3;
    t3 = std::move(t2);
    // t1.detach();
    t1 = std::move(t3);
    t1.join();
    /*上述会触发terminate 在thread的析构函数中发出，因为
    t1已经保存一个线程，给其进行赋值导致该线程无人管了，而线程
    无人管需要调用detach分离才可以，要不然就出错 ，加上t1.detach();
    这句就没事了。相当于不管t1里村的线程了
    线程结束前还没调用join或者detach就会引发异常，*/
    sleep(2);
    return 0;
}
#endif
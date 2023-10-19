#include <thread>
#include <iostream>
#include <string>
using std::string;

void update_data_for_string(int id,string &s)
{
    std::cout<<"线程函数非const引用，如和构造线程类"<<std::endl;
}
class X
{
    public:
        void do_some()
        {
            std::cout<<"类成员函数调用"<<std::endl;
        }
};
void process_big_object(std::unique_ptr<int> x)
{
    std::cout<<"左值变量转为右值变量 move构造函数"<<std::endl;
}
int main()
{
    string s = "qwewewe";
    std::thread t(update_data_for_string,0,std::ref(s));
    // std::thread t(update_data_for_string,0,s);
    /*上面那个语句是不能通过编译的，上述线程函数接受一个非const引用
    但是thread的构造函数并不知情，会把s的副本（由s复制得出）当作
    move-only类型，以右值传递，然后线程函数接受到一个右值，非const
    引用不能接受右值，失败，std::ref就是把其变为引用传入构造函数*/
    X my_x;
    std::thread t1(&X::do_some,&my_x);
    // std::cout<<&X::do_some<<std::endl; //这里打印出1
    /*其实看完对象模型 上述代码就可以很好理解了，取其类的函数地址
    对象中没有存储成员函数，成员函数另有位置存储，上述取其类成员
    函数其实就是取得offset，然后do_some的第一个参数就是this指针
    (this->vb_func + 1)(this)
    */
   int *a = new int(9);
   std::unique_ptr<int> uniqX(a);
   std::thread t2(process_big_object,std::move(uniqX));
//    std::thread t2(process_big_object,uniqX);
/*上面这句是不对的，因为unique_ptr是不可复制的，而线程函数是
按值传递，必然会导致临时对象复制构造和copy构造函数创建，不能
通过编译，只能通过move来搞*/
    t.join();
    t1.join();
    t2.join();
    return 0;
}
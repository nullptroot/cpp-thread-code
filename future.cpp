#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <future>
int find_the_answer_to_ltuae()
{
    std::cout<<"在异步运行"<<std::endl;
    sleep(2);
    
    return 9;
}

int main()
{
    std::future<int> the_answer = std::async(find_the_answer_to_ltuae);
    std::cout<<"主线程运行"<<std::endl;
    sleep(1);
    /*当运行到the_answer.get()时，异步的线程没有运行完，就会阻塞等待
    运行完返回结果*/
    std::cout<<"the answer is "<<the_answer.get()<<std::endl;
    return 0;
}
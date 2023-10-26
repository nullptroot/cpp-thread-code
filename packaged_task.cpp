#include <deque>
#include <mutex>
#include <future>
#include <thread>
#include <utility>
#include <iostream>
#include <unistd.h>
/*packaged_task 封装一个任务  可以得到一个future对象
什么时候使用啥时候调用future的get方法

它是一个可调用对象，给用户了一个得到future的方法，可以
自己调用，也可以加入到线程异步调用
*/
std::mutex m;
std::deque<std::packaged_task<void()>> tasks;

bool gui_shutdown_message_received()
{
    return false;
}
void get_and_process_gui_message()
{
    std::cout<<"processing..."<<std::endl;
    sleep(2);
}
/*这是个gui线程 不断地轮询执行*/
void gui_thread()
{
    /*没收到shutdown消息就一直轮询*/
    while(!gui_shutdown_message_received())
    {
        /*获取并且处理gui的消息*/
        get_and_process_gui_message();
        std::packaged_task<void()> task;
        {
            std::lock_guard<std::mutex> lk(m);
            /*若任务队列为空，即没有消息要处理 继续轮询*/
            if(tasks.empty())
                continue;
            /*有任务的话，就获取任务 弹出任务*/
            task = std::move(tasks.front());
            tasks.pop_front();
        }
        /*直接执行任务*/
        task();
    }
}
/*开始线程*/

/*布置任务的函数*/
template<typename Func>
std::future<void> post_task_for_gui_thread(Func f)
{
    /*将任务和一个packaged_task绑定*/
    std::packaged_task<void()> task(f);
    /*获取到其future*/
    std::future<void> res = task.get_future();
    std::lock_guard<std::mutex> lk(m);
    /*将其加入到任务队列中*/
    tasks.push_back(std::move(task));
    /*返回给调用者future 供其使用*/
    return res;
}
int main()
{
    std::thread gui_bg_thread(gui_thread);
    auto cmp = [](){std::cout<<"任务在执行"<<std::endl;sleep(3);};
    /*不可以copy  但是下面会移动构造，或者编译器优化，直接把f当函数的res使用了*/
    auto f = post_task_for_gui_thread(cmp);
    /*future可移动不可以copy*/
    // auto g = f; //这就不行
    // auto g = std::move(f); // 这样就可以
    f.get();
    gui_bg_thread.join();/*别忘记join*/
    return 0;

}
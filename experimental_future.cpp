// #include <experimental/future>
#include "../future/experimental/future"
/*下面的不明所以*/
// std::experimental::future<int> find_the_answer
// /*这里find_the_answer() 会返回int值*/
// auto fut = find_the_answer();
/*这里fut应该是个future 支持后续调用的*/
// auto fut2 = fut.then(find_the_question);

/*下述代码 返回future 对应的线程自行执行*/
template<typename Func>
/*declval 返回一个类型的右值引用*/
std::experimental::future<decltype(std::declval<Func>()())>
spawn_async(Func &&func)
{
    std::experimental::promise<decltype(std::declval<Func>()())> p;
    auto res = p.get_future();
    std::thread t([p=std::move(p),f = std::decay_t<Func>(func)] () mutable
    {
        try
        {
            p.set_value_at_thread_exit(f());
        }
        catch (...)
        {
            p.set_exception_at_thread_exit(std::current_exception());
        }
    });
    t.detach();
    return res;
}
/*串行执行的登录操作*/
void process_login(std::string const &username,std::string const &password)
{
    try
    {
        user_id const id = backend.authenticate_user(username,password);
        user_data const info_to_display = backend.request_current_info(id);
        update_display(info_to_display);
    }
    catch(std::exception &e)
    {
        display_error(e);
    }
}
/*异步任务处理用户登录

但是下面任务因为承担过多的任务而发生阻塞
我们应该按照后续函数的方式，将任务接合
形成调用链，每完成一个任务就执行下一个任务
和之前的消息通知似的*/
std::future<void> process_login(std::string const &username,std::string const &password)
{
    return std::async(std::launch::async,[=] ()
    {
        try
        {
            user_id const id = backend.authenticate_user(username,password);
            user_data const info_to_display = backend.request_current_info(id);
            update_display(info_to_display);
        }
        catch(std::exception &e)
        {
            display_error(e);
        }
    });
}
/*下面使用后续函数调用
这里的then函数会接受上一个函数的返回值作为
参数继续调用函数，then函数返回值也是future
对象 也就是async返回一个future对象，然后调用
then继续执行，然后再返回future对象

下面的调用链有一个函数阻塞了就会都阻塞住

authenticate_user 返回user_id
request_current_info 返回user_data
*/
std::experimental::future<void> process_login(std::string const &username,std::string const &password)
{
    return std::async(std::launch::async,[=] ()
    {
        return backend.authenticate_user(username,password);
    }).then([](std::experimental::future<user_id> id)
    {
        return backend.request_current_info(id.get());
    }).then([](std::experimental::future<user_data> info_to_display)
    {
        try
        {
            update_display(info_to_display.get());
        }
        catch(std::exception &e)
        {
            display_error(e);
        }
    })
}

/*下面后续函数的调用不会出现future<future<some_value>>
的情况，因为有个future展开的操作（根折叠引用似的），
就是不用担心会多层future

async_authenticate_user 返回future<user_id>
async_request_current_info 但会future<user_data>
*/
std::experimental::future<void> process_login(std::string const &username,std::string const &password)
{
        return backend.async_authenticate_user(username,password)
        .then([](std::experimental::future<user_id> id)
    {
        return backend.async_request_current_info(id.get());
    }).then([](std::experimental::future<user_data> info_to_display)
    {
        try
        {
            update_display(info_to_display.get());
        }
        catch(std::exception &e)
        {
            display_error(e);
        }
    })
}

/*下面是experimental::shared_future
他支持多个then的调用，fut是shared
下面fut2和fut3就不是shared了
需要显示的加上.share()才行
后续的参数需要声明为shared才合法*/
auto fut = spawn_async(some_function).share();

auto fut2 = fut.then([](std::experimental::shared_future<some_data> data)
{
    do_stuff(data);
});

auto fut3 = fut.then([](std::experimental::shared_future<some_data> data)
{
    do_other_stuff(data);
});
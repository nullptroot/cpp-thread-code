#include <string>
struct card_inserted
{
    std::string acount;
};
/*消息传递同步机制  分析完代码发现其实就是一个有限状态机，涉及状态转换*/
class atm
{
    messaging::receiver incoming;
    messaging::sender bank;
    messaging::sender interface_hardware;
    void (atm::*state)();
    std::string account;
    std::string pin;
    void waiting_for_card()
    {
        /*硬件发出信号  请插入卡*/
        interface_hardware.send(display_enter_card());
        /*静候消息处理回复  wait后面绑定一个一种消息类型（这里
        是插入卡，若接受到的消息类型不匹配，便舍弃继续等待）*/
        incoming.wait().handle<card_inserted>([&](card_inserted const &msg)
        {
            account = msg.acount;
            pin="";
            /*硬件发出信号 请输入密码  更换状态了 下次run
            函数里的循环就执行getting_pin了*/
            interface_hardware.send(display_enter_pin());
            state = &atm::getting_pin;
        });
    }
    void getting_pin()
    {
        /*这里等待三种消息，数字的按压 清除最后一个输入 取消按压
        这里只有pin长度符合要求时才会发生状态通知*/
        incoming.wait().handle<digit_pressed>([&](digit_pressed const &msg)
        {
            unsigned const pin_length = 4;
            ping += msg.digit;
            if(pin.length() == pin_length)
            {
                bank.send(verify_pin(account,pin,incoming));
                state = &atm::verifying_pin;
            }
        }).handle<clear_last_pressed>([&](clear_last_pressed const &msg)
        {
            if(!pin.empty())
                pin.resize(pin.length()-1);
        }).handle<cancle_pressed>([&](concle_pressed const &msg)
        {
            state = &atm::done_processing;
        });
    }
    void run()
    {
        state = &atm::waiting_for_card;
        try
        {
            for(;;)
                (this->*state)();
        }
        catch(messaging::close_queue const &)
        {
        }
        
    }
}

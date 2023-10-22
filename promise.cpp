#include <future>
#include <vector>
#include <iostream>
/*这里 promise是和future相关联的
future阻塞等待数据，promise调用
set_value方法，就表示数据设置成功
future就可以得到数据了。
下面的东西过于复杂，不好实现，就直接抄的书*/
void process_connection(connection_set &connections)
{
    while(!done(connections))
    {
        for(auto connection = connections.begin(),end = connections.end(); connection != end;++connection)
        {
            if(connection->has_incoming_data())
            {
                data_packet data = connection->incoming();
                std::promise<payload_type> &p = connection->get_promise(data.id);
                p.set_value(data.payload);
            }
            if(connection->has_outgoing_data())
            {
                outgoing_packet data = connection->top_of_outging_queue();
                connecttion->send(data.payload);
                data.promise.set_value(true);
            }
        }
    }
}
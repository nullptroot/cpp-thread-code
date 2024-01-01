#include <memory>
#include <shared_mutex>
#include <algorithm>
#include <unordered_map>
#include <list>
#include <mutex>
#include <vector>
#include <map>
/*线程安全hash表的实现，冲突使用了拉链法*/
/*cpp代码实现线程安全哈希表，可以安全的用在多线程任务中，代码如下：

主要有两个部分，threadsafe_lookup_table内部封装了一个安全hash桶，
每个hash桶里面其实是一种线程安全的链表，也就是把数据安装k分散到不同链表实现并发
内部的安全hash桶可以安全的读取、存入kv和删除kv，这里根据键值获取值
属于读哈希表，可以多个线程共同读，因此使用std::shared_lock\<std::shared_mutex\> lock(mutex)，
而剩下两个方法需要改写哈希表，因此要是有互斥独占锁。

还有一个获取map的方法，此方法需要访问所有的元素，因此需要对所有元素互斥上锁（获取map期间，不允许其他线程更改哈希表）

具体代码如下*/
template<typename Key,typename Value,typename Hash=std::hash<Key>>
class threadsafe_lookup_table
{
    private:
        class bucket_type
        {
            private:
                typedef std::pair<Key,Value> bucket_value;
                typedef std::list<bucket_type> bucket_data;
                typedef typename bucket_data::iterator bucket_iterator;
                bucket_data data;
                /*shared_mutex支持多个线程读，单个线程改*/
                mutable std::shared_mutex mutex;
                bucket_iterator find_entry_for(Key const &key) const
                {
                    return std::find_if(data.begin(),data.end(),[&](bucket_value const &item){return item.first == key;});
                }
            public:
                /*不做任何改动，是异常安全的*/
                Value value_for(Key const &key,Value const &default_value) const
                {
                    /*共享访问*/
                    std::shared_lock<std::shared_mutex> lock(mutex);
                    bucket_iterator const found_entry = find_entry_for(key);
                    return (found_entry == data.end()) ? default_value : found_entry->second;
                }
                void add_or_update_mapping(Key const &key,Value const &value)
                {
                    /*独占访问*/
                    std::unique_lock<std::shared_mutex> lock(mutex);
                    bucket_iterator const found_entry = find_entry_for(key);
                    if(found_entry == data.end())
                    /*下面是异常安全的*/
                        data.push_back(bucket_value(key,value));
                    else
                    /*这里赋值操作不是异常安全的，留给用户保证赋值操作是异常安全的*/
                        found_entry->second = value;
                }
                /*此函数是异常安全的*/
                void remove_mapping(Key const &key)
                {
                    std::unique_lock<std::shared_mutex> lock(mutex);
                    bucket_iterator const found_entry = find_entry_for(key);
                    if(found_entry != data.end())
                        data.erase(found_entry);
                }
        };
        std::vector<std::unique_ptr<bucket_type>> buckets;
        Hash hasher;
        /*桶的数目固定，因此此函数不需要加锁，后续调用get_bucket也不需要加锁*/
        bucket_type & get_bucket(Key const &key) const
        {
            std::size_t const bucket_index = hasher(key) % buckets.size();
            return *buckets[bucket_index];
        }
    public:
        typedef Key key_type;
        typedef Value mapped_type;
        typedef Hash hash_type;
        threadsafe_lookup_table(unsigned num_buckets = 19,Hash const &header_ = Hash()):
                                    buckets(num_buckets),hasher(hasher_)
        {
            for(unsigned i = 0; i < num_buckets; ++i)
                buckets[i].reset(new bucket_type);
        }
        threadsafe_lookup_table(threadsafe_lookup_table const &other) = delete;
        threadsafe_lookup_table& operator = (threadsafe_lookup_table const &other) = delete;
        Value value_for(Key const &key,Value const &default_value = Value()) const
        {
            /*桶的数目固定，因此此函数不需要加锁，*/
            return get_bucket(key).value_for(key,default_value);
        }
        void add_or_update_mapping(Key const &key,Value const &value)
        {
            /*桶的数目固定，因此此函数不需要加锁，*/
            get_bucket(key).add_or_update_mapping(key,value);
        }
        void remove_mapping(Key const &key)
        {
            /*桶的数目固定，因此此函数不需要加锁，*/
            get_bucket(key).remove_mapping(key);
        }

        std::map<Key,Value> get_map() const
        {
            std::vector<std::unique_lock<std::shared_mutex>> locks;
            for(unsigned i = 0; i < buckets.size(); ++i)
            {
                locks.push_back(std::unique_lock<std::shared_mutex>(buckets[i].mutex));
            }
            std::map<Key,Value> res;
            for(unsigned i = 0; i < buckets.size(); ++i)
            {
                for(auto it = buckets[i].data.begin(); it != buckets[i].data.end(); ++it)
                    res.insert(*it);
            }
            return res;
        }
};
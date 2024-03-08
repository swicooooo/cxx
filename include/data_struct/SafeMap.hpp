#pragma once

#include <map>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <iterator>
#include <algorithm>
#include <vector>
#include <memory>

template<typename Key,typename Value, typename Hash=std::hash<Key>>
class SafeMap{

class BucketType{
    friend class SafeMap;
public:
    Value valueFor(const Key& key, const Value& defaultValue){
        std::shared_lock<std::shared_mutex> lock(smt_);
        auto item=findEntryFor(key);
        return item==data_.end()?defaultValue:item->second;
    }
    void addOrUpdateFor(const Key& key, const Value& value){
        std::unique_lock<std::shared_mutex> lock(smt_);
        auto item=findEntryFor(key);
        if(item==data_.end())
            data_.emplace_back(key,value);
        else
            item->second=value;
    }
    void romoveFor(const Key& key){
        std::unique_lock<std::shared_mutex>lock(smt_);
        auto item=findEntryFor(key);
        if(item!=data_.end())
            data_.erase(item);
    }

private:
    using bucket_value=std::pair<Key,Value>;
    using bucket_data=std::list<bucket_value>;
    using bucket_iterator=typename bucket_data::iterator;
    bucket_data data_;
    mutable std::shared_mutex smt_;

    bucket_iterator findEntryFor(const Key& key){
        return std::find_if(data_.begin(), data_.end(), [&key](const bucket_value& item){
            return item.first==key;
        });
    }
};

public:
    SafeMap(int num_buckets=19,const Hash& hasher=Hash()):buckets_(num_buckets),hasher_(hasher){
        for(unsigned i=0;i<num_buckets;++i)
            buckets_[i].reset(new BucketType);
    }
    SafeMap(const SafeMap& other)=delete;
    SafeMap& operator=(const SafeMap& other)=delete;

    Value valueFor(const Key& key, const Value& defaultValue){
        return getBucket(key).valueFor(key,defaultValue);
    }
    void addOrUpdateFor(const Key& key, const Value& value){
        getBucket(key).addOrUpdateFor(key,value);
    }
    void removeFor(const Key& key){
        getBucket(key).romoveFor(key);
    }
    std::map<Key,Value> getMap(){
        std::vector<std::unique_lock<std::shared_mutex>> locks;
        for(unsigned i=0;i<buckets_.size();++i)
            locks.push_back(std::unique_lock<std::shared_mutex>(buckets_[i]->smt_));
        std::map<Key, Value> res;
        for(auto& bucket: buckets_){
            for(auto& item: bucket->data_)
                res.insert(item);
        }
        return res;
    }
private:
    std::vector<std::unique_ptr<BucketType>> buckets_;
    Hash hasher_;

    BucketType& getBucket(const Key& key)const{
        auto index=hasher_(key)%buckets_.size();
        return *buckets_[index];
    }
};
#ifndef CHATSERVICE_LIB_CONNECTION_POOL_H
#define CHATSERVICE_LIB_CONNECTION_POOL_H

#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <functional>
#include <condition_variable>
#include <yaml-cpp/yaml.h>
#include <corpc/common/log.h>
#include <corpc/common/noncopyable.h>

namespace ChatService {

template <typename T>
class ConnectionPool : corpc::Noncopyable {
public:
    static ConnectionPool<T> *getInstance() {
        static ConnectionPool<T> pool;
        return &pool;
    }

    // 给外部提供的接口，返回可用的空闲连接
    std::shared_ptr<T> getConnection() {
        std::unique_lock<std::mutex> lock(queueMutex_);
        while (connectionQueue_.empty()) {
            if (cv_status::timeout == condition_.wait_for(lock, chrono::milliseconds(connectionTimeout_))) {
                if (connectionQueue_.empty()) {
                    USER_LOG_ERROR << "get connection timeout...";
                    return nullptr;
                }
            }
        }

        std::shared_ptr<T> ret(connectionQueue_.front(), [&](T *ptr) {
            std::unique_lock<std::mutex> lock(queueMutex_);
            ptr->refreshAliveTime();
            connectionQueue_.push(ptr);
        });
        connectionQueue_.pop();
        if (connectionQueue_.empty()) {
            // 没有可用连接了，通知生产线程去生产
            condition_.notify_all();
        }

        return ret;
    }

    void init(const YAML::Node &node) {
        if (started_) {
            USER_LOG_ERROR << "Connection pool has already started!";
            return;
        }

        // 初始化连接池
        initSize_ = node["init_pool_size"].as<int>();
        maxSize_ = node["max_pool_size"].as<int>();
        maxFreeTime_ = node["max_free_time"].as<int>();
        connectionTimeout_ = node["max_connect_timeout"].as<int>();

        // 初始化数据库
        T::loadConf(node);

        // 创建初始数量连接
        for (int i = 0; i < initSize_; i++) {
            T *p = new T();
            if (p->connect() == false) {
                USER_LOG_ERROR << "connect error!";
            }
            p->refreshAliveTime();
            connectionQueue_.push(p);
            connectionCnt_++;
        }

        // 启动一个新线程，作为连接生产者
        std::thread connect_producer(bind(&ConnectPool<T>::produceConnection, this));
        connect_producer.detach();

        // 启动新线程扫描空闲连接，超过max freetime时间的空闲连接，进行连接回收
        std::thread scanner(bind(&ConnectPool<T>::scanConnection, this));
        scaner.detach();

        started_ = 1;
    }

private:
    // 运行在独立的线程，专门负责生产新的连接
    void produceConnection() {
        while (true) {
            std::unique_lock<std::mutex> lock(queueMutex_);
            while (!connectionQueue_.empty()) {
                condition_.wait(lock);
            }

            // 还没到最大值，可创建
            if (connectionCnt_ < maxSize_) {
                T *p = new T();
                p->connect();
                p->refreshAliveTime(); // 刷新开始空闲的起始时间
                connectionQueue_.push(p);
                connectionCnt_++;
            }

            // 通知消费者线程可消费了
            condition_.notify_all();
        }
    }

    // 释放空闲连接
    void scanConnection() {
        while (true) {
            // 通过sleep模型定时效果
            this_thread::sleep_for(chrono::seconds(maxFreeTime));

            // 扫描队列，释放多余连接
            std::unique_lock<std::mutex> lock(queueMutex_);
            while (connectionCnt_ > initSize_)
            {
                T *p = connectionQueue_.front();
                if (p->getAliveTime() > maxFreeTime * 1000) {
                    connectionQueue_.pop();
                    delete p; //释放连接
                    connectionCnt_--;
                }
            }
        }
    }

private:
    int initSize_;          // 连接池初始连接量
    int maxSize_;           // 连接的最大连接量
    int maxFreeTime_;        // 连接池的最大空闲时间
    int connectionTimeout_; // 连接池获取连接的超时时间

    std::queue<T*> connectionQueue_; // 储存数据库连接的队列
    std::mutex queueMutex_; // 维护线程安全
    std::atomic<int> connectionCnt_; // 记录所创建的连接的总数量

    std::condition_variable condition_; // 设置条件变量，负责生产线程的唤醒和休眠

    int started_{0};
};

}

#endif
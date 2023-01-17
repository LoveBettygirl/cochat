#ifndef CHATSERVICE_UTILS_ID_GENERATOR_H
#define CHATSERVICE_UTILS_ID_GENERATOR_H

#include <string>
#include <ctime>
#include <corpc/common/noncopyable.h>

namespace ChatService {

class IDGenerator : corpc::Noncopyable {
public:
    static IDGenerator *getInstance() {
        static IDGenerator gen;
        return &gen;
    }

    uint64_t getUID(const std::string &info) {
        union {
            struct {
                uint64_t addId:16;
                uint64_t timestamp:32;
                uint64_t infoHash:15;
                uint64_t reserved:1;
            };
            uint64_t id;
        } idGen;

        uint32_t curSecond = time(nullptr);
        // 若秒数不同了，则自增ID重新从0开始
        if (curSecond != lastSecond_) {
            lastSecond_ = curSecond;
            addId_ = 0;
        }
        idGen.reserved = 0;
        idGen.infoHash = std::hash<std::string>()(info);
        idGen.timestamp = curSecond;
        idGen.addId = addId_++;
        return idGen.id;
    }
private:
    uint32_t lastSecond_;  // 上次产生ID时的时间戳，单位：秒
    uint32_t addId_;  // 自增ID
};

}

#endif
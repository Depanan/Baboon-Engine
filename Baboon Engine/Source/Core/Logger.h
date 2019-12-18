#pragma once
#include <queue>
#include <string>

#define PER_FRAME_LIMIT 20 //so we don't print too much stuff per frame


#define LOGINFO(s) ServiceLocator::GetLogger()->log(std::string("INFO: ") + s)
#define LOGERROR(s) ServiceLocator::GetLogger()->log(std::string("ERROR: ") + s)



//Using cout per thread can lead to issues hence we queue the messages using this class and we print them in a non threaded area 
class Logger
{
public:
    void log(const std::string& message);//queues an info message
    void process();//prints the queued messages


private:
    static std::queue<std::string> sm_message_queue;
};
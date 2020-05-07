#include "Logger.h"
#include <iostream>
std::queue<std::string> Logger::sm_message_queue;


void  Logger::log(const std::string& message)
{
   
    std::lock_guard<std::mutex> guard(m_LogMutex);
    sm_message_queue.push(message);
}

void Logger::process()
{
    int i = 0;
    while(!sm_message_queue.empty() && i < PER_FRAME_LIMIT){
        std::string s = sm_message_queue.front();
        std::cout << s << std::endl;
        sm_message_queue.pop();
        i++;
    }
}
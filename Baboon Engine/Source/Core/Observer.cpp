#include "Observer.h"
#include <cassert>

void Subject::Notify(int message, void* data)
{
    for (Observer* observer : m_Views)
    {
        observer->ObserverUpdate(message,data);
    }
}
void Subject::Register(Observer* observer)
{
    auto findResult = std::find(m_Views.begin(), m_Views.end(), observer);
    assert(findResult == m_Views.end(),"Cant add same observer twice");
    m_Views.push_back(observer);
}
void Subject::Unregister(Observer* toRemoveObserver)
{
    auto findResult = std::find(m_Views.begin(), m_Views.end()  , toRemoveObserver);
    if (findResult != m_Views.end())
    {
        std::swap(*findResult, *(--m_Views.end()));
        m_Views.pop_back();
    }
}
#pragma once
#include <vector>

class Subject;
class Observer
{
public:
    virtual ~Observer(){}
    virtual void ObserverUpdate(int message, void* data) = 0;
private:


};

class Subject
{
public:
    virtual ~Subject() {}
    virtual void Notify(int message, void* data = nullptr);
    void Register(Observer* observer);
    void Unregister(Observer* toRemoveObserver);
    enum Message { CAMERADIRTY,SCENELOADED, SCENEDIRTY,LIGHTDIRTY,MATERIALDIRTY};//TODO: Not sure if this is the best place to declare de messages think about it

private:

    std::vector<Observer*> m_Views;
};

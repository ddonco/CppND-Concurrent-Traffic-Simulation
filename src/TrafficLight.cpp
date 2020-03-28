#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait()
    // to wait for and receive new messages and pull them from the queue using move semantics.
    // The received object should then be returned by the receive function.

    // modify queue under lock
    std::unique_lock<std::mutex> uLock(_mutex);

    // pass unique lock to condition variable
    _cond.wait(uLock, [this] { return !_queue.empty(); });

    // remove last message from queue;
    T msg = std::move(_queue.back());
    _queue.pop_back();

    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex>
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    // lock mutex
    std::lock_guard<std::mutex> uLock(_mutex);

    // add message to queue
    _queue.push_back(std::move(msg));

    // notify client after adding new message to queue
    _cond.notify_one();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
    _phaseQueue = std::make_shared<MessageQueue<TrafficLightPhase>>();
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop
    // runs and repeatedly calls the receive function on the message queue.
    // Once it receives TrafficLightPhase::green, the method returns.

    while (true)
    {
        // get new phase from queue
        TrafficLightPhase phase = _phaseQueue->receive();

        // exit method when phase turns green
        if (phase == TrafficLightPhase::green)
            return;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles
    // and toggles the current phase of the traffic light between red and green and sends an update method
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds.
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.

    std::chrono::time_point<std::chrono::system_clock> currentTime, previousTime;

    // traffic light phase change update interval, a random duration between 4 and 6 seconds
    auto waitDuration = std::chrono::seconds((std::rand() % 6) + 4);
    while (true)
    {
        currentTime = std::chrono::system_clock::now();

        // Check that the update frequency time interval has elapsed before changing the traffic light phase.
        // Check that the difference beteen the current timestamp and the timestamp of the last update
        // is greater than the update frequency interval.
        if ((currentTime - previousTime) > waitDuration)
        {
            //change traffic light phase
            if (_currentPhase == TrafficLightPhase::red)
            {
                _currentPhase = TrafficLightPhase::green;
            }
            else
            {
                _currentPhase = TrafficLightPhase::red;
            }

            // add the new traffic light phase to the queue
            TrafficLightPhase msg = _currentPhase;
            std::future<void> ftr = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, _phaseQueue, std::move(msg));
            ftr.wait();

            // update timestamp of last phase change
            previousTime = std::chrono::system_clock::now();
            // pick new update frequeny interval
            waitDuration = std::chrono::seconds((std::rand() % 6) + 4);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
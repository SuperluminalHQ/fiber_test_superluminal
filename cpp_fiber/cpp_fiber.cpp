// cpp_fiber.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <assert.h>
#include <thread>
#include <vector>
#include <deque>
#include <string>
#include <atomic>
#include <mutex>
#include <stdlib.h>
#include <Superluminal/PerformanceAPI.h>
#include <format>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>



struct Fiber {
    ~Fiber() {
        test = 0;
    }

    bool valid() {
        return test == 0xfaaffaaf;
    }
    void* handle;
    int data;
    uint32_t test = 0xfaaffaaf;
};

class SpinLock {
    std::atomic_flag locked = ATOMIC_FLAG_INIT;
public:
    void lock() {
        while (locked.test_and_set(std::memory_order_acquire)) { ; }
    }
    void unlock() {
        locked.clear(std::memory_order_release);
    }
};

thread_local Fiber* gThreadFiber = nullptr;
thread_local Fiber* gCurrentFiber = nullptr;

SpinLock gStackSpinLock;
std::deque<Fiber*> gStack;


void threadfunc(int index) {
    PerformanceAPI::SetCurrentThreadName("Test - Work Thread");
    assert(gThreadFiber == nullptr);
    Fiber threadFiber;
    threadFiber.data = -1;
    threadFiber.handle = ConvertThreadToFiber(nullptr);
    gThreadFiber = &threadFiber;
    assert(gThreadFiber->valid());

    while (true) {
        assert(gCurrentFiber == nullptr);
        {
            std::lock_guard<SpinLock> lock(gStackSpinLock);
            if (gStack.size() == 0) {
                break;
            }
            assert(gStack.size() >= 0);
            gCurrentFiber = gStack.front();
            assert(gCurrentFiber != nullptr);
            assert(gCurrentFiber->valid());
            gStack.pop_front();
            assert(gCurrentFiber->valid());
            std::cout << "[Worker" << index << "] schedule fiber: " << gCurrentFiber->data << "\n";
        }

        SwitchToFiber(gCurrentFiber->handle);
        if (gCurrentFiber != nullptr) {
            std::lock_guard<SpinLock> lock(gStackSpinLock);
            gStack.push_back(gCurrentFiber);
            gCurrentFiber = nullptr;
        }
        Sleep(5);
    }

    ConvertFiberToThread();

}

int workload(int t) {
    int N = t * 1000 * 2;
    std::string d = std::format("N = {}", N);
    PERFORMANCEAPI_INSTRUMENT_DATA("func1", d.c_str());

    int r = 0;
    std::string data = std::format("N = {}", N);
    {
        PERFORMANCEAPI_INSTRUMENT_DATA("Workload - Part1", data.c_str());
        for (int i = 0; i < N; i++) {
            r += i * 11;
            r %= (r + 1);
        }
    }

    assert(gThreadFiber->valid());
    SwitchToFiber(gThreadFiber->handle);

    {
        PERFORMANCEAPI_INSTRUMENT_DATA("Workload - Part2", data.c_str());
        for (int i = 0; i < N; i++) {
            r += i * 11;
            r %= (r + 1);
        }
    }

    return r;
}

void func1(int a) {
    std::string d = std::format("a = {}", a);
    PERFORMANCEAPI_INSTRUMENT_DATA("func1", d.c_str());

    workload(1000);
    workload(200 + a);
    workload(300);
}

void func2(int a) {
    std::string d = std::format("a = {}", a);
    PERFORMANCEAPI_INSTRUMENT_DATA("func2", d.c_str());
    workload(900);
    workload(700 + a);
}

void func0(int a) {
    std::string d = std::format("a = {}", a);
    PERFORMANCEAPI_INSTRUMENT_DATA("func0", d.c_str());


    func1(a);
    workload(500);
    func2(2 * a);
}

void fiberFunc(void* data) {

    Fiber* f = (Fiber*)data;
    Fiber* f1 = (Fiber*)GetFiberData();
    std::string d = std::format("Fiber = {}", f->data);
    PERFORMANCEAPI_INSTRUMENT_DATA("FiberFunc", d.c_str());

    assert(f == f1);
    std::cout << "[Fiber] Start of fiber: " << f->data << "\n";

    int seed = rand() % 1000;
    {

    func0(100 + seed);
    }

    std::cout << "[Fiber] End of fiber: " << f->data << "\n";


    gCurrentFiber = nullptr;
    assert(gThreadFiber->valid());
    SwitchToFiber(gThreadFiber->handle);
}



int main()
{
    constexpr int N = 24;
    std::vector<Fiber> fibers;
    fibers.resize(N);
    PerformanceAPI::SetCurrentThreadName("Test - Main Thread");
    assert(gThreadFiber == nullptr);

    Fiber threadFiber;
    threadFiber.data = -1;
    threadFiber.handle =  ConvertThreadToFiber(nullptr);
    gThreadFiber = &threadFiber;
    assert(gThreadFiber->valid());

    {
        PERFORMANCEAPI_INSTRUMENT("Init Fiber");
        for (int i = 0; i < N; i++) {
            fibers[i].data = i;
            fibers[i].handle = CreateFiber(32 * 1024, fiberFunc, &fibers[i]);
            std::cout << "[Main] Create fiber: " << fibers[i].data << "\n";
        }
    }

    for (int i = 0; i < fibers.size(); i++) {
        gStack.push_back(&fibers[i]);
    }

    std::thread t1(threadfunc, 1);
    std::thread t2(threadfunc, 2);
    std::thread t3(threadfunc, 3);
    std::cout << "Hello World!\n";
    t1.join();
    t2.join();
    t3.join();

    {
        PERFORMANCEAPI_INSTRUMENT("Delete Fiber");

        for (int i = 0; i < fibers.size(); i++) {
            DeleteFiber(fibers[i].handle);
        }
    }

    ConvertFiberToThread();
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

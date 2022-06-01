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
#ifdef SUPERLUMINAL_ENABLED
#  include <Superluminal/PerformanceAPI.h>
#else
#  include <Tracy.hpp>
#endif
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
    char* name;
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

// threadfunc loop over the fiber queue and schedule fiber in
// first-in first-out manner. When a fiber return to the worker
// thread, if gCurrentFiber is not null, the fiber is push back
// to the queue and will be reschedule later.
void threadfunc(int index) {
//#  define ZoneNamedNS( varname, name, depth, active )
    ZoneScopedNS("Thread Func", 32);

    tracy::SetThreadName(std::format("Test - Work Thread[{}]", index).c_str());
    assert(gThreadFiber == nullptr);
    Fiber threadFiber;
    threadFiber.data = -1;
    threadFiber.name = new char[64];
    memset(threadFiber.name, 0, 64);
    std::format_to(threadFiber.name, "fiber worker[{}]", index);
    threadFiber.handle = ConvertThreadToFiber(nullptr);
    TracyFiberEnter(threadFiber.name);

    char* frame_name = new char[64];
    memset(frame_name, 0, 64);
    std::format_to(frame_name, "My Frame - {}", index);

    gThreadFiber = &threadFiber;
    assert(gThreadFiber->valid());

    while (true) {
        FrameMarkNamed(frame_name);
        ZoneScopedNS("Scheduler Loop", 32);

        assert(gCurrentFiber == nullptr);
        {
            ZoneScopedNS("Pop Fiber", 32);

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
        }

        {
            ZoneScopedNS("Schedule Job", 32);
            ZoneText(gCurrentFiber->name, 63);

            TracyFiberEnter(gCurrentFiber->name);
            SwitchToFiber(gCurrentFiber->handle);
        }

        if (gCurrentFiber != nullptr) {
            ZoneScopedNS("Push Fiber", 32);

            std::lock_guard<SpinLock> lock(gStackSpinLock);
            gStack.push_back(gCurrentFiber);
            gCurrentFiber = nullptr;
        }
    }

    TracyFiberLeave;
    ConvertFiberToThread();
}

// Dummy workload to do some long CPU work.
// The workload switch back to the worker thread in the middle
// so another fiber can be scheduled before this function finishes.
template<int D>
int workload(int t) {
    int N = t * 1000 * 2;
    std::string d = std::format("N = {}", N);
    ZoneScopedNS("workload", 32);
    ZoneText(d.c_str(), d.size());
    //PERFORMANCEAPI_INSTRUMENT_DATA("workload", d.c_str());

    int r = 0;
    std::string data = std::format("N = {}", N);
    {
        //PERFORMANCEAPI_INSTRUMENT_DATA("Workload - Part1", data.c_str());
        ZoneScopedNS("Workload - Part1", 32);
        ZoneText(data.c_str(), data.size());
        for (int i = 0; i < N; i++) {
            r += i * 11;
            r %= (r + 1);
        }
    }

    assert(gThreadFiber->valid());
    TracyFiberEnter(gThreadFiber->name);
    SwitchToFiber(gThreadFiber->handle); // Comment this SwitchToFiber to make Superluminal give
                                           // meaningful result

    {
        //PERFORMANCEAPI_INSTRUMENT_DATA("Workload - Part2", data.c_str());
        ZoneScopedNS("Workload - Part2", 32);
        ZoneText(data.c_str(), data.size());
        for (int i = 0; i < N; i++) {
            r += i * 11;
            r %= (r + 1);
        }
    }

    return r;
}

void func1(int a) {
    std::string d = std::format("a = {}", a);
    //PERFORMANCEAPI_INSTRUMENT_DATA("func1", d.c_str());
    ZoneScopedNS("func1", 32);
    ZoneText(d.c_str(), d.size());

    workload<11>(1000);
    workload<12>(200 + a);
    workload<13>(300);
}

void func2(int a) {
    std::string d = std::format("a = {}", a);
    //PERFORMANCEAPI_INSTRUMENT_DATA("func2", d.c_str());
    ZoneScopedNS("func1", 32);
    ZoneText(d.c_str(), d.size());
    workload<21>(900);
    workload<22>(700 + a);
}

template<int D>
void func0(int a, int depth) {
    std::string d = std::format("a = {}", a);
    //PERFORMANCEAPI_INSTRUMENT_DATA("func0", d.c_str());
    ZoneScopedNS("func0",32);
    ZoneText(d.c_str(), d.size());

    if (depth > 0) {
        func0<D>(a / 2, depth - 1);
    }

    if constexpr (D == 1) {
        a *= 2;
    }

    func1(a);
    workload<0>(500);
    func2(2 * a);
}

// The fiber func run func0 workload and return to the worker thread
// Inside func0 there are some switch back to the work thread, so
// the worker thread can schedule another fiber.
void fiberFunc(void* data) {
    Fiber* f = (Fiber*)data;
    Fiber* f1 = (Fiber*)GetFiberData();
    std::string d = std::format("Fiber = {}", f->data);
    //PERFORMANCEAPI_INSTRUMENT_DATA("FiberFunc", d.c_str());
    ZoneScopedNS("FiberFunc", 32);
    ZoneText(d.c_str(), d.size());

    assert(f == f1);
    int seed = rand() % 1000;
    {
        if ((f->data % 2) == 0) {
            std::cout << "[Fiber] Start mode 1 fiber: " << f->data << "\n";
            func0<1>(100 + seed, 2);
        }
        else {
            std::cout << "[Fiber] Start mode 2 fiber: " << f->data << "\n";
            func0<2>(100 + seed, 0);
        }
    }

    std::cout << "[Fiber] End of fiber: " << f->data << "\n";


    gCurrentFiber = nullptr; // convention to tell the worker thread not to reschedule the fiber
    assert(gThreadFiber->valid());

    TracyFiberEnter(gThreadFiber->name);
    SwitchToFiber(gThreadFiber->handle);
}



int main()
{
    TracyMessageL("Start App");
    ZoneScopedNS("Main", 32);

    constexpr int N = 24;
    std::vector<Fiber> fibers;
    fibers.resize(N);
    tracy::SetThreadName("Test - Main Thread");
    assert(gThreadFiber == nullptr);

    Fiber threadFiber;
    threadFiber.data = -1;
    threadFiber.handle =  ConvertThreadToFiber(nullptr);
    threadFiber.name = new char[64];
    memset(threadFiber.name, 0, 64);
    std::format_to(threadFiber.name, "main fiber");
    gThreadFiber = &threadFiber;
    assert(gThreadFiber->valid());

    {
        //PERFORMANCEAPI_INSTRUMENT("Init Fiber");
        ZoneScopedNS("Init Fiber", 32);
        for (int i = 0; i < N; i++) {
            fibers[i].data = i;
            fibers[i].handle = CreateFiber(32 * 1024, fiberFunc, &fibers[i]);
            fibers[i].name = new char[64];
            memset(fibers[i].name, 0, 64);
            std::format_to(fibers[i].name, "fiber[{}]", i);
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

    {
        ZoneScopedNS("Wait Worker Thread", 32);

        t1.join();
        t2.join();
        t3.join();
    }

    {
        //PERFORMANCEAPI_INSTRUMENT("Delete Fiber");
        ZoneScopedNS("Delete Fiber", 32);

        for (int i = 0; i < fibers.size(); i++) {
            DeleteFiber(fibers[i].handle);
        }
    }

    ConvertFiberToThread();

    TracyMessageL("Stop App");
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

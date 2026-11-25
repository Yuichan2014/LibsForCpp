#ifndef EASYTHREAD
#define EASYTHREAD

#include <iostream>
#include <functional>
#include <thread>
#include <utility>

const std::string INTRODUCTION = R"(
    EasyThread is a simple asynchronous thread wrapper that allows you to run a function in a separate thread and retrieve its result later.
    It is designed to be easy to use and understand, making it suitable for beginners and those who want to quickly implement multithreading in their applications.
    The class template takes a return type and a variable number of argument types, allowing you to run any function with any signature in a separate thread.
)";

// Simple asynchronous thread wrapper that allows you to run a function in a separate thread and retrieve its result later.
template <typename T, typename... Args>
class AsyncThreads {
public:
    // The result of the function executed in the thread
    T result{};

    // The function to be executed in the thread
    std::function<T(Args...)> func;

    // The thread object
    std::thread th;

    // Default constructor
    AsyncThreads() = default;

    // Constructor that takes a function to run in the thread
    explicit AsyncThreads(std::function<T(Args...)> f) : func(std::move(f)) {}

    // Start the thread with the provided arguments
    void start(Args... arg) {
        th = std::thread([this, arg...] { result = func(arg...); });
    }

    // Retrieve the result of the function executed in the thread
    T getValue() const {
        return this->result;
    }

    // Join the thread to wait for its completion
    void join() {
        if (th.joinable()) { th.join(); }
    }
};

#endif

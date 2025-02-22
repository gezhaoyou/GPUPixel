#pragma once
#include <functional>
#include <memory>

class dispatch_queue {
   public:
    dispatch_queue();
    ~dispatch_queue();

    void dispatch_async(std::function<void()> func);
    void dispatch_sync(std::function<void()> func);
    void dispatch_after(int msec, std::function<void()> func);
    void dispatch_flush();

    dispatch_queue(dispatch_queue const&) = delete;
    dispatch_queue& operator=(dispatch_queue const&) = delete;

   private:
    struct impl;
    std::unique_ptr<impl> m;
};


/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "thread_pool.h"

#include "restool_errors.h"
#include <iostream>
#include <string>

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;

ThreadPool::ThreadPool(size_t threadCount) : threadCount_(threadCount)
{}

uint32_t ThreadPool::Start()
{
    if (!workerThreads_.empty() || threadCount_ <= 0) {
        cerr << "Error: ThreadPool start failed." << endl;
        return RESTOOL_ERROR;
    }
    running_ = true;
    workerThreads_.reserve(threadCount_);
    for (size_t i = 0; i < threadCount_; ++i) {
        workerThreads_.emplace_back([this] { this->WorkInThread(); });
    }
    cout << "Info: thread pool is started" << endl;
    return RESTOOL_SUCCESS;
}

void ThreadPool::Stop()
{
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        running_ = false;
    }
    condition_.notify_all();
    for (std::thread &worker : workerThreads_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    cout << "Info: thread pool is stopped" << endl;
}


ThreadPool::~ThreadPool()
{
    if (running_) {
        Stop();
    }
}

void ThreadPool::WorkInThread()
{
    while (this->running_) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(this->queueMutex_);
            // wake up when there's a task or when the pool is stopped
            this->condition_.wait(lock, [this] { return !this->running_ || !this->tasks_.empty(); });
            if (!this->running_) {
                // exit thread when the pool is stopped
                return;
            }
            if (!this->tasks_.empty()) {
                task = std::move(this->tasks_.front());
                this->tasks_.pop();
            }
        }
        if (task) {
            task();
        }
    }
}
} // namespace Restool
} // namespace Global
} // namespace OHOS

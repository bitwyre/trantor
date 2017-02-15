#include <trantor/utils/ConcurrentTaskQueue.h>
#include <trantor/utils/Logger.h>
#include <assert.h>
#include <sys/prctl.h>
using namespace trantor;
ConcurrentTaskQueue::ConcurrentTaskQueue(size_t threadNum,const std::string &name):
queueCount_(threadNum),
queueName_(name),
stop_(false)
{
    assert(threadNum>0);
    for(unsigned int i=0;i<threadNum;i++)
    {
        threads_.push_back(std::thread(std::bind(&ConcurrentTaskQueue::queueFunc,this,i)));
    }
}
void ConcurrentTaskQueue::runTaskInQueue(const std::function<void ()> &task)
{
    std::lock_guard<std::mutex> lock(taskMutex_);
    taskQueue_.push(task);
    taskCond_.notify_one();
}

void ConcurrentTaskQueue::queueFunc(int queueNum) {
    char tmpName[32];
    snprintf(tmpName,sizeof(tmpName),"%s%d",queueName_.c_str(),queueNum);
    ::prctl(PR_SET_NAME,tmpName);
    while(!stop_)
    {
        std::function<void ()> r;
        {
            std::unique_lock<std::mutex> lock(taskMutex_);
            while (!stop_ && taskQueue_.size() == 0) {
                taskCond_.wait(lock);
            }
            if(taskQueue_.size()>0)
            {
                LOG_TRACE<<"got a new task!";
                r=std::move(taskQueue_.front());
                taskQueue_.pop();
            }
            else
                continue;
        }
        r();
    }
}

size_t ConcurrentTaskQueue::getTaskCount()
{
    std::lock_guard<std::mutex> guard(taskMutex_);
    return taskQueue_.size();
}
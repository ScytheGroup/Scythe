module;
#include <Common/Macros.hpp>
module Tools;

import :JobScheduler;

import <thread>;

Job::Job(const JobFunction& function, uint32_t id, bool unmanaged, const JobFunction& onJobFinishedFunction)
    : function(function)
    , id(id)
    , isUnmanaged(unmanaged)
    , onJobFinished(onJobFinishedFunction)
{
    Assert(function != nullptr, "Attempt to create a null job");
}

void Job::Execute()
{
    std::unique_lock<spinlock> lock(stateMut);
    state = RUNNING;
    lock.unlock();
    function();
    lock.lock();
    state = FINISHED;
    OnJobFinished();
}

void Job::IncrementHandles()
{
    ++numHandles;
}

size_t Job::DecrementHandles()
{
    return numHandles.fetch_sub(1) - 1;
}

void Job::OnJobFinished()
{
    if (barrier)
        barrier->OnJobFinished(this);
    if (onJobFinished)
        onJobFinished();
}

void Job::IncrementDependencies()
{
    if (state == PENDING)
        ++dependencies;
}

void Job::DecrementDependencies()
{
    std::unique_lock lock(stateMut);
    uint32_t deps = dependencies.fetch_sub(1);

    if (!isUnmanaged && state == PENDING && deps - 1 == 0u)
    {
        DebugPrint("Job {} dependencies done and is now being queued", GetId());
        JobScheduler::Get().QueueJob({ this });
    }
};

JobHandle::JobHandle(Job* job)
    : Handle(job->GetId())
    , jobRef(job)
{
    job->IncrementHandles();
}

JobHandle::~JobHandle()
{
    if (jobRef)
    {
        size_t handlesLeft = jobRef->DecrementHandles();
        {
            std::unique_lock lock(jobRef->stateMut);
            if (jobRef->ShouldBeDeleted() && !handlesLeft)
                delete jobRef;
        }
    }
}

JobHandle::JobHandle(const JobHandle& other)
    : Handle(other)
    , jobRef(other.jobRef)
{
    jobRef->IncrementHandles();
}

JobHandle& JobHandle::operator=(const JobHandle& other)
{
    if (this == &other)
        return *this;
    Handle::operator=(other);
    jobRef = other.jobRef;
    jobRef->IncrementHandles();
    return *this;
}

JobHandle::JobHandle(JobHandle&& other) noexcept
    : Handle(other)
{
    std::swap(jobRef, other.jobRef);
}

JobHandle& JobHandle::operator=(JobHandle&& other) noexcept
{
    if (this == &other)
        return *this;
    Handle::operator=(other);
    std::swap(jobRef, other.jobRef);
    return *this;
}

void JobHandle::Wait()
{
    while (jobRef->GetState() == Job::RUNNING)
    {
        std::this_thread::yield();
    }
}

JobScheduler::JobScheduler()
    : jobs()
{
    threads.reserve(GetMaxConcurrency());
    for (uint32_t i = 0; i < GetMaxConcurrency(); ++i)
    {
        threads.emplace_back([] { JobScheduler::Get().MainFunction(); });
    }
}

JobScheduler::~JobScheduler()
{
    run = false;
    cvJobAdded.notify_all();
}

JobHandle JobScheduler::CreateUnmanagedJob(JobFunction function, uint32_t numDeps, JobFunction onJobFinished)
{
    static uint32_t id = 0;
    auto handle = JobHandle(new Job{ function, id++, true, onJobFinished });
    handle->dependencies = numDeps;

    return handle;
}

JobHandle JobScheduler::CreateJob(JobFunction function, uint32_t numDeps, JobFunction onJobFinished)
{
    static uint32_t id = 0;
    auto handle = JobHandle(new Job{ function, id++, false, onJobFinished });
    handle->dependencies = numDeps;
    if (numDeps == 0)
    {
        QueueJob(handle);
    }

    return handle;
}

void JobScheduler::Update()
{}

void JobScheduler::QueueJob(JobHandle job)
{
    std::unique_lock<spinlock> lock(cvLock);
    jobs.push_back({ job });
    cvJobAdded.notify_one();
}

void JobScheduler::QueueJobs(Array<JobHandle> newJobs)
{
    std::unique_lock<spinlock> lock(cvLock);
    jobs.insert(jobs.end(), newJobs.begin(), newJobs.end());
    cvJobAdded.notify_one();
}

// This constant represents the number of reserved threads for major systems and os
static constexpr uint32_t numReservedThreads = 3;

uint32_t JobScheduler::GetMaxConcurrency()
{
    return std::max(1u, std::jthread::hardware_concurrency() - numReservedThreads);
}

void JobScheduler::MainFunction()
{
    static std::atomic_int32_t threadCount = 0;
    size_t threadId = threadCount++;
    while (run)
    {
        JobHandle job;

        do
        {
            std::unique_lock<spinlock> lock(cvLock);
            cvJobAdded.wait(lock, [&] { return !jobs.empty() || !run; });
            if (!run && jobs.empty())
                return;

            job = jobs.front().GetPtr();
            jobs.pop_front();
        } while (!job.IsValid());

        job->Execute();
    }
}

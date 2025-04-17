module;
#include "PhysicsSystem-Headers.hpp"
#include <Jolt/Core/JobSystem.h>

module Systems.Physics;

spinlock JobSystemWrapper::maplock;

JobSystemWrapper::JobSystemWrapper() : jobScheduler(JobScheduler::Get())
{}

int JobSystemWrapper::GetMaxConcurrency() const
{
    return jobScheduler.GetMaxConcurrency();
}

JPH::JobHandle JobSystemWrapper::CreateJob(const char* inName, JPH::ColorArg inColor, const JobFunction& inJobFunction, uint32_t inNumDependencies)
{
    JobSystemWrapper::Job* jphJob = new InternalJob(inName, inColor, this, inJobFunction, inNumDependencies);
    ::JobHandle jobHandle = jobScheduler.CreateUnmanagedJob(inJobFunction, inNumDependencies);

    maplock.lock();
    jphMapPtrToJobScytheJobHandle[jphJob] = jobHandle;
    maplock.unlock();
    
    if (inNumDependencies == 0)
        QueueJob(jphJob);
    
    return JobHandle{jphJob};
}

JPH::JobSystem::Barrier* JobSystemWrapper::CreateBarrier()
{
    return new BarrierImpl();
}

void JobSystemWrapper::DestroyBarrier(Barrier* inBarrier)
{
    delete static_cast<BarrierImpl*>(inBarrier);
}

void JobSystemWrapper::WaitForJobs(Barrier* inBarrier)
{
    static_cast<BarrierImpl*>(inBarrier)->Wait();
}

void JobSystemWrapper::QueueJob(Job* inJob)
{
    std::unique_lock<spinlock> lock(maplock);
    auto& jobHandle = jphMapPtrToJobScytheJobHandle.at(inJob);
    jobScheduler.QueueJob(jobHandle);
}

void JobSystemWrapper::QueueJobs(Job** inJobs, JPH::uint inNumJobs)
{
    std::unique_lock<spinlock> lock(maplock);
    Array<::JobHandle> jobs;
    jobs.reserve(inNumJobs);
    for (JPH::uint i = 0; i < inNumJobs; i++)
    {
        auto& jobHandle = jphMapPtrToJobScytheJobHandle[inJobs[i]];
        jobs.push_back(jobHandle);
    }
    jobScheduler.QueueJobs(jobs);
}

void JobSystemWrapper::FreeJob(Job* inJob)
{
    std::unique_lock<spinlock> lock(maplock);
    jphMapPtrToJobScytheJobHandle.erase(inJob);
    delete inJob;
}

// BarriersImpl
void JobSystemWrapper::BarrierImpl::AddJob(const JobHandle& inJob)
{
    std::unique_lock<spinlock> lock(maplock);
    if (jphMapPtrToJobScytheJobHandle.contains(inJob.GetPtr()))
        internalBarrier.AddJob(jphMapPtrToJobScytheJobHandle[inJob.GetPtr()]);
}

void JobSystemWrapper::BarrierImpl::AddJobs(const JobHandle* inHandles, JPH::uint inNumHandles)
{
    std::unique_lock<spinlock> lock(maplock);
    Array<::JobHandle> jobs;
    jobs.reserve(inNumHandles);
    for (JPH::uint i = 0; i < inNumHandles; i++)
    {
        if (jphMapPtrToJobScytheJobHandle.contains(inHandles[i].GetPtr()))
            jobs.push_back(jphMapPtrToJobScytheJobHandle.at(inHandles[i].GetPtr()));
    }
    internalBarrier.AddJobs(jobs);
}

void JobSystemWrapper::BarrierImpl::OnJobFinished(Job* inJob)
{
}

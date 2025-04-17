module;

#include "PhysicsSystem-Headers.hpp"
#include <Jolt/Core/JobSystem.h>

export module Systems.Physics:JobSystemWrapper;

import Common;
import Systems;
import Tools;

export class JobSystemWrapper
    : public JPH::JobSystem
{
    JobScheduler& jobScheduler;

    using InternalJob = JobSystemWrapper::Job;
    static std::map<Job*, ::JobHandle> jphMapPtrToJobScytheJobHandle;
    static spinlock maplock;    
public:
    JobSystemWrapper();
    int GetMaxConcurrency() const override;
    JobHandle CreateJob(const char* inName, JPH::ColorArg inColor, const JobFunction& inJobFunction, uint32_t inNumDependencies) override;
    Barrier* CreateBarrier() override;
    void DestroyBarrier(Barrier* inBarrier) override;
    void WaitForJobs(Barrier* inBarrier) override;

protected:
    void QueueJob(Job* inJob) override;
    void QueueJobs(Job** inJobs, uint32_t inNumJobs) override;
    void FreeJob(Job* inJob) override;
    
private:
    class BarrierImpl : public Barrier
    {
        ::Barrier internalBarrier;

    public:
        JPH_OVERRIDE_NEW_DELETE

        // Constructor
        BarrierImpl() = default;
        virtual ~BarrierImpl() override = default;

        // See Barrier
        virtual void AddJob(const JobHandle& inJob) override;
        virtual void AddJobs(const JobHandle* inHandles, uint32_t inNumHandles) override;

        // Check if there are any jobs in the job barrier
        inline bool IsEmpty() const { return internalBarrier.IsEmpty(); };

        // Wait for all jobs in this job barrier, while waiting, execute jobs that are part of this barrier on the current thread
        void Wait() { internalBarrier.Wait(); };

        // Flag to indicate if a barrier has been handed out
        std::atomic<bool> mInUse{ false };

    protected:
        // Called by a Job to mark that it is finished
        void OnJobFinished(Job* inJob) override;
    };
};

export std::map<JobSystemWrapper::Job*, ::JobHandle> JobSystemWrapper::jphMapPtrToJobScytheJobHandle;
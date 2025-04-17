export module Tools:JobScheduler;
import Common;

import <condition_variable>;

export
{

using JobFunction = std::function<void()>;

class JobScheduler;
class JobHandle;
class Barrier;

class Job final
{
    friend JobScheduler;
    friend JobHandle;
    friend Barrier;

    std::atomic<uint32_t> id;
    std::atomic<uint32_t> dependencies = 0;
    std::atomic<uint32_t> numHandles = 0;
    JobFunction function;
    JobFunction onJobFinished = nullptr;
    std::thread::id threadIndex;
    Barrier* barrier = nullptr;
    bool isUnmanaged;

    spinlock stateMut;

    Job() = delete;

protected:
    Job(const JobFunction& function, uint32_t id, bool unmanaged,
        const JobFunction& onJobFinishedFunction = nullptr);
    ~Job() = default;

    void Execute();

    enum : uint8_t
    {
        PENDING,
        QUEUED,
        RUNNING,
        FINISHED
    } state = PENDING;

    bool SetBarrier(Barrier* newBarrier)
    {
        std::unique_lock<spinlock> lock(stateMut);
        if (state == FINISHED || newBarrier == nullptr)
            return false;
        barrier = newBarrier;
        return true;
    }

    void IncrementHandles();
    size_t DecrementHandles();

    void OnJobFinished();

    bool ShouldBeDeleted() { return numHandles == 0u; }

public:
    uint32_t GetId() const { return id; }

    auto GetState() const { return state; }

    void IncrementDependencies();
    void DecrementDependencies();
};

class JobHandle final : public Handle
{
    Job* jobRef = nullptr;
    friend JobScheduler;

public:
    JobHandle(Job* job);
    JobHandle()
        : Handle(std::numeric_limits<uint32_t>::max())
    {}

    ~JobHandle();

    JobHandle(const JobHandle& other);
    JobHandle& operator=(const JobHandle& other);
    JobHandle(JobHandle&& other) noexcept;
    JobHandle& operator=(JobHandle&& other) noexcept;

    void Wait();

    Job* operator->() const noexcept { return GetPtr(); }

    Job* GetPtr() const { return jobRef; }

    bool IsValid() const { return jobRef != nullptr; }
};

class JobScheduler final : public InstancedSingleton<JobScheduler>
{
    std::deque<JobHandle> jobs;
    Array<std::jthread> threads;

    spinlock cvLock;
    std::condition_variable_any cvJobAdded;
    std::atomic_bool run = true;

protected:
    friend class EntityComponentSystem;
    friend class JobSystemWrapper;
    friend Job;
    void QueueJob(JobHandle job);
    void QueueJobs(Array<JobHandle> jobs);
    void MainFunction();

    JobHandle CreateUnmanagedJob(JobFunction function, uint32_t numDeps = 0, JobFunction onJobFinished = nullptr);

public:
    JobScheduler();
    ~JobScheduler();
    static uint32_t GetMaxConcurrency();

    JobHandle CreateJob(JobFunction function, uint32_t numDeps = 0, JobFunction onJobFinished = nullptr);

    void Update();

    // Executes a delegate by creating jobs for all function calls, permitting parallel resolution
    template <class... Targs>
    static Barrier&& ExecuteDelegateAsync(Delegate<Targs...>& delegate, Targs&&... args);
};

// Allows waiting on multiple jobs to finish
class Barrier
{
    std::atomic<int32_t> count = 0;

    std::condition_variable_any cv;
    spinlock spin;

public:
    Barrier() = default;

    template <class... TJobs>
        requires std::is_base_of_v<JobHandle, TJobs...>
    Barrier(TJobs... jobs)
        : count(sizeof...(jobs))
    {
        (AddJob(jobs), ...);
    }

    ~Barrier()
    {
        if (count != 0u)
            Wait();
    };

    void AddJob(JobHandle job)
    {
        if (job->SetBarrier(this))
            ++count;
    }

    void AddJobs(Array<JobHandle> jobs)
    {
        for (auto& j : jobs)
            AddJob(j);
    }

    void OnJobFinished(Job* job)
    {
        --count;
        if (count == 0u)
            cv.notify_all();
    }

    void Wait()
    {
        std::unique_lock<spinlock> lock(spin);
        int32_t val = count;
        while (val != 0u)
        {
            cv.wait_for(lock, std::chrono::milliseconds(100), [this] { return count == 0u; });
            val = count;
        }
    }

    bool IsEmpty() const { return count == 0u; }
};

template <class... Targs>
Barrier&& JobScheduler::ExecuteDelegateAsync(Delegate<Targs...>& delegate, Targs&&... args)
{
    Barrier barrier;
    barrier.AddJob(Get().CreateJob([&]() { delegate.Execute(std::forward<Targs>(args)...); }));
    return std::move(barrier);
}
}

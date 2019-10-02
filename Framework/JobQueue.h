#pragma once

#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

// ========================================================
// class JobQueue
// ========================================================

class JobQueue final
{
public:
	typedef std::function<void()> Job;

	// Wait for the worker thread to exit.
	~JobQueue()
	{
		if (worker.joinable())
		{
			waitAll();
			mutex.lock();
			terminating = true;
			condition.notify_one();
			mutex.unlock();
			worker.join();
		}
	}

	// Launch the worker thread.
	void launch()
	{
		ASSERT(!worker.joinable()); // Not already launched!
		worker = std::thread(&JobQueue::queueLoop, this);
	}

	// Add a new job to the thread's queue.
	void pushJob(Job job)
	{
		std::lock_guard<std::mutex> lock(mutex);
		queue.push(std::move(job));
		condition.notify_one();
	}

	// Wait until all work items have been completed.
	void waitAll()
	{
		std::unique_lock<std::mutex> lock(mutex);
		condition.wait(lock, [this]() { return queue.empty(); });
	}

private:
	void queueLoop()
	{
		for (;;)
		{
			Job job;
			{
				std::unique_lock<std::mutex> lock(mutex);
				condition.wait(lock, [this] { return !queue.empty() || terminating; });
				if (terminating)
				{
					break;
				}
				job = queue.front();
			}

			job();

			{
				std::lock_guard<std::mutex> lock(mutex);
				queue.pop();
				condition.notify_one();
			}
		}
	}

	bool terminating = false;

	std::thread worker;
	std::queue<Job> queue;
	std::mutex mutex;
	std::condition_variable condition;
};


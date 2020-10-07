#include "ModuleTaskManager.h"
#include <thread>


void ModuleTaskManager::threadMain()
{
	while (true)
	{
		// TODO 3:

		// - Wait for new tasks to arrive
		
		std::unique_lock<std::mutex> lock(mtx);

		if (!scheduledTasks.empty())
		{
			// - Retrieve a task from scheduledTasks
			Task* task = scheduledTasks.front();
			scheduledTasks.pop();

			// - Execute it
			task->execute();
				
			// - Insert it into finishedTasks
			finishedTasks.push(task);
		}

	}
}

bool ModuleTaskManager::init()
{
	// TODO 1: Create threads (they have to execute threadMain())
	for (int i = 0; i < MAX_THREADS; ++i)
	{
		threads[i] = std::thread(threadMain);
	}

	return true;
}

bool ModuleTaskManager::update()
{
	// TODO 4: Dispatch all finished tasks to their owner module (use Module::onTaskFinished() callback)
	
	if (!finishedTasks.empty())
	{
		Task* completedTask = finishedTasks.front();
		finishedTasks.pop();

		completedTask->owner->onTaskFinished(completedTask);
	}

	return true;
}

bool ModuleTaskManager::cleanUp()
{
	// TODO 5: Notify all threads to finish and join them

	for (int i = 0; i < MAX_THREADS; ++i)
	{
		threads[i].join();
	}

	return true;
}

void ModuleTaskManager::scheduleTask(Task *task, Module *owner)
{
	task->owner = owner;

	// TODO 2: Insert the task into scheduledTasks so it is executed by some thread
	scheduledTasks.push(task);
}

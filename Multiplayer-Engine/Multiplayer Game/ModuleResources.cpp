#include "Networks.h"


#if defined(USE_TASK_MANAGER)

void ModuleResources::TaskLoadTexture::execute()
{
	*texture = App->modTextures->loadTexture(filename);
}

#endif


bool ModuleResources::init()
{
	background = App->modTextures->loadTexture("background.jpg");

#if !defined(USE_TASK_MANAGER)
	
	loadingFinished = true;
	completionRatio = 1.0f;
#else
	loadTextureAsync("sunsetr_background2.png", &tex_sunset_background);
	loadTextureAsync("crosshairs_v01.png", &tex_crosshairs_ss);
	loadTextureAsync("cowboywindow2.png", &tex_cowboy_window);
	
	loadTextureAsync("explosion1.png",       &explosion1);
	loadTextureAsync("blood.png", &blood);

#endif

	audioClipShot = App->modSound->loadAudioClip("gun_shot.wav");
	audioClipDoorOpen = App->modSound->loadAudioClip("door_open.wav");
	audioClipDoorClose = App->modSound->loadAudioClip("door_close.wav");
	audioClipExplosion = App->modSound->loadAudioClip("explosion.wav");
	audioClipManHit = App->modSound->loadAudioClip("man_hit.wav");
	audioClipWomanHit = App->modSound->loadAudioClip("woman_hit.wav");
	audioClipMexicanHit = App->modSound->loadAudioClip("mexican_hit.wav");
	//App->modSound->playAudioClip(audioClipExplosion);

	return true;
}

#if defined(USE_TASK_MANAGER)

void ModuleResources::loadTextureAsync(const char * filename, Texture **texturePtrAddress)
{
	ASSERT(taskCount < MAX_RESOURCES);
	
	TaskLoadTexture *task = &tasks[taskCount++];
	task->owner = this;
	task->filename = filename;
	task->texture = texturePtrAddress;

	App->modTaskManager->scheduleTask(task, this);
}

void ModuleResources::onTaskFinished(Task * task)
{
	ASSERT(task != nullptr);

	TaskLoadTexture *taskLoadTexture = dynamic_cast<TaskLoadTexture*>(task);

	for (uint32 i = 0; i < taskCount; ++i)
	{
		if (task == &tasks[i])
		{
			finishedTaskCount++;
			task = nullptr;
			break;
		}
	}

	ASSERT(task == nullptr);

	if (finishedTaskCount == taskCount)
	{
		finishedLoading = true;

		// Create the explosion animation clip
		explosionClip = App->modRender->addAnimationClip();
		explosionClip->frameTime = 0.1f;
		explosionClip->loop = false;
		for (int i = 0; i < 16; ++i)
		{
			float x = (i % 4) / 4.0f;
			float y = (i / 4) / 4.0f;
			float w = 1.0f / 4.0f;
			float h = 1.0f / 4.0f;
			explosionClip->addFrameRect(vec4{ x, y, w, h });
		}

		bloodSplash = App->modRender->addAnimationClip();
		bloodSplash->frameTime = 0.1f;
		bloodSplash->loop = false;
		for (int i = 0; i < 16; ++i)
		{
			float x = (i % 4) / 4.0f;
			float y = (i / 4) / 4.0f;
			float w = 1.0f / 4.0f;
			float h = 1.0f / 4.0f;
			bloodSplash->addFrameRect(vec4{ x, y, w, h });
		}
	}
}

#endif

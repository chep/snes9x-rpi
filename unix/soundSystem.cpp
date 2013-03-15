#include <iostream>
#include <fstream>

#include "soundSystem.hpp"
#include "port.h"
#include "soundux.h"

static const unsigned int audioFrequencies[8] =
{0, 8192, 11025, 16000, 22050, 29300, 36600, 44100};

static const unsigned int audioBufferSizes [8] =
{ 0, 256, 256, 256, 512, 512, 1024, 1024};

SoundSystem::SoundSystem(unsigned int mode, std::string device):
	playback_handle(NULL),
	audioBuffer(NULL),
	threadProcess(NULL)
{
	int err;
	snd_pcm_hw_params_t *hw_params;
	unsigned int frequency;

	if (mode >= sizeof(audioFrequencies))
		mode = sizeof(audioFrequencies) - 1;
	frequency = audioFrequencies[mode];
	bufferSize = audioBufferSizes[mode];

	err = snd_pcm_open(&playback_handle,
	                   device.c_str(),
	                   SND_PCM_STREAM_PLAYBACK,
	                   0);
	if(err < 0)
	{
		std::cerr<<"Cannot open audio device "<<device<<": "<<snd_strerror (err)<<std::endl;
		throw -1;
	}


	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0)
	{
		std::cerr<<"Cannot allocate hardware parameter structure: "<<snd_strerror(err)<<std::endl;
		throw -1;
	}
				 
	if ((err = snd_pcm_hw_params_any(playback_handle, hw_params)) < 0)
	{
		std::cerr<<"Cannot initialize hardware parameter structure: "<<snd_strerror(err)<<std::endl;
		goto error;
	}
	
	err = snd_pcm_hw_params_set_access(playback_handle,
	                                   hw_params,
	                                   SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0)
	{
		std::cerr<<"Cannot set access type: "<<snd_strerror(err)<<std::endl;
		goto error;
	}
	
	err = snd_pcm_hw_params_set_format(playback_handle,
	                                   hw_params,
	                                   SND_PCM_FORMAT_S16_LE);
	if (err < 0)
	{
		std::cerr<<"Cannot set sample format: "<<snd_strerror(err)<<std::endl;
		goto error;
	}
	
	err = snd_pcm_hw_params_set_rate_near(playback_handle, hw_params, &frequency, 0);
	if (err < 0)
	{
		std::cerr<<"Cannot set sample rate: "<<snd_strerror (err)<<std::endl;
		goto error;
	}
	
	err = snd_pcm_hw_params_set_channels(playback_handle,
	                                     hw_params,
	                                     2);
	if (err < 0)
	{
		std::cerr<<"Cannot set channel count: "<<snd_strerror(err)<<std::endl;
		goto error;
	}
	
	if ((err = snd_pcm_hw_params(playback_handle, hw_params)) < 0)
	{
		std::cerr<<"Cannot set parameters: "<<snd_strerror(err)<<std::endl;
		goto error;
	}
	
	if ((err = snd_pcm_prepare(playback_handle)) < 0)
	{
		std::cerr<<"Cannot prepare audio interface for use: "<<snd_strerror(err)<<std::endl;
		goto error;
	}

	snd_pcm_hw_params_free (hw_params);

	audioBuffer = new boost::int16_t[bufferSize];
	if (!audioBuffer)
	{
		std::cerr<<"Unable to allocate audio buffer."<<std::endl;
		throw -1;
	}
	std::memset(audioBuffer, 0, bufferSize);

	threadProcess = new boost::thread(boost::bind(&SoundSystem::processSound, this));

	return;

error:
	snd_pcm_hw_params_free (hw_params);
	throw -1;
}

SoundSystem::~SoundSystem()
{
	if (threadProcess)
		delete threadProcess;

	if (playback_handle)
		snd_pcm_close(playback_handle);

	if (audioBuffer)
		delete [] audioBuffer;
}


void SoundSystem::processSound()
{	
	while (true)
	{
		S9xMixSamplesO (audioBuffer, bufferSize,
		                0);
		snd_pcm_sframes_t err = snd_pcm_writei(playback_handle,
		                                       audioBuffer,
		                                       bufferSize/2);
	}
}

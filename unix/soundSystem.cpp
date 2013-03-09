#include <iostream>

#include "soundSystem.hpp"
#include "port.h"
#include "soundux.h"

#define AUDIO_BUFFER_SIZE (1024 * 16)

static const unsigned int audioFrequencies[8] =
{0, 8192, 11025, 16000, 22050, 29300, 36600, 44100};

static const unsigned int audioBufferSizes [8] =
{ 0, 256, 256, 256, 512, 512, 1024, 1024};

SoundSystem::SoundSystem(unsigned int mode, std::string device):
	playback_handle(NULL),
	audioBuffer(NULL),
	mixedSamples(0),
	threadProcess(NULL)
{
	int err;
	snd_pcm_hw_params_t *hw_params;
	unsigned int frequency;

	device = "plughw:0,0";
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

	audioBuffer = new char[AUDIO_BUFFER_SIZE];
	if (!audioBuffer)
	{
		std::cerr<<"Unable to allocate audio buffer."<<std::endl;
		throw -1;
	}
	std::memset(audioBuffer, 0, AUDIO_BUFFER_SIZE);

	threadProcess = new boost::thread(boost::bind(&SoundSystem::processSound, this));

	return;

error:
	snd_pcm_hw_params_free (hw_params);
	throw -1;
}

SoundSystem::~SoundSystem()
{
	if (playback_handle)
		snd_pcm_close(playback_handle);

	if (audioBuffer)
		delete [] audioBuffer;

	if (threadProcess)
		delete threadProcess;
}


void SoundSystem::processSound()
{	
	static uint8 Buf[MAX_BUFFER_SIZE] __attribute__((aligned(4)));
// static uint8 buffer[AUDIO_BUFFER_SIZE];

	// unsigned int nbytesToWrite(0);
	// unsigned int position(0);
	//do
	{
		// S9xMixSamplesO (Buf, so.buffer_size/2,
		//                 0);
		// write (so.sound_fd, (char *) Buf,
		//        so.buffer_size);

		// int sample_count = so.buffer_size;
		// int byte_offset;

		// sample_count >>= 1;
 

		// if (so.samples_mixed_so_far < sample_count)
		// {
		// 	byte_offset = so.play_position + (so.samples_mixed_so_far << 1);

		// 		S9xMixSamplesO (Buf, sample_count - so.samples_mixed_so_far,
		// 		                byte_offset & SOUND_BUFFER_SIZE_MASK);
		// 	so.samples_mixed_so_far = 0;
		// }
		// else
		// 	so.samples_mixed_so_far -= sample_count;
    
		// {
		// 	int I;
		// 	int J = so.buffer_size;

		// 	byte_offset = so.play_position;
		// 	so.play_position = (so.play_position + so.buffer_size) & SOUND_BUFFER_SIZE_MASK;

		// 	do
		// 	{
		// 		if (byte_offset + J > SOUND_BUFFER_SIZE)
		// 		{
		// 			I = write (so.sound_fd, (char *) Buf + byte_offset,
		// 			           SOUND_BUFFER_SIZE - byte_offset);
		// 			if (I > 0)
		// 			{
		// 				J -= I;
		// 				byte_offset = (byte_offset + I) & SOUND_BUFFER_SIZE_MASK;
		// 			}
		// 		}
		// 		else
		// 		{
		// 			I = write (so.sound_fd, (char *) Buf + byte_offset, J);
		// 			if (I > 0)
		// 			{
		// 				J -= I;
		// 				byte_offset = (byte_offset + I) & SOUND_BUFFER_SIZE_MASK;
		// 			}
		// 		}
		// 	} while ((I < 0 && errno == EINTR) || J > 0);
		// }

//	} while (1);
	}
	while (true)
	{
		//void *bufs[2] = {Buf, Buf + so.buffer_size/2};
		S9xMixSamplesO (Buf, so.buffer_size,
		                0);
		snd_pcm_writei(playback_handle,
		               Buf,
		               so.buffer_size);

		//write buffer:
		// while (nbytesToWrite)
		// {
		// 	snd_pcm_sframes_t nbytesWritten(0);
		// 	if (position + nbytesToWrite > AUDIO_BUFFER_SIZE)
		// 		nbytesWritten = snd_pcm_writei(playback_handle,
		// 		                               audioBuffer + position,
		// 		                               AUDIO_BUFFER_SIZE - nbytesToWrite);
		// 	else
		// 		nbytesWritten = snd_pcm_writei(playback_handle,
		// 		                               audioBuffer + position,
		// 		                               nbytesToWrite);
		// 	if (nbytesWritten > 0)
		// 	{
		// 		nbytesToWrite -= nbytesWritten;
		// 		position = (position + nbytesWritten) % AUDIO_BUFFER_SIZE;
		// 	}
		// 	else
		// 	{
		// 		std::cerr<<"Unable to write to alsa device"<<std::endl;
		// 		throw -1;
		// 	}
		// }
	}
}



// void SoundSystem::generateSound()
// {
// 	boost::mutex::scoped_lock lock(mutex, boost::try_to_lock);
// 	if (!lock)
// 		return;

// 	unsigned generatedBytes(mixedSamples * 2);
// 	if (generatedBytes >= bufferSize)
// 		return;
// }


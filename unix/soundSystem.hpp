#ifndef _SOUNDSYSTEM_HPP_
#define _SOUNDSYSTEM_HPP_
#include <alsa/asoundlib.h>
#include <boost/thread.hpp>

class SoundSystem
{
public:
	SoundSystem(unsigned int mode = 7,
	            std::string device = "default");
	~SoundSystem(void);


public:
	void processSound(void);


private:
	snd_pcm_t *playback_handle;
	char *audioBuffer;
	unsigned bufferSize;
	
	unsigned mixedSamples;

	boost::thread *threadProcess;
};

#endif

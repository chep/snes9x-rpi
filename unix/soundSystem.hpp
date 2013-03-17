#ifndef _SOUNDSYSTEM_HPP_
#define _SOUNDSYSTEM_HPP_
#include <alsa/asoundlib.h>
#include <boost/thread.hpp>
#include <boost/cstdint.hpp>

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
	boost::int16_t *audioBuffer;
	unsigned bufferSize;
	
	boost::thread *threadProcess;
};

#endif

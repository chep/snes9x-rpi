#ifndef _SOUNDSYSTEM_HPP_
#define _SOUNDSYSTEM_HPP_

static const unsigned int audioFrequencies[8] =
{0, 8192, 11025, 16000, 22050, 29300, 36600, 44100};

static const unsigned int audioBufferSizes [8] =
{ 0, 256, 256, 256, 512, 512, 1024, 1024};

class SoundSystem
{
public:
	SoundSystem() {}
	virtual ~SoundSystem(void) {}

};


#endif

#ifndef _PULSESOUNDSYSTEM_HPP_
#define _PULSESOUNDSYSTEM_HPP_

#include <thread>
#include <cstdint>
#include <pulse/simple.h>

#include "soundSystem.hpp"
#include "exceptions.hpp"

class PulseSoundSystem : public SoundSystem
{
public:
	PulseSoundSystem(unsigned p_mode = 7);
	virtual ~PulseSoundSystem(void);


public:
	void processSound(uint32_t p_rate);

private:
	int16_t *m_audioBuffer;
	unsigned m_bufferSize;

	std::thread *m_threadProcess;
	bool m_terminated;

	pa_simple *m_paSimple;
};


#endif

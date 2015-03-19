#include <iostream>
#include <pulse/simple.h>
#include <pulse/error.h>

#include "pulseSoundSystem.hpp"
#include "port.h"
#include "soundux.h"

PulseSoundSystem::PulseSoundSystem(unsigned int p_mode):
	SoundSystem(),
	m_audioBuffer(NULL),
	m_threadProcess(NULL),
	m_terminated(false)
{
	if (p_mode >= sizeof(audioFrequencies) / sizeof(audioFrequencies[0]))
		p_mode = (sizeof(audioFrequencies) / sizeof(audioFrequencies[0]))- 1;
	m_bufferSize = audioBufferSizes[p_mode];

	m_audioBuffer = new int16_t[m_bufferSize];
	if (!m_audioBuffer)
		throw SnesException("Unable to allocate audio buffer.");

	memset(m_audioBuffer, 0, m_bufferSize * sizeof(*m_audioBuffer));

	m_threadProcess = new std::thread(&PulseSoundSystem::processSound,
	                                  this,
	                                  audioFrequencies[p_mode]);
}


PulseSoundSystem::~PulseSoundSystem()
{
	if (m_threadProcess)
	{
		m_terminated = true;
		m_threadProcess->join();
		delete m_threadProcess;
	}

	if (m_audioBuffer)
		delete [] m_audioBuffer;
}

//#define SHOW_LATENCY
void PulseSoundSystem::processSound(uint32_t p_rate)
{
	int error;
	pa_sample_spec ss;
	pa_buffer_attr attr;

	ss.format = PA_SAMPLE_S16LE;
	ss.channels = 2;
	ss.rate = p_rate;

	attr.maxlength = -1;
	attr.tlength = m_bufferSize * sizeof(*m_audioBuffer);
	attr.prebuf = m_bufferSize * sizeof(*m_audioBuffer);
	attr.minreq = 0;
	attr.fragsize = -1;

	m_paSimple = pa_simple_new(NULL, // Use the default server.
	                           "Snes9x", // Our application's name.
	                           PA_STREAM_PLAYBACK,
	                           NULL, // Use the default device.
	                           "Sound", // Description of our stream.
	                           &ss, // Our sample format.
	                           NULL, // Use default channel map
	                           &attr, // Use default buffering attributes.
	                           NULL); // Ignore error code.

	if (!m_paSimple)
		throw SnesException("Unable to init pulseaudio.");

	while (!m_terminated)
	{
		S9xMixSamplesO(m_audioBuffer,
		               m_bufferSize,
		               0);

		if (pa_simple_write(m_paSimple,
		                    m_audioBuffer,
		                    m_bufferSize * sizeof(*m_audioBuffer),
		                    &error) < 0)
			std::cerr<<"Pulse: "<<pa_strerror(error)<<std::endl;

		#ifdef SHOW_LATENCY
		pa_usec_t latency;
		if ((latency = pa_simple_get_latency(m_paSimple, &error)) == pa_usec_t(-1))
			std::cerr<<"Pulse latency: "<<pa_strerror(error)<<std::endl;
		std::cerr<<latency<<" usec \r";
		#endif
	}

	pa_simple_free(m_paSimple);
}

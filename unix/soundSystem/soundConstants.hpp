#ifndef _SOUNDCONSTANTS_HPP_
#define _SOUNDCONSTANTS_HPP_

/********************************* Constants **********************************/
#define VOL_DIV16 0x0080

#define SOUND_DECODE_LENGTH 16

#define FIXED_POINT 0x10000UL
#define FIXED_POINT_SHIFT 16
#define FIXED_POINT_REMAINDER 0xffffUL


/********************************** Macros ************************************/
#define CLIP16(v) \
	if ((v) < -32768) \
    (v) = -32768; \
	else \
	if ((v) > 32767) \
(v) = 32767

#define CLIP8(v) \
	if ((v) < -128) \
    (v) = -128; \
	else \
	if ((v) > 127) \
(v) = 127

/********************************** Types *************************************/
enum SoundState { SOUND_SILENT, SOUND_ATTACK, SOUND_DECAY, SOUND_SUSTAIN,
                  SOUND_RELEASE, SOUND_GAIN, SOUND_INCREASE_LINEAR,
                  SOUND_INCREASE_BENT_LINE, SOUND_DECREASE_LINEAR,
                  SOUND_DECREASE_EXPONENTIAL};

enum SoundMode { MODE_NONE = SOUND_SILENT, MODE_ADSR, MODE_RELEASE = SOUND_RELEASE,
                 MODE_GAIN, MODE_INCREASE_LINEAR, MODE_INCREASE_BENT_LINE,
                 MODE_DECREASE_LINEAR, MODE_DECREASE_EXPONENTIAL};

enum SoundType { SOUND_SAMPLE = 0, SOUND_NOISE, SOUND_EXTRA_NOISE, SOUND_MUTE };


#endif

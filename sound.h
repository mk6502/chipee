#ifndef CHIPEE_SOUND_H_
#define CHIPEE_SOUND_H_

void init_chipee_sound();
void audio_callback(void* user_data, unsigned char* raw_buffer, int bytes);
void beep_sound();
void stop_chipee_sound();

#endif

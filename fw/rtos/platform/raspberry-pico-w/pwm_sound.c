// Output PWM signals on pins 0 and 1

#include "pico/stdlib.h"
#include "hardware/pwm.h"

#include "pwm_sound.h"

#include "qor.h"


int sound_status = SOUND_STATUS_STOPPED;


static inline void pwm_calcDivTop(pwm_config *c, int frequency, int sysClock) {
	uint  count = sysClock * 16 / frequency;
	uint  div =  count / 60000;  // to be lower than 65535*15/16 (rounding error)
	if (div < 16) {
		div = 16;
	}
	c->div = div;
	c->top = count / div;
}


void play_melody(uint gpio, note_struct * n, uint tempo) {
	// Find out which PWM slice is connected to the GPIO
	uint slice_num = pwm_gpio_to_slice_num(gpio);

	pwm_config cfg = pwm_get_default_config();

	pwm_set_chan_level(slice_num, PWM_CHAN_A, 0);

	int wholenote = (60000 * 4) / tempo;

	int loop;
	int duration;

	sound_status = SOUND_STATUS_PLAYING;
	for (loop=0;;loop++) {
		duration = n[loop].duration;
		if (duration == 0) break;

		if (duration > 0)
			duration = wholenote / duration;
		else
			duration = (3 * wholenote / (-duration))/2;

		if (n[loop].frequency!=REST) {
			pwm_set_enabled(slice_num, false);
			//get new frequency
			pwm_calcDivTop(&cfg,n[loop].frequency,125000000);
			pwm_init(slice_num,&cfg,true);
			pwm_set_chan_level(slice_num,PWM_CHAN_A,cfg.top / 2);
			pwm_set_enabled(slice_num, true);
			//sleep_us(900 * duration);
			qor_sleep(duration);
			pwm_set_chan_level(slice_num,PWM_CHAN_A,0);
			sleep_us(100 * duration);
		} else {
			//sleep_us(1000 * duration);
			qor_sleep(duration);
		}
		if (sound_status == SOUND_STATUS_STOPPED) {
			break;
		}
	}
	sound_status = SOUND_STATUS_STOPPED;
}

void stop_melody() {
	sound_status = SOUND_STATUS_STOPPED;
}

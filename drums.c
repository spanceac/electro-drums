#include "stm32f10x.h"
#include "stm32-ints.h"
#include "samples/kick-16bit.h"
#include "samples/crash-16bit.h"
#include "samples/snare-16bit.h"
#include "samples/ride-16bit.h"
#include "samples/tom1-16bit.h"
#include "samples/tom2-16bit.h"

#define bit_set(var,bitno) ((var) |= 1 << (bitno))
#define bit_clr(var,bitno) ((var) &= ~(1 << (bitno)))
#define testbit(var,bitno) (((var)>>(bitno)) & 0x01)

#define TMR_CLK_HZ 24e6
#define TMR_FREQ_HZ 22050
#define TMR_LOAD_VAL ((TMR_CLK_HZ) / (TMR_FREQ_HZ))
#define BTTN_STABLE_TIME_MS 30
/* the button polling code repeats with frequency of audio timer */
#define BTTN_STABLE_CYCLES (((BTTN_STABLE_TIME_MS)*(TMR_FREQ_HZ)) / 1000)

#define dac_write_12bit_left_aligned(val) DAC->DHR12L1 = val

#define TRACK_NR 6
#define NR_SAMPLES_OF(track) ((sizeof(track))/(sizeof(s16)))
#define MIN_SIGNED_16BIT_VAL -32768
#define MAX_SIGNED_16BIT_VAL 32767

void timer2_init(void)
{
	bit_set(RCC->APB1ENR, 0); /* enable TMR2 clock */
	TIM2->ARR = TMR_LOAD_VAL;
	TIM2->CR1 = 0;
	TIM2->CR2 = 0;
	TIM2->PSC = 0; /* no prescaler */
	bit_set(TIM2->CR2, 5); /* timer update event used for TRGO */ 
	bit_set(TIM2->CR1, 7); /* auto reload */
	bit_set(TIM2->CR1, 0); /* counter enable */
}

void dac_init(void)
{
	bit_set(RCC->APB1ENR, 29); /* enable DAC clock */
	DAC->CR = 0;
	bit_set(DAC->CR, 5); /* Timer 2 TRGO trigger */
	bit_set(DAC->CR, 2); /* trigger enable */
	bit_set(DAC->CR, 0); /* DAC channel 1 enable */
}

void set_fast_clk(void)
{
	/* setting SYSCLK to 24 MHz internal clock
	SYSCLK = (HSI / 2) * PLL_MUL = (8 / 2) * 6 = 24 MHz */ 
	bit_set(RCC->CFGR, 20); /* PLL x 6 (must be set while PLL off) */
	bit_set(RCC->CR, 24); /* PLL ON */
	bit_set(RCC->CFGR, 1); /* PLL CLK is SYSCLK */
}

void main(void)
{
	int i = 0;
	u16 dac_val = 0;
	char bttn_press = 0;
	signed int mix = 0, samp_sum = 0;

	unsigned int button_tracker[TRACK_NR];

	char tracks_playing = 0; /* each bit tells us track playing */
	int samp_play_cnt[TRACK_NR] = {0};

	int tracks_samp_nr[TRACK_NR] = {NR_SAMPLES_OF(kick), NR_SAMPLES_OF(crash),
	    NR_SAMPLES_OF(snare), NR_SAMPLES_OF(ride),
	    NR_SAMPLES_OF(tom1), NR_SAMPLES_OF(tom2)};

	s16 const *tracks[TRACK_NR] = {
	    kick,
	    crash,
	    snare,
	    ride,
	    tom1,
	    tom2
	};

	set_fast_clk(); /* enable 24 MHz SYSCLK */
	RCC->APB2ENR |= (1 << 3) | 1;	/* Enable the GPIOB (bit 3) and AFIO (bit 0) */

	bit_set(AFIO->MAPR, 25); /* JTAG disabled - SWD enabled to have PB3 available as GPIO */
	GPIOB->CRL = 0x88888888; /* Set GPIOB Pin 0 - 7 to input pull-up/down */
	GPIOB->ODR = 0xffffffff;
	dac_init();
	timer2_init();

	for (i = 0; i < TRACK_NR; i++)
		button_tracker[i] = 0;

	while (1)
	{
		mix = 0;
		samp_sum = 0;

		for (i = 0; i < TRACK_NR; i++)
		{
			if (testbit(GPIOB->IDR, i) == 0)
			{
				if (button_tracker[i] == BTTN_STABLE_CYCLES)
				{
					/* stable, we call a button press */
					bit_set(tracks_playing, i);
					samp_play_cnt[i] = tracks_samp_nr[i];
				}
				button_tracker[i]++;
			}
			else
				button_tracker[i] = 0;
		}

		for (i = 0; i < TRACK_NR; i++) /* here we mix the samples */
		{
			if (testbit(tracks_playing, i) == 1)
			{
				int current_samp_nr = tracks_samp_nr[i] - samp_play_cnt[i];
				samp_sum += tracks[i][current_samp_nr];
				samp_play_cnt[i]--; /* decrement number of samples left to play */
				if (samp_play_cnt[i] == 0)
					bit_clr(tracks_playing, i); /* finished playing this track */
			}
		}
		mix = samp_sum;

		if (mix < -32768)
			mix = -32768;
		else if (mix > 32767)
			mix = 32767;

		mix += 32768; /* add DC offset to convert to unsingned for DAC*/
		dac_val = mix;
		dac_write_12bit_left_aligned(dac_val);

		while (testbit(TIM2->SR, 0) == 0); /* wait for timer reset */
		bit_clr(TIM2->SR, 0);
	}
}

void nmi_handler(void)
{
	return;
}

void hardfault_handler(void)
{
	return;
}


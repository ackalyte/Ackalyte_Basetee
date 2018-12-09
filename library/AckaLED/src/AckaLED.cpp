/*  AckaLED - This library is based on OCTOWS2811 library written by
	Paul Stoffregen, PJRC.COM, LLC. 
	http://www.pjrc.com/teensy/td_libs_OctoWS2811.html
    Copyright (c) 2013 Paul Stoffregen, PJRC.COM, LLC
    Some Teensy-LC support contributed by Mark Baysinger.
    https://forum.pjrc.com/threads/40863-Teensy-LC-port-of-OctoWS2811


	OctoWS2811 is a high performance WS2811 LED Display Library.
	However it is limited to only 8 channels and doesn't support RGBW type LED. AckaLED
	fill in these gaps for OctoWS2811 library.

	Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

#include <string.h>
#include "AckaLED.h"


uint16_t AckaLED::stripLen;
void * AckaLED::frameBuffer;
void * AckaLED::drawBuffer;
uint8_t AckaLED::params;
DMAChannel AckaLED::dma1;
DMAChannel AckaLED::dma2;
DMAChannel AckaLED::dma3;


static uint16_t ones = 0xFFFF;
static volatile uint8_t update_in_progress = 0;
static uint32_t update_completed_at = 0;

AckaLED::AckaLED(uint32_t numPerStrip, void *frameBuf, void *drawBuf, uint8_t config)
{
	stripLen = numPerStrip;
	frameBuffer = frameBuf;
	drawBuffer = drawBuf;
	params = config;
}

// Waveform timing: these set the high time for a 0 and 1 bit, as a fraction of
// the total 800 kHz or 400 kHz clock cycle.  The scale is 0 to 255.  The Worldsemi
// datasheet seems T1H should be 600 ns of a 1250 ns cycle, or 48%.  That may
// erroneous information?  Other sources reason the chip actually samples the
// line close to the center of each bit time, so T1H should be 80% if TOH is 20%.
// The chips appear to work based on a simple one-shot delay triggered by the
// rising edge.  At least 1 chip tested retransmits 0 as a 330 ns pulse (26%) and
// a 1 as a 660 ns pulse (53%).  Perhaps it's actually sampling near 500 ns?
// There doesn't seem to be any advantage to making T1H less, as long as there
// is sufficient low time before the end of the cycle, so the next rising edge
// can be detected.  T0H has been lengthened slightly, because the pulse can
// narrow if the DMA controller has extra latency during bus arbitration.  If you
// have an insight about tuning these parameters AND you have actually tested on
// real LED strips, please contact paul@pjrc.com.  Please do not email based only
// on reading the datasheets and purely theoretical analysis.
#define WS2811_TIMING_T0H  60
#define WS2811_TIMING_T1H  176

void AckaLED::begin(void)
{
	uint32_t bufsize, frequency;
	
	if ((params & 0xF0) == SK6812_800kHz)
	{
		bufsize = stripLen * 32 * 2;
	}
	else
	{
		bufsize = stripLen * 24 * 2;
	}

	memset(frameBuffer, 0, bufsize);
	if (drawBuffer) {
		memset(drawBuffer, 0, bufsize);
	} else {
		drawBuffer = frameBuffer;
	}

	pinMode(9, OUTPUT);

	// configure the 8 output pins
	GPIOD_PCOR = 0xFFFF;
	pinMode(2, OUTPUT);		// strip #1
	pinMode(14, OUTPUT);	// strip #2
	pinMode(7, OUTPUT);		// strip #3
	pinMode(8, OUTPUT);		// strip #4
	pinMode(6, OUTPUT);		// strip #5
	pinMode(20, OUTPUT);	// strip #6
	pinMode(21, OUTPUT);	// strip #7
	pinMode(5, OUTPUT);		// strip #8

#if defined (__MK66FX1M0__)	// configure additional 7 output pins for Teensy 3.6
	pinMode(47, OUTPUT);	// strip #9
	pinMode(48, OUTPUT);	// strip #10
	pinMode(55, OUTPUT);	// strip #11
	pinMode(53, OUTPUT);	// strip #12
	pinMode(52, OUTPUT);	// strip #13
	pinMode(51, OUTPUT);	// strip #14
	pinMode(54, OUTPUT);	// strip #15
#endif

	// create the two waveforms for WS2811 low and high bits
	switch (params & 0xF0) {
	case WS2811_400kHz:
		frequency = 400000;
		frameSetDelay = 50;
		break;
	case WS2811_800kHz:
		frequency = 800000;
		frameSetDelay = 50;
		break;
	case WS2813_800kHz:
		frequency = 800000;
		frameSetDelay = 300;
		break;
	case SK6812_800kHz:
		frequency = 800000;
		frameSetDelay = 80;
	default:
		frequency = 800000;
		frameSetDelay = 50;
	}


#if defined(__MK20DX128__)
	FTM1_SC = 0;
	FTM1_CNT = 0;
	uint32_t mod = (F_BUS + frequency / 2) / frequency;
	FTM1_MOD = mod - 1;
	FTM1_SC = FTM_SC_CLKS(1) | FTM_SC_PS(0);
	FTM1_C0SC = 0x69;
	FTM1_C1SC = 0x69;
	FTM1_C0V = (mod * WS2811_TIMING_T0H) >> 8;
	FTM1_C1V = (mod * WS2811_TIMING_T1H) >> 8;
	// pin 16 triggers DMA(port B) on rising edge
	CORE_PIN16_CONFIG = PORT_PCR_IRQC(1)|PORT_PCR_MUX(3);

#elif defined(__MK20DX256__)
	FTM2_SC = 0;
	FTM2_CNT = 0;
	uint32_t mod = (F_BUS + frequency / 2) / frequency;
	FTM2_MOD = mod - 1;
	FTM2_SC = FTM_SC_CLKS(1) | FTM_SC_PS(0);
	FTM2_C0SC = 0x69;
	FTM2_C1SC = 0x69;
	FTM2_C0V = (mod * WS2811_TIMING_T0H) >> 8;
	FTM2_C1V = (mod * WS2811_TIMING_T1H) >> 8;
	// pin 32 is FTM2_CH0, PTB18, triggers DMA(port B) on rising edge
	// pin 25 is FTM2_CH1, PTB19
	CORE_PIN32_CONFIG = PORT_PCR_IRQC(1)|PORT_PCR_MUX(3);

#elif defined(__MK64FX512__) || defined(__MK66FX1M0__)
	FTM2_SC = 0;
	FTM2_CNT = 0;
	uint32_t mod = (F_BUS + frequency / 2) / frequency;
	FTM2_MOD = mod - 1;
	FTM2_SC = FTM_SC_CLKS(1) | FTM_SC_PS(0);
	FTM2_C0SC = 0x69;
	FTM2_C1SC = 0x69;
	FTM2_C0V = (mod * WS2811_TIMING_T0H) >> 8;
	FTM2_C1V = (mod * WS2811_TIMING_T1H) >> 8;
	// FTM2_CH0, PTA10 (not connected), triggers DMA(port A) on rising edge
	PORTA_PCR10 = PORT_PCR_IRQC(1)|PORT_PCR_MUX(3);

#elif defined(__MKL26Z64__)
	analogWriteResolution(8);
	analogWriteFrequency(3, frequency);
	analogWriteFrequency(4, frequency);
	analogWrite(3, WS2811_TIMING_T0H);
	analogWrite(4, WS2811_TIMING_T1H);
	CORE_PIN3_CONFIG = 0;
	CORE_PIN4_CONFIG = 0;

#endif

	// DMA channel #1 sets WS2811 high at the beginning of each cycle
	dma1.source(ones);
	dma1.destination(GPIOD_PSOR);
	dma1.transferSize(2);
	dma1.transferCount(bufsize/2);
	dma1.disableOnCompletion();

	// DMA channel #2 writes the pixel data at 23% of the cycle
	dma2.sourceBuffer((uint16_t *)frameBuffer, bufsize);
	dma2.destination(GPIOD_PDOR);
	dma2.transferSize(2);
	dma2.transferCount(bufsize / 2);  // Corrupt output when equal to bufsize. Why?
	dma2.disableOnCompletion();

	// DMA channel #3 clear all the pins low at 69% of the cycle
	dma3.source(ones);
	dma3.destination(GPIOD_PCOR);
	dma3.transferSize(2);
	dma3.transferCount(bufsize/2);
	dma3.disableOnCompletion();
	dma3.interruptAtCompletion();

#if defined(__MK20DX128__)
	// route the edge detect interrupts to trigger the 3 channels
	dma1.triggerAtHardwareEvent(DMAMUX_SOURCE_PORTB);
	dma2.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM1_CH0);
	dma3.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM1_CH1);
	DMAPriorityOrder(dma3, dma2, dma1);
#elif defined(__MK20DX256__)
	// route the edge detect interrupts to trigger the 3 channels
	dma1.triggerAtHardwareEvent(DMAMUX_SOURCE_PORTB);
	dma2.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM2_CH0);
	dma3.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM2_CH1);
	DMAPriorityOrder(dma3, dma2, dma1);
#elif defined(__MK64FX512__) || defined(__MK66FX1M0__)
	// route the edge detect interrupts to trigger the 3 channels
	dma1.triggerAtHardwareEvent(DMAMUX_SOURCE_PORTA);
	dma2.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM2_CH0);
	dma3.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM2_CH1);
	DMAPriorityOrder(dma3, dma2, dma1);
#elif defined(__MKL26Z64__)
	// route the timer interrupts to trigger the 3 channels
	dma1.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM2_OV);
	dma2.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM2_CH0);
	dma3.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM2_CH1);
#endif

	// enable a done interrupts when channel #3 completes
	dma3.attachInterrupt(isr);
}

void AckaLED::isr(void)
{
	dma3.clearInterrupt();
	update_completed_at = micros();
	update_in_progress = 0;
}

int AckaLED::busy(void)
{
	if (update_in_progress) return 1;
	// busy for 50 (or 300 for ws2813) us after the done interrupt, for WS2811 reset
	if (micros() - update_completed_at < frameSetDelay) return 1;
	return 0;
}

void AckaLED::show(void)
{
	uint16_t *p = (uint16_t *)drawBuffer;
	if ((params & 0xF0) == SK6812_800kHz)
	{
		for (int i = 0; i < 32; i++)
		{
			p++;
		}
	}
	else
	{
		for (int i = 0; i < 24; i++)
		{
			p++;
		}
	}

	p = (uint16_t *)frameBuffer;
	if ((params & 0xF0) == SK6812_800kHz)
	{
		for (int i = 0; i < 32; i++)
		{
			p++;
		}
	}
	else
	{
		for (int i = 0; i < 24; i++)
		{
			p++;
		}
	}

	while (update_in_progress) ;
	if (drawBuffer != frameBuffer) {
		if ((params & 0xF0) == SK6812_800kHz)
		{
			memcpy(frameBuffer, drawBuffer, stripLen * 32 * 2);
		}
		else
		{
			memcpy(frameBuffer, drawBuffer, stripLen * 24 * 2);
		}
	}
	p = (uint16_t *)drawBuffer;
	if ((params & 0xF0) == SK6812_800kHz)
	{
		for (int i = 0; i < 32; i++)
		{
			p++;
		}
	}
	else
	{
		for (int i = 0; i < 24; i++)
		{
			p++;
		}
	}

	p = (uint16_t *)frameBuffer;
	if ((params & 0xF0) == SK6812_800kHz)
	{
		for (int i = 0; i < 32; i++)
		{
			p++;
		}
	}
	else
	{
		for (int i = 0; i < 24; i++)
		{
			p++;
		}
	}



	while (micros() - update_completed_at < frameSetDelay) ;
	// ok to start, but we must be very careful to begin
	// without any prior 3 x 800kHz DMA requests pending

#if defined(__MK20DX128__)
	uint32_t cv = FTM1_C1V;
	noInterrupts();
	// CAUTION: this code is timing critical.
	while (FTM1_CNT <= cv) ;
	while (FTM1_CNT > cv) ; // wait for beginning of an 800 kHz cycle
	while (FTM1_CNT < cv) ;
	FTM1_SC = 0;            // stop FTM1 timer (hopefully before it rolls over)
	update_in_progress = 1;
	PORTB_ISFR = (1<<0);    // clear any prior rising edge
	uint32_t tmp __attribute__((unused));
	FTM1_C0SC = 0x28;
	tmp = FTM1_C0SC;        // clear any prior timer DMA triggers
	FTM1_C0SC = 0x69;
	FTM1_C1SC = 0x28;
	tmp = FTM1_C1SC;
	FTM1_C1SC = 0x69;
	dma1.enable();
	dma2.enable();          // enable all 3 DMA channels
	dma3.enable();
	FTM1_SC = FTM_SC_CLKS(1) | FTM_SC_PS(0); // restart FTM1 timer

#elif defined(__MK20DX256__)
	FTM2_C0SC = 0x28;
	FTM2_C1SC = 0x28;
	delay(1);
	uint32_t cv = FTM2_C1V;
	noInterrupts();
	// CAUTION: this code is timing critical.
	while (FTM2_CNT <= cv) ;
	while (FTM2_CNT > cv) ; // wait for beginning of an 800 kHz cycle
	while (FTM2_CNT < cv) ;
	FTM2_SC = 0;             // stop FTM2 timer (hopefully before it rolls over)
	update_in_progress = 1;
	PORTB_ISFR = (1<<18);    // clear any prior rising edge
	uint32_t tmp __attribute__((unused));
	FTM2_C0SC = 0x28;
	tmp = FTM2_C0SC;         // clear any prior timer DMA triggers
	FTM2_C0SC = 0x69;
	FTM2_C1SC = 0x28;
	tmp = FTM2_C1SC;
	FTM2_C1SC = 0x69;
	dma1.enable();
	dma2.enable();           // enable all 3 DMA channels
	dma3.enable();
	FTM2_SC = FTM_SC_CLKS(1) | FTM_SC_PS(0); // restart FTM2 timer

#elif defined(__MK64FX512__) || defined(__MK66FX1M0__)
	FTM2_C0SC = 0x28;
	FTM2_C1SC = 0x28;
	delay(1);
	uint32_t cv = FTM2_C1V;
	noInterrupts();
	// CAUTION: this code is timing critical.
	while (FTM2_CNT <= cv) ;
	while (FTM2_CNT > cv) ; // wait for beginning of an 800 kHz cycle
	while (FTM2_CNT < cv) ;
	FTM2_SC = 0;             // stop FTM2 timer (hopefully before it rolls over)
	update_in_progress = 1;
	PORTA_ISFR = (1<<10);    // clear any prior rising edge
	uint32_t tmp __attribute__((unused));
	FTM2_C0SC = 0x28;
	tmp = FTM2_C0SC;         // clear any prior timer DMA triggers
	FTM2_C0SC = 0x69;
	FTM2_C1SC = 0x28;
	tmp = FTM2_C1SC;
	FTM2_C1SC = 0x69;
	dma1.enable();
	dma2.enable();           // enable all 3 DMA channels
	dma3.enable();
	FTM2_SC = FTM_SC_CLKS(1) | FTM_SC_PS(0); // restart FTM2 timer

#elif defined(__MKL26Z64__)
	uint32_t sc = FTM2_SC;
	uint32_t cv = FTM2_C1V;
	noInterrupts();
	update_in_progress = 1;
	while (FTM2_CNT <= cv) ;
	while (FTM2_CNT > cv) ; // wait for beginning of an 800 kHz cycle
	while (FTM2_CNT < cv) ;
	FTM2_SC = 0;		// stop FTM2 timer (hopefully before it rolls over)
	dma1.clearComplete();
	dma2.clearComplete();
	dma3.clearComplete();
	uint32_t bufsize = stripLen*24;
	dma1.transferCount(bufsize);
	dma2.transferCount(bufsize);
	dma3.transferCount(bufsize);
	dma2.sourceBuffer((uint8_t *)frameBuffer, bufsize);
	// clear any pending event flags
	FTM2_SC = 0x80;
	FTM2_C0SC = 0xA9;	// clear any previous pending DMA requests
	FTM2_C1SC = 0xA9;
	// clear any prior pending DMA requests
	dma1.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM2_OV);
	dma2.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM2_CH0);
	dma3.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM2_CH1);
	dma1.enable();
	dma2.enable();		// enable all 3 DMA channels
	dma3.enable();
	FTM2_SC = 0x188;
#endif
	interrupts();
}

void AckaLED::setPixel(uint32_t num, int color)
{
	uint32_t strip, offset, mask;
	uint16_t bit, *p;

	switch (params & 7) {
	  case WS2811_RBG:
			color = (color&0xFF0000) | ((color<<8)&0x00FF00) | ((color>>8)&0x0000FF);
			break;
	  case WS2811_GRB:
			color = ((color<<8)&0xFF0000) | ((color>>8)&0x00FF00) | (color&0x0000FF);
			break;
	  case WS2811_GBR:
			color = ((color<<8)&0xFFFF00) | ((color>>16)&0x0000FF);
			break;
		case SK6812_GRBW:
			color = ((color << 16) & 0xFF000000) | (color & 0x00FF0000) | ((color << 8) & 0x0000FF00) | ((color >> 24) & 0x000000FF);
			break;
	  default:
		break;
	}

	strip = num / stripLen;  // Cortex-M4 has 2 cycle unsigned divide :-)
	offset = num % stripLen;
	bit = (1<<strip);
	if ((params & 7) == SK6812_GRBW)
	{
		p = ((uint16_t *)drawBuffer) + offset * 32;
		for (mask = (1 << 31); mask; mask >>= 1) 
		{
			if (color & mask) 
			{
				*p++ |= bit;
			}
			else 
			{
				*p++ &= ~bit;
			}
		}
	}
	else
	{
		p = ((uint16_t *)drawBuffer) + offset * 24;
		for (mask = (1 << 23); mask; mask >>= 1) 
		{
			if (color & mask) 
			{
				*p++ |= bit;
			}
			else 
			{
				*p++ &= ~bit;
			}
		}
	}
}

int AckaLED::getPixel(uint32_t num)
{
	uint32_t strip, offset, mask;
	uint16_t bit, *p;
	int color=0;

	strip = num / stripLen;
	offset = num % stripLen;
	bit = (1<<strip);
	if ((params & 7) == SK6812_GRBW)
	{
		p = ((uint16_t *)drawBuffer) + offset * 32;
		for (mask = (1 << 31); mask; mask >>= 1) 
		{
			if (*p++ & bit)
			{
				color |= mask;
			}
		}
	}
	else
	{
		p = ((uint16_t *)drawBuffer) + offset * 24;
		for (mask = (1 << 23); mask; mask >>= 1) 
		{
			if (*p++ & bit)
			{
				color |= mask;
			}
		}
		switch (params & 7) 
		{
		case WS2811_RBG:
			color = (color & 0xFF0000) | ((color << 8) & 0x00FF00) | ((color >> 8) & 0x0000FF);
			break;
		case WS2811_GRB:
			color = ((color << 8) & 0xFF0000) | ((color >> 8) & 0x00FF00) | (color & 0x0000FF);
			break;
		case WS2811_GBR:
			color = ((color << 8) & 0xFFFF00) | ((color >> 16) & 0x0000FF);
			break;
		case SK6812_GRBW:
			color = ((color << 16) & 0xFF000000) | (color & 0x00FF0000) | ((color << 8) & 0x0000FF00) | ((color >> 24) & 0x000000FF);
			break;
		default:
			break;
		}
	}
	return color;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pico_parallel_ws2812b.pio.h"

#define NUM_PIXELS 5
#define LED_PIN 25

//TODO
//technically, NUM_CHANNELS could be a factor of 32, but that would need a tweak to the PIO code.
//would probably good to have options for 16 and 8
#define NUM_CHANNELS 32

//the actual number of hardware outputs you want
#define REAL_NUM_CHANNELS 10

#define RESET_TIME_US 400
#define START_PIN 1

int dma_chan;

uint32_t pixels[NUM_PIXELS*24];


//note  -- this sets the order as RGB. May need to change.
uint32_t set_pixel_colour(int pixel, int channel, uint8_t r, uint8_t g, uint8_t b) {
	
	uint32_t colour_value = (b << 16 | r << 8 | g);
	
	for(int i=0; i<24;i++) {
		if (colour_value & (1u<<i)) {
			pixels[(pixel*24) + i] |= 1u << (channel); 
		}
		else {
			pixels[(pixel*24) + i] &= ~(1u<<channel);
		}
		
	}
}

int64_t dma_start() {
	dma_channel_set_read_addr(dma_chan, pixels, true);
	gpio_put(LED_PIN, 1);
	return 0;
}

//this will be called once a full DMA to the TX Fifo has completed. It just needs to wait and restart
void dma_handler() {
	dma_irqn_acknowledge_channel(DMA_IRQ_0,dma_chan);
	
	//need a pause to re-set the LED strip before we can start again.
	//can't use sleep in irq handlers as weird things happen
	add_alarm_in_us(RESET_TIME_US, dma_start, NULL, false);
}

//this should just need changing over to the parallel PIO program
//need to init the values first.
int main() {
    PIO pio = pio0;
    int sm = 0;
	
	gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
	
    uint offset = pio_add_program(pio, &ws2812_parallel_program);
	
    ws2812_parallel_program_init(pio, sm, offset, START_PIN, REAL_NUM_CHANNELS, 800000);

	dma_chan = dma_claim_unused_channel(true);
	
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
	
	channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
	channel_config_set_read_increment(&c, true);
	
	//See datasheet 2.5.3.1
	channel_config_set_dreq(&c, DREQ_PIO0_TX0);
	
	dma_channel_configure(
        dma_chan,
        &c,
        &pio0_hw->txf[0], // Write address (only need to set this once)
        NULL,             // Don't provide a read address yet
        NUM_PIXELS*24, 	  // number of transfers
        false             // Don't start yet
    );
	

	// Tell the DMA to raise IRQ line 0 when the channel finishes a block
    dma_channel_set_irq0_enabled(dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
	
	//kick off the initial transfer
	dma_handler();
	
	//a bit of test colour
	
	for (int i=0; i<NUM_PIXELS;i++) {
		for (int j=0; j<5;j++) {
			
			set_pixel_colour(i,j,000,100,0);
		}
	}
	set_pixel_colour(1,1,100,0,0);
	set_pixel_colour(2,2,0,0,100);
	
	
	//do whatever you want
	//do your animation here
	while (true) {
        
        sleep_ms(250);
        gpio_put(LED_PIN, 0);
        sleep_ms(250);
    }
}
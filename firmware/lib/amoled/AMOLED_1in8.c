/*****************************************************************************
* | File      	:   AMOLED_1in8.c
* | Author      :   Waveshare Team
* | Function    :   AMOLED Interface Functions
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2025-03-20
* | Info        :   
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of theex Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
******************************************************************************/
#include <assert.h>
#include "DEV_Config.h"
#include "AMOLED_1in8.h"
#include "pico/mutex.h"
#include "pico/time.h"

AMOLED_1IN8_ATTRIBUTES AMOLED_1IN8;

// dma_tx (DEV_Config.c) is one single global DMA channel, used with zero
// locking by both this driver's own calls AND lib/bootlog's independent
// diagnostic calls - on the Doom project (not this calibration firmware),
// those come from different cores (bootlog from core0, the game's own
// present_frame_to_amoled() from core1), racing on dma_tx unsynchronized.
// Confirmed on hardware (2026-08-16, see doom/docs/DECISIONS.md): both the
// game view AND the bootlog text froze simultaneously right after a
// diagnostic change made bootlog print on every frame instead of rarely,
// which otherwise would have made this collision unlikely to ever hit.
// Guards just AMOLED_1IN8_DisplayWindowPacked() (the only function either
// caller actually uses - see DECISIONS.md on why the others proved
// intermittently unreliable). Lazy-init is safe: the first-ever call to
// this function always happens during single-core boot, before core1
// (or, in this calibration firmware, before any second caller) exists.
static mutex_t dma_tx_mutex;
static volatile bool dma_tx_mutex_ready = false;
static bool packed_transfer_active = false;
static bool address_axes_exchanged = false;
#if AMOLED_ENABLE_PROFILING
static volatile uint32_t dma_timeout_count;
#endif

static bool wait_for_dma_or_recover(uint64_t timeout_us)
{
    uint64_t started_us = time_us_64();
    while (dma_channel_is_busy(dma_tx)) {
        if (time_us_64() - started_us > timeout_us) {
#if AMOLED_ENABLE_PROFILING
            dma_timeout_count++;
#endif
            dma_channel_abort(dma_tx);
            pio_sm_set_enabled(qspi.pio, qspi.sm, false);
            pio_sm_clear_fifos(qspi.pio, qspi.sm);
            pio_sm_restart(qspi.pio, qspi.sm);
            pio_sm_set_enabled(qspi.pio, qspi.sm, true);
            return false;
        }
        tight_loop_contents();
    }
    return true;
}

uint32_t AMOLED_1IN8_GetDmaTimeoutCount(void)
{
#if AMOLED_ENABLE_PROFILING
    return dma_timeout_count;
#else
    return 0;
#endif
}

/********************************************************************************
function:	Sets the start position and size of the display area
parameter:
        qspi    ：  qspi structure
		Xstart 	:   X direction Start coordinates
		Ystart  :   Y direction Start coordinates
		Xend    :   X direction end coordinates
		Yend    :   Y direction end coordinates
********************************************************************************/
void AMOLED_1IN8_SetWindows(uint32_t Xstart, uint32_t Ystart, uint32_t Xend, uint32_t Yend){
    QSPI_Select(qspi); 
    QSPI_REGISTER_Write(qspi, 0x2a); 
    QSPI_DATA_Write(qspi, Xstart>>8);
    QSPI_DATA_Write(qspi, Xstart&0xff);
    QSPI_DATA_Write(qspi, (Xend-1)>>8);
    QSPI_DATA_Write(qspi, (Xend-1)&0xff);
    QSPI_Deselect(qspi); 
    
    QSPI_Select(qspi); 
    QSPI_REGISTER_Write(qspi, 0x2b);
    QSPI_DATA_Write(qspi, Ystart>>8);
    QSPI_DATA_Write(qspi, Ystart&0xff);
    QSPI_DATA_Write(qspi, (Yend-1)>>8);
    QSPI_DATA_Write(qspi, (Yend-1)&0xff);
    QSPI_Deselect(qspi); 
    
    QSPI_Select(qspi); 
    QSPI_REGISTER_Write(qspi, 0x2c);
    QSPI_Deselect(qspi); 
    WAIT_TIME();
}

/******************************************************************************
function :	Initialize the lcd register
parameter:
        qspi    ：  qspi structure
******************************************************************************/
static void AMOLED_1IN8_InitReg(){
    QSPI_Select(qspi); 
    QSPI_REGISTER_Write(qspi, 0x11);
    sleep_ms(120);
    QSPI_Deselect(qspi);

    QSPI_Select(qspi);
    QSPI_REGISTER_Write(qspi, 0x44);
    QSPI_DATA_Write(qspi, 0x01);
    QSPI_DATA_Write(qspi, 0xC5); 
    QSPI_Deselect(qspi);
    
    QSPI_Select(qspi);
    QSPI_REGISTER_Write(qspi, 0x35);
    QSPI_DATA_Write(qspi, 0x00);
    QSPI_Deselect(qspi);

    QSPI_Select(qspi);
    QSPI_REGISTER_Write(qspi, 0x3A);
    QSPI_DATA_Write(qspi, 0x55);  
    QSPI_Deselect(qspi);
    
    QSPI_Select(qspi);
    QSPI_REGISTER_Write(qspi, 0xC4); 
    QSPI_DATA_Write(qspi, 0x80); 
    QSPI_Deselect(qspi);

    QSPI_Select(qspi);
    QSPI_REGISTER_Write(qspi, 0x53); 
    QSPI_DATA_Write(qspi, 0x20);
    QSPI_Deselect(qspi);

    QSPI_Select(qspi);
    QSPI_REGISTER_Write(qspi, 0x51);   
    QSPI_DATA_Write(qspi, 0xFF);
    QSPI_Deselect(qspi);

    QSPI_Select(qspi);
    QSPI_REGISTER_Write(qspi, 0x29);  
    QSPI_Deselect(qspi);
    
    sleep_ms(10);
}

/********************************************************************************
function :	Reset the lcd
parameter:
        qspi    ：  qspi structure
********************************************************************************/
static void AMOLED_1IN8_Reset(){
    gpio_put(qspi.pin_rst,1);
    DEV_Delay_ms(50);
    gpio_put(qspi.pin_rst,0);
    DEV_Delay_ms(50);
    gpio_put(qspi.pin_rst,1);
    DEV_Delay_ms(300);
}

/********************************************************************************
function :	Initialize the lcd
parameter:
        qspi    ：  qspi structure
********************************************************************************/
void AMOLED_1IN8_Init()
{
    //Hardware reset
    AMOLED_1IN8_Reset();
    
    //Set the initialization register
    AMOLED_1IN8_InitReg();

    AMOLED_1IN8.HEIGHT	= AMOLED_1IN8_HEIGHT;
    AMOLED_1IN8.WIDTH   = AMOLED_1IN8_WIDTH;
}

/******************************************************************************
function :	Set AMOLED Brightness
parameter:
******************************************************************************/
void AMOLED_1IN8_SetBrightness(uint8_t brightness){
    if(brightness > 100) brightness = 100;
    brightness = brightness * 255 / 100;

    // QSPI_1Wrie_Mode(&qspi);
    QSPI_Select(qspi); 
    QSPI_REGISTER_Write(qspi, 0x51);
    QSPI_DATA_Write(qspi, brightness);
    QSPI_Deselect(qspi);
}

void AMOLED_1IN8_SetMemoryAccessControl(uint8_t madctl)
{
    QSPI_Select(qspi);
    QSPI_REGISTER_Write(qspi, 0x36);
    QSPI_DATA_Write(qspi, madctl);
    QSPI_Deselect(qspi);
    address_axes_exchanged = (madctl & 0x20) != 0;
}


/******************************************************************************
function :	Clear screen
parameter:
******************************************************************************/
void AMOLED_1IN8_Clear(UWORD Color) {
    // Color data
    UWORD i;
	UWORD image[AMOLED_1IN8.HEIGHT];
	for(i=0;i<AMOLED_1IN8.HEIGHT;i++){
		image[i] = Color>>8 | (Color&0xff)<<8;
	}
	UBYTE *partial_image = (UBYTE *)(image);

    // Send command in one-line mode
    // QSPI_1Wrie_Mode(&qspi);
    AMOLED_1IN8_SetWindows(0,0,AMOLED_1IN8.WIDTH,AMOLED_1IN8.HEIGHT);
    QSPI_Select(qspi);
    QSPI_Pixel_Write(qspi,0x2c);

    // Four-wire mode sends RGB data
    // QSPI_4Wrie_Mode(&qspi);
    channel_config_set_dreq(&c, pio_get_dreq(qspi.pio, qspi.sm, true));
    for (int i = 0; i < AMOLED_1IN8.HEIGHT; i++) {
        dma_channel_configure(dma_tx, 
                            &c,
                            &qspi.pio->txf[qspi.sm],  // Destination pointer (PIO TX FIFO)
                            partial_image,            // Source pointer (data buffer)
                            AMOLED_1IN8.WIDTH*2,      // Data length (unit: number of transmissions)
                            true);                    // Start transferring immediately
        
        // Waiting for DMA transfer to complete
        while(dma_channel_is_busy(dma_tx));
    }

    QSPI_Deselect(qspi);
}


/******************************************************************************
function :	Send data to AMOLED to complete full screen refresh
parameter:
        Image   ：  Image data
******************************************************************************/
void AMOLED_1IN8_Display(UWORD *Image)
{
    // Send command in one-line mode
    // QSPI_1Wrie_Mode(&qspi);
    AMOLED_1IN8_SetWindows(0,0,AMOLED_1IN8.WIDTH,AMOLED_1IN8.HEIGHT);
    QSPI_Select(qspi);
    QSPI_Pixel_Write(qspi,0x2c);

    // Four-wire mode sends RGB data
    // QSPI_4Wrie_Mode(&qspi);
    channel_config_set_dreq(&c, pio_get_dreq(qspi.pio, qspi.sm, true));
    dma_channel_configure(dma_tx, 
                        &c,
                        &qspi.pio->txf[qspi.sm],  // Destination pointer (PIO TX FIFO)
                        (UBYTE *)Image,           // Source pointer (data buffer)
                        AMOLED_1IN8.WIDTH*AMOLED_1IN8.HEIGHT*2,   // Data length (unit: number of transmissions)
                        true);                    // Start transferring immediately
    
    // Waiting for DMA transfer to complete
    // A lost PIO DREQ used to block core1 forever here, which in turn blocks
    // Doom's core0/core1 render rendezvous and looks like a total gameplay
    // freeze. A 35KB tile normally completes in well under 20ms; on timeout,
    // abort the transfer and restart the PIO state machine so the next tile
    // or frame can recover instead of deadlocking permanently.
    wait_for_dma_or_recover(20000);
    QSPI_Deselect(qspi);
}

// Single-DMA-transfer window blit for a *tightly packed* Image buffer
// (no full-panel stride) - see the header comment on why this exists
// instead of just using AMOLED_1IN8_DisplayWindows().
void AMOLED_1IN8_DisplayWindowPackedStart(uint32_t Xstart, uint32_t Ystart,
                                          uint32_t Xend, uint32_t Yend,
                                          const UWORD *Image)
{
    if (!dma_tx_mutex_ready) {
        mutex_init(&dma_tx_mutex);
        dma_tx_mutex_ready = true;
    }
    mutex_enter_blocking(&dma_tx_mutex);

    // Holding the mutex until Wait() prevents every other display caller from
    // changing the shared DMA channel, PIO state machine, window, or CS.
    assert(!packed_transfer_active);

    uint32_t logical_width = address_axes_exchanged ? AMOLED_1IN8.HEIGHT : AMOLED_1IN8.WIDTH;
    uint32_t logical_height = address_axes_exchanged ? AMOLED_1IN8.WIDTH : AMOLED_1IN8.HEIGHT;
    if(Yend > logical_height) Yend = logical_height;
    if(Xend > logical_width) Xend = logical_width;

    AMOLED_1IN8_SetWindows(Xstart, Ystart, Xend, Yend);
    QSPI_Select(qspi);
    QSPI_Pixel_Write(qspi, 0x2c);

    channel_config_set_dreq(&c, pio_get_dreq(qspi.pio, qspi.sm, true));
    dma_channel_configure(dma_tx,
                        &c,
                        &qspi.pio->txf[qspi.sm],
                        (const UBYTE *)Image,
                        (Xend - Xstart) * (Yend - Ystart) * 2,
                        true);
    packed_transfer_active = true;
}

bool AMOLED_1IN8_DisplayWindowPackedWait(void)
{
    assert(packed_transfer_active);

    // Never let one lost PIO DREQ permanently strand core1 and therefore
    // Doom's render rendezvous.
    bool completed = wait_for_dma_or_recover(20000);
    QSPI_Deselect(qspi);

    packed_transfer_active = false;
    mutex_exit(&dma_tx_mutex);
    return completed;
}

void AMOLED_1IN8_DisplayWindowPacked(uint32_t Xstart, uint32_t Ystart,
                                     uint32_t Xend, uint32_t Yend,
                                     UWORD *Image)
{
    AMOLED_1IN8_DisplayWindowPackedStart(Xstart, Ystart, Xend, Yend, Image);
    AMOLED_1IN8_DisplayWindowPackedWait();
}

// Stream a window in chunks while keeping one command/CS transaction open.
// This avoids a full-window RAM buffer without returning to the unreliable
// per-row SetWindows pattern used by DisplayWindows().
void AMOLED_1IN8_DisplayStreamBegin(uint32_t Xstart, uint32_t Ystart, uint32_t Xend, uint32_t Yend)
{
    if (!dma_tx_mutex_ready) {
        mutex_init(&dma_tx_mutex);
        dma_tx_mutex_ready = true;
    }
    mutex_enter_blocking(&dma_tx_mutex);
    AMOLED_1IN8_SetWindows(Xstart, Ystart, Xend, Yend);
    QSPI_Select(qspi);
    QSPI_Pixel_Write(qspi, 0x2c);
}

void AMOLED_1IN8_DisplayStreamWrite(const void *data, uint32_t byte_count)
{
    channel_config_set_dreq(&c, pio_get_dreq(qspi.pio, qspi.sm, true));
    dma_channel_configure(dma_tx, &c, &qspi.pio->txf[qspi.sm], data,
                          byte_count, true);
    while (dma_channel_is_busy(dma_tx));
}

void AMOLED_1IN8_DisplayStreamEnd(void)
{
    QSPI_Deselect(qspi);
    mutex_exit(&dma_tx_mutex);
}

/******************************************************************************
function :	Send data to AMOLED to complete partial refresh
parameter:
		Xstart 	:   X direction Start coordinates
		Ystart  :   Y direction Start coordinates
		Xend    :   X direction end coordinates
		Yend    :   Y direction end coordinates
        Image   ：  Image data
******************************************************************************/
void AMOLED_1IN8_DisplayWindows(uint32_t Xstart, uint32_t Ystart, uint32_t Xend, uint32_t Yend, UWORD *Image) {
    
    if(Yend > AMOLED_1IN8.HEIGHT) Yend = AMOLED_1IN8.HEIGHT;
    if(Xend > AMOLED_1IN8.WIDTH) Xend = AMOLED_1IN8.WIDTH;

    // Send command in one-line mode
    // QSPI_1Wrie_Mode(&qspi);
    AMOLED_1IN8_SetWindows(Xstart, Ystart, Xend, Yend);
    QSPI_Select(qspi);
    QSPI_Pixel_Write(qspi, 0x2c);

    // Four-wire mode sends RGB data
    // QSPI_4Wrie_Mode(&qspi);
    channel_config_set_dreq(&c, pio_get_dreq(qspi.pio, qspi.sm, true));

    int i;
    uint32_t pixel_offset;
    UBYTE *partial_image;
    for (i = Ystart; i < Yend - 1; i++) {
        pixel_offset = (i * AMOLED_1IN8.WIDTH + Xstart) * 2;
        partial_image = (UBYTE *)Image + pixel_offset;
        dma_channel_configure(dma_tx, 
                            &c,
                            &qspi.pio->txf[qspi.sm],  // Destination pointer (PIO TX FIFO)
                            partial_image,            // Source pointer (data buffer)
                            (Xend-Xstart)*2,          // Data length (unit: number of transmissions)
                            true);                    // Start transferring immediately

        // Waiting for DMA transfer to complete
        while(dma_channel_is_busy(dma_tx));
    }

    QSPI_Deselect(qspi);
}

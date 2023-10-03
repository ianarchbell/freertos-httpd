/* hw_config.c
Copyright 2021 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use
this file except in compliance with the License. You may obtain a copy of the
License at

   http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.
*/
/*

This file should be tailored to match the hardware design.

There should be one element of the spi[] array for each hardware SPI used.

There should be one element of the sd_cards[] array for each SD card slot.
The name is should correspond to the FatFs "logical drive" identifier.
(See http://elm-chan.org/fsw/ff/doc/filename.html#vol)
In general, this should correspond to the (zero origin) array index.
The rest of the constants will depend on the type of
socket, which SPI it is driven by, and how it is wired.

*/

#include <string.h>
//
#include "my_debug.h"
//
#include "hw_config.h"
//
#include "ff.h" /* Obtains integer types */
//
#include "diskio.h" /* Declarations of disk functions */

/* 
See https://docs.google.com/spreadsheets/d/1BrzLWTyifongf_VQCc2IpJqXWtsrjmG7KnIbSBy-CPU/edit?usp=sharing,
tab "Monster", for pin assignments assumed in this configuration file.
*/

/* Tested
    0: SPI: OK, 12 MHz only
       SDIO: OK at 31.25 MHz
    1: SPI: OK at 20.8 MHz
    2: SPI: OK at 20.8 MHz
    3: SDIO: OK at 20.8 MHz (not at 31.25 MHz)
    4: SDIO: OK at 20.8 MHz (not at 31.25 MHz)
*/

// SD Card Pins on S1 board
/*

SPI     SDIO    GPIO
CLK     CLK     10
SI      CMD     11  MOSI
SO      D0      12  MISO
        D1      13
        D2      14
CS      D3      15        
        CD      NC

*/

// Hardware Configuration of SPI "objects"
// Note: multiple SD cards can be driven by one SPI if they use different slave
// selects.
static spi_t spis[] = {  // One for each SPI.
    {
        .hw_inst = spi1,  // SPI component
        .miso_gpio = 12,  // GPIO number (not Pico pin number)
        .mosi_gpio = 11,
        .sck_gpio = 10,
        .set_drive_strength = true,
        .mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        .sck_gpio_drive_strength = GPIO_DRIVE_STRENGTH_2MA,
        .no_miso_gpio_pull_up = true,

        // .baud_rate = 1000 * 1000
        .baud_rate = 12500 * 1000
        // .baud_rate = 25 * 1000 * 1000 // Actual frequency: 20833333.
    }
};

/* SPI Interfaces */
static sd_spi_if_t spi_ifs[] = {
    {   // spi_ifs[0]
        .spi = &spis[0],  // Pointer to the SPI driving this card
        .ss_gpio = 15,     // The SPI slave select GPIO for this SD card
        .set_drive_strength = true,
        .ss_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA
    }
};

// static sd_sdio_if_t sdio_ifs[] = {
//     /*
//         Pins CLK_gpio, D1_gpio, D2_gpio, and D3_gpio are at offsets from pin D0_gpio.
//         The offsets are determined by sd_driver\SDIO\rp2040_sdio.pio.
//             CLK_gpio = (D0_gpio + SDIO_CLK_PIN_D0_OFFSET) % 32;
//             As of this writing, SDIO_CLK_PIN_D0_OFFSET is 30,
//               which is -2 in mod32 arithmetic, so:
//             CLK_gpio = D0_gpio -2.
//             D1_gpio = D0_gpio + 1;
//             D2_gpio = D0_gpio + 2;
//             D3_gpio = D0_gpio + 3;
//      */
//     {
//         .CMD_gpio = 11,
//         .D0_gpio = 12,
//         .SDIO_PIO = pio1,
//         .DMA_IRQ_num = DMA_IRQ_1,
//         .baud_rate = 15 * 1000 * 1000  // 15 MHz
//     }           
// };

static sd_card_t sd_cards[] = {  // One for each SD card
    {   // sd_cards[0]: Socket sd0
        .pcName = "0:",  // Name used to mount device
        .type = SD_IF_SPI,
        .spi_if_p = &spi_ifs[0],  // Pointer to the SPI interface driving this card
        // SD Card detect:
        .use_card_detect = false,                 
    }
};

// Hardware Configuration of the SD Card "objects"
// static sd_card_t sd_cards[] = {  // One for each SD card
//     {        // Socket sd0 SDIO
//         .pcName = "1:",  // Name used to mount device
//         .type = SD_IF_SDIO,
//         .sdio_if_p = &sdio_ifs[0],
//         .use_card_detect = false,

//         // SD Card detect:
        
//         // .card_detect_gpio = 9,  
//         // .card_detected_true = 0, // What the GPIO read returns when a card is
//         //                          // present.
//         // .card_detect_use_pull = true,
//         // .card_detect_pull_hi = true
//     }

//     // assertion "0 == gpio_get(sd_card_p->spi_if_p->ss_gpio)" failed

// };

/* ********************************************************************** */
size_t sd_get_num() { return count_of(sd_cards); }
sd_card_t *sd_get_by_num(size_t num) {
    if (num <= sd_get_num()) {
        return &sd_cards[num];
    } else {
        return NULL;
    }
}
size_t spi_get_num() { return count_of(spis); }
spi_t *spi_get_by_num(size_t num) {
    if (num <= spi_get_num()) {
        return &spis[num];
    } else {
        return NULL;
    }
}

/* [] END OF FILE */

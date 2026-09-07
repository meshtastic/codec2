/*
 * Writes the codec2 codebook blob to external QSPI flash, once.
 *
 * Modelled on meshtastic/nrf52_factory_erase: do the work in setup(), then
 * enter DFU. Handing control back to the bootloader is what stops this running
 * a second time - the board comes up as a mass-storage drive expecting the real
 * firmware, so there is no marker to leave behind and no state to get wrong.
 *
 * Failure is the exception: on a bad write we stay up and keep reporting,
 * because rebooting into DFU would look identical to success.
 *
 * Board agnostic: the QSPI pins and the peripheral power rail come from
 * platformio.ini as CB_* defines, so a new target is a handful of flags rather
 * than a copied board definition.
 */
#include <Arduino.h>
// Required, and not only for the symbols: the include is what makes the
// framework's bundled TinyUSB the one that gets linked, so Serial is the
// same CDC object the core's TinyUSB_Device_Init() brings up.
#include <Adafruit_TinyUSB.h>
#include <nrf_gpio.h>
#include <nrfx_qspi.h>
#include <string.h>

#include "codebook_blob.h" // generated: ../make_codebook_blob.py --c-array

// The core enables NRFX_QSPI but omits this the way it supplies it for POWER,
// SPIS and TEMP.
#ifndef NRFX_QSPI_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_QSPI_DEFAULT_CONFIG_IRQ_PRIORITY 7
#endif

#include <nrfx_qspi.c>

#ifndef CB_QSPI_SCK
#error "set CB_QSPI_SCK/CS/IO0..IO3 in platformio.ini for this board"
#endif

#define QSPI_XIP_BASE 0x12000000UL
#define BLOB_OFFSET 0
#define ERASE_BLOCK 4096

// EasyDMA source buffer: the QSPI peripheral can only read from RAM.
static uint8_t ramChunk[4096];

static void say(const char *msg)
{
    Serial.println(msg);
    Serial.flush();
}

// No LED: boards differ on whether they even have one this build could drive,
// so failure reports over serial and stays put rather than rebooting into DFU,
// which would look identical to success.
static void fail(const char *msg)
{
    for (;;) {
        say(msg);
        delay(1000);
    }
}

// Absolute nRF pin numbers, like nrfx_qspi takes. pinMode()/digitalWrite() would
// route through the board variant's g_ADigitalPinMap instead, which is only the
// identity map on some boards - and driving the wrong pin here means the flash
// never gets power.
static void cbWrite(uint32_t pin, bool high)
{
    nrf_gpio_cfg_output(pin);
    nrf_gpio_pin_write(pin, high ? 1 : 0);
}

// The QSPI chip can sit behind a load switch. Boards that have one set
// CB_POWER_EN; the toggle brings the regulator up from a known state rather
// than a half-enabled one left by the previous reset.
static void powerUpPeripherals()
{
#ifdef CB_GPS_RF_EN
    cbWrite(CB_GPS_RF_EN, false); // hold the GPS front end off across the toggle
#endif
#ifdef CB_POWER_EN
    cbWrite(CB_POWER_EN, true);
    delay(100);
    cbWrite(CB_POWER_EN, false);
    delay(100);
    cbWrite(CB_POWER_EN, true);
    delay(100);
#endif
}

void setup()
{
    // Enumerate before anything that can block, so a hang is still diagnosable
    // over serial instead of a board that simply vanishes from USB.
    Serial.begin(115200);
    for (uint32_t t = millis(); !Serial && millis() - t < 3000;) {
    }
    delay(1500);

    say("codec2 codebook provisioning");

    say("powering peripherals");
    powerUpPeripherals();

    say("qspi init");
    nrfx_qspi_config_t cfg =
        NRFX_QSPI_DEFAULT_CONFIG(CB_QSPI_SCK, CB_QSPI_CS, CB_QSPI_IO0, CB_QSPI_IO1, CB_QSPI_IO2, CB_QSPI_IO3);
    cfg.phy_if.sck_freq = NRF_QSPI_FREQ_DIV4; // conservative; this runs once

    if (nrfx_qspi_init(&cfg, NULL, NULL) != NRFX_SUCCESS)
        fail("QSPI init failed - check CB_QSPI_* and CB_POWER_EN for this board");

    say("erasing");
    for (uint32_t off = 0; off < CODEBOOK_BLOB_LEN; off += ERASE_BLOCK)
        if (nrfx_qspi_erase(NRF_QSPI_ERASE_LEN_4KB, BLOB_OFFSET + off) != NRFX_SUCCESS)
            fail("QSPI erase failed");

    say("writing");
    // EasyDMA cannot read internal flash, and the blob is a const array that
    // lives there, so each chunk is staged through RAM. Lengths must also be
    // 4-byte aligned; the tail padding is past the blob and never read.
    for (uint32_t off = 0; off < CODEBOOK_BLOB_LEN; off += sizeof(ramChunk)) {
        uint32_t n = CODEBOOK_BLOB_LEN - off;
        if (n > sizeof(ramChunk))
            n = sizeof(ramChunk);
        memset(ramChunk, 0xff, sizeof(ramChunk));
        memcpy(ramChunk, CODEBOOK_BLOB + off, n);
        const uint32_t padded = (n + 3u) & ~3u;
        if (nrfx_qspi_write(ramChunk, padded, BLOB_OFFSET + off) != NRFX_SUCCESS)
            fail("QSPI write failed");
    }

    say("verifying through XIP");
    // XIP is the path the firmware will actually use - a read-back over the
    // same DMA that wrote it would prove less.
    const uint8_t *xip = (const uint8_t *)(QSPI_XIP_BASE + BLOB_OFFSET);
    if (memcmp(xip, CODEBOOK_BLOB, CODEBOOK_BLOB_LEN) != 0)
        fail("verify failed - blob does not read back");

    Serial.print("wrote and verified ");
    Serial.print(CODEBOOK_BLOB_LEN);
    Serial.println(" bytes, rebooting to DFU for the firmware");
    Serial.flush();
    delay(200);

    enterUf2Dfu();
}

void loop() {}

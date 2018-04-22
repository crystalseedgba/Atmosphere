#ifndef __FUSEE_SDMMC_H__
#define __FUSEE_SDMMC_H__

#include <stdbool.h>
#include <stdint.h>
#include "utils.h"

struct PACKED tegra_sdmmc {

    /* SDHCI standard registers */
    uint32_t dma_address;
    uint16_t block_size;
    uint16_t block_count;
    uint32_t argument;
    uint16_t transfer_mode;
    uint16_t command;
    uint16_t response[0x8];
    uint32_t buffer;
    uint32_t present_state;
    uint8_t host_control;
    uint8_t power_control;
    uint8_t block_gap_control;
    uint8_t wake_up_control;
    uint16_t clock_control;
    uint8_t timeout_control;
    uint8_t software_reset;
    uint32_t int_status;
    uint32_t int_enable;
    uint32_t signal_enable;
    uint16_t acmd12_err;
    uint16_t host_control2;
    uint32_t capabilities;
    uint32_t capabilities_1;
    uint32_t max_current;
    uint32_t _0x4c;
    uint16_t set_acmd12_error;
    uint16_t set_int_error;
    uint16_t adma_error;
    uint8_t _0x55[0x3];
    uint32_t adma_address;
    uint32_t upper_adma_address;
    uint16_t preset_for_init;
    uint16_t preset_for_default;
    uint16_t preset_for_high;
    uint16_t preset_for_sdr12;
    uint16_t preset_for_sdr25;
    uint16_t preset_for_sdr50;
    uint16_t preset_for_sdr104;
    uint16_t preset_for_ddr50;
    uint8_t _0x70[0x3];
    uint32_t _0x74[0x22];
    uint16_t slot_int_status;
    uint16_t host_version;

    /* vendor specific registers */
    uint32_t vendor_clock_cntrl;
    uint32_t vendor_sys_sw_cntrl;
    uint32_t vendor_err_intr_status;
    uint32_t vendor_cap_overrides;
    uint32_t vendor_boot_cntrl;
    uint32_t vendor_boot_ack_timeout;
    uint32_t vendor_boot_dat_timeout;
    uint32_t vendor_debounce_count;
    uint32_t vendor_misc_cntrl;
    uint32_t max_current_override;
    uint32_t max_current_override_hi;
    uint32_t _0x12c[0x21];
    uint32_t vendor_io_trim_cntrl;

    /* start of sdmmc2/sdmmc4 only */
    uint32_t vendor_dllcal_cfg;
    uint32_t vendor_dll_ctrl0;
    uint32_t vendor_dll_ctrl1;
    uint32_t vendor_dllcal_cfg_sta;
    /* end of sdmmc2/sdmmc4 only */

    uint32_t vendor_tuning_cntrl0;
    uint32_t vendor_tuning_cntrl1;
    uint32_t vendor_tuning_status0;
    uint32_t vendor_tuning_status1;
    uint32_t vendor_clk_gate_hysteresis_count;
    uint32_t vendor_preset_val0;
    uint32_t vendor_preset_val1;
    uint32_t vendor_preset_val2;
    uint32_t sdmemcomppadctrl;
    uint32_t auto_cal_config;
    uint32_t auto_cal_interval;
    uint32_t auto_cal_status;
    uint32_t io_spare;
    uint32_t sdmmca_mccif_fifoctrl;
    uint32_t timeout_wcoal_sdmmca;
    uint32_t _0x1fc;
};


/**
 * Primary data structure describing a Fusée MMC driver.
 */
struct mmc {
    char *name;

    volatile struct tegra_sdmmc *regs;
    volatile struct tegra_sdmmc *b_regs;
};


/**
 * SDMMC controllers
 */
enum sdmmc_controller {
    SWITCH_MICROSD = 0,
    SWITCH_EMMC = 3
};

/**
 * Initiailzes an SDMMC controller for use with an eMMC or SD card device.
 *
 * @param mmc An (uninitialized) structure for the MMC device.
 * @param controller The controller number to be initialized. Either SWITCH_MICROSD or SWITCH_EMMC.
 */
int sdmmc_init(struct mmc *mmc, enum sdmmc_controller controller);

/**
 * Reads a sector or sectors from a given SD card.
 */
int sdmmc_read(struct mmc *mmc, void *buffer, uint32_t sector, unsigned int count);


#endif

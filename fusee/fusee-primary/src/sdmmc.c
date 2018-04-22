#include <string.h>

#include "sdmmc.h"
#include "car.h"
#include "timers.h"
#include "apb_misc.h"
#include "lib/printk.h"

#define TEGRA_SDMMC_BASE (0x700B0000)
#define TEGRA_SDMMC_SIZE (0x200)

/**
 * Debug print for SDMMC information.
 */
void mmc_print(struct mmc *mmc, char *fmt, ...)
{
    va_list list;

    // TODO: check SDMMC log level before printing

    va_start(list, fmt);
    printk("%s: ", mmc->name);
    vprintk(fmt, list);
    printk("\n");
    va_end(list);
}




/**
 * Retreives the SDMMC register range for the given controller.
 */
static struct tegra_sdmmc *sdmmc_get_regs(enum sdmmc_controller controller)
{
    // Start with the base addresss of the SDMMC_BLOCK
    uintptr_t addr = TEGRA_SDMMC_BASE;

    // Offset our address by the controller number.
    addr += (controller * TEGRA_SDMMC_SIZE);

    // Return the controller.
    return (struct tegra_sdmmc *)addr;
}

/**
 * Set up a new SDMMC driver.
 *
 * @param mmc The SDMMC structure to be initiailized with the device state.
 * @param controler The controller description to be used; usually SWITCH_EMMC
 *      or SWTICH_MICROSD.
 */
int sdmmc_init(struct mmc* mmc, enum sdmmc_controller controller)
{
    volatile struct tegra_car *car = car_get_regs();
    volatile struct tegra_sdmmc *regs;

    uint32_t timebase;
    bool is_timeout;

    /* XXX fixme XXX */
    bool is_hs400_hs667 = false;

    mmc->name = "eMMC";
    mmc_print(mmc, "initializing in %s-speed mode...", is_hs400_hs667 ? "low" : "high");

    // Get a reference to the registers for the relevant SDMMC controller.
    regs = mmc->regs = sdmmc_get_regs(controller);

    // FIXME: set up clock and reset to fetch the relevant clock register offsets

    // Put SDMMC4 in reset
    car->rst_dev_l_set |= 0x8000;

    // Set SDMMC4 clock source (PLLP_OUT0) and divisor (1)
    car->clk_src[CLK_SOURCE_SDMMC4] = CLK_SOURCE_FIRST | CLK_DIVIDER_UNITY;

    // Set SDMMC4 clock enable
    car->clk_dev_l_set |= 0x8000;

    // host_clk_delay(0x64, clk_freq) -> Delay 100 host clock cycles
    udelay(5000);

    // Take SDMMC4 out of reset
    car->rst_dev_l_clr |= 0x8000;

    // Set IO_SPARE[19] (one cycle delay)
    regs->io_spare |= 0x80000;

    // Clear SEL_VREG
    regs->vendor_io_trim_cntrl &= ~(0x04);

    // Set trimmer value to 0x08 (SDMMC4)
    regs->vendor_clock_cntrl &= ~(0x1F000000);
    regs->vendor_clock_cntrl |= 0x08000000;

    // Set SDMMC2TMC_CFG_SDMEMCOMP_VREF_SEL to 0x07
    regs->sdmemcomppadctrl &= ~(0x0F);
    regs->sdmemcomppadctrl |= 0x07;

    // Set auto-calibration PD/PU offsets
    regs->auto_cal_config = ((regs->auto_cal_config & ~(0x7F)) | 0x05);
    regs->auto_cal_config = ((regs->auto_cal_config & ~(0x7F00)) | 0x05);

    // Set PAD_E_INPUT_OR_E_PWRD (relevant for eMMC only)
    regs->sdmemcomppadctrl |= 0x80000000;

    // Wait one milisecond
    udelay(1000);

    // Set AUTO_CAL_START and AUTO_CAL_ENABLE
    regs->auto_cal_config |= 0xA0000000;

    // Wait one second
    udelay(1);

    // Program a timeout of 10ms
    is_timeout = false;
    timebase = get_time();

    // Wait for AUTO_CAL_ACTIVE to be cleared
    mmc_print(mmc, "initialing autocal...");
    while((regs->auto_cal_status & 0x80000000) && !is_timeout) {
        // Keep checking if timeout expired
        is_timeout = get_time_since(timebase) > 10000;
    }
    
    // AUTO_CAL_ACTIVE was not cleared in time
    if (is_timeout)
    {
        mmc_print(mmc, "autocal timed out!");

        // Set CFG2TMC_EMMC4_PAD_DRVUP_COMP and CFG2TMC_EMMC4_PAD_DRVDN_COMP
        APB_MISC_GP_EMMC4_PAD_CFGPADCTRL_0 = ((APB_MISC_GP_EMMC4_PAD_CFGPADCTRL_0 & ~(0x3F00)) | 0x1000);
        APB_MISC_GP_EMMC4_PAD_CFGPADCTRL_0 = ((APB_MISC_GP_EMMC4_PAD_CFGPADCTRL_0 & ~(0xFC)) | 0x40);
        
        // Clear AUTO_CAL_ENABLE
        regs->auto_cal_config &= ~(0x20000000);
    }

    mmc_print(mmc, "autocal complete.");

    // Clear PAD_E_INPUT_OR_E_PWRD (relevant for eMMC only)
    regs->sdmemcomppadctrl &= ~(0x80000000);
    
    // Set SDHCI_CLOCK_INT_EN
    regs->clock_control |= 0x01;
    
    // Program a timeout of 2000ms
    timebase = get_time();
    is_timeout = false;
    
    // Wait for SDHCI_CLOCK_INT_STABLE to be set
    mmc_print(mmc, "waiting for internal clock to stabalize...");
    while(!(regs->clock_control & 0x02) && !is_timeout) {
        // Keep checking if timeout expired
        is_timeout = get_time_since(timebase) > 2000000;
    }
    
    // Clock failed to stabilize
    if (is_timeout) {
        mmc_print(mmc, "clock never stabalized!");
        return -1;
    } else {
        mmc_print(mmc, "clock stabalized.");
    }

    // Clear upper 17 bits
    regs->host_control2 &= ~(0xFFFE0000);

    // Clear SDHCI_PROG_CLOCK_MODE
    regs->clock_control &= ~(0x20);

    // Set SDHCI_CTRL2_HOST_V4_ENABLE
    regs->host_control2 |= 0x1000;

    // SDHCI_CAN_64BIT must be set
    if (!(regs->capabilities & 0x10000000)) {
        mmc_print(mmc, "missing CAN_64bit capability");
        return -1;
    }

    // Set SDHCI_CTRL2_64BIT_ENABLE
    regs->host_control2 |= 0x2000;

    // Clear SDHCI_CTRL_SDMA and SDHCI_CTRL_ADMA2
    regs->host_control &= 0xE7;
    
    // Set the timeout to be the maximum value
    regs->timeout_control &= ~(0x0F);
    regs->timeout_control |= 0x0E;
    
    // Clear SDHCI_CTRL_4BITBUS and SDHCI_CTRL_8BITBUS
    regs->host_control &= 0xFD;
    regs->host_control &= 0xDF;

    // Set SDHCI_POWER_180
    regs->power_control &= 0xF1;
    regs->power_control |= 0x0A;

    regs->power_control |= 0x01;

    if (is_hs400_hs667)
    {
        // Set DQS_TRIM_VAL
        regs->vendor_cap_overrides &= ~(0x3F00);
        regs->vendor_cap_overrides |= 0x2800;
    }

    // Clear TAP_VAL_UPDATED_BY_HW
    regs->vendor_tuning_cntrl0 &= ~(0x20000);

    // Software tap value should be 0 for SDMMC4, but HS400/HS667 modes
    // must take this value from the tuning procedure
    uint32_t tap_value = is_hs400_hs667 ? 1 : 0;

    // Set TAP_VAL
    regs->vendor_clock_cntrl &= ~(0xFF0000);
    regs->vendor_clock_cntrl |= (tap_value << 16);

    // Clear SDHCI_CTRL_HISPD
    regs->host_control &= 0xFB;

    // Clear SDHCI_CTRL_VDD_180 
    regs->host_control2 &= ~(0x08);

    // Set SDHCI_DIVIDER and SDHCI_DIVIDER_HI
    // FIXME: divider SD if necessary
    regs->clock_control &= ~(0xFFC0);
    //regs->clock_control |= ((sd_divider_lo << 0x08) | (sd_divider_hi << 0x06));

    // HS400/HS667 modes require additional DLL calibration
    if (is_hs400_hs667)
    {
        // Set CALIBRATE
        regs->vendor_dllcal_cfg |= 0x80000000;

        // Program a timeout of 5ms
        timebase = get_time();
        is_timeout = false;

        // Wait for CALIBRATE to be cleared
        mmc_print(mmc, "starting calibration...");
        while(regs->vendor_dllcal_cfg & 0x80000000 && !is_timeout) {
            // Keep checking if timeout expired
            is_timeout = get_time_since(timebase) > 5000;
        }

        // Failed to calibrate in time
        if (is_timeout) {
            mmc_print(mmc, "calibration failed!");
            return -1;
        }

        mmc_print(mmc, "calibration okay.");

        // Program a timeout of 10ms
        timebase = get_time();
        is_timeout = false;

        // Wait for DLL_CAL_ACTIVE to be cleared
        mmc_print(mmc, "waiting for calibration to finalize.... ");
        while((regs->vendor_dllcal_cfg_sta & 0x80000000) && !is_timeout) {
            // Keep checking if timeout expired
            is_timeout = get_time_since(timebase) > 10000;
        }

        // Failed to calibrate in time
        if (is_timeout) {
            mmc_print(mmc, "calibration failed to finalize!");
            return -1;
        }

        mmc_print(mmc, "calibration complete!");
    }

    // Set SDHCI_CLOCK_CARD_EN
    regs->clock_control |= 0x04;

    mmc_print(mmc, "initialized.");
    return 0;
}

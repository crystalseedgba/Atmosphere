#include "utils.h"
#include "hwinit.h"
#include "lib/printk.h"
#include "display/video_fb.h"

#include "fuse.h"
#include "hwinit/btn.h"


void puts_nl(const char *s)
{
  printk("%s\n", s);
}

static void reset_using_pmc()
{
    volatile uint32_t *reset;

    reset = (uint32_t *)0x7000e400;
    *reset |= (1 << 4);
}

int main(void) {
    u32 *lfb_base;
    u32 *irom_to_print = (u32 *)0x115A00;

    nx_hwinit();
    display_init();

    // Set up the display, and register it as a printk provider.
    lfb_base = display_init_framebuffer();
    video_init(lfb_base);

    puts_nl("                ");
    puts_nl("                ");
    puts_nl("                ");
    puts_nl("                                             *     .--.");
    puts_nl("                                                  / /  `");
    puts_nl("                                 +               | |");
    puts_nl("                                        '         \\ \\__,");
    puts_nl("                                    *          +   '--'  *");
    puts_nl("                                        +   /\\ ");
    puts_nl("                           +              .'  '.   *");
    puts_nl("                                  *      /======\\      +");
    puts_nl("                                        ;:.  _   ;");
    puts_nl("                                        |:. (_)  |");
    puts_nl("                                        |:.  _   |");
    puts_nl("                              +         |:. (_)  |          *");
    puts_nl("                                        ;:.      ;");
    puts_nl("                                      .' \\:.    / `.");
    puts_nl("                                     / .-'':._.'`-. \\ ");
    puts_nl("                                     |/    /||\\    \\|");
    puts_nl("                                   _..--\"\"\"````\"\"\"--.._");
    puts_nl("                             _.-'``                    ``'-._");
    puts_nl("                           -'                                '-");
    puts_nl("                    __      __                 _ _       _              _ ");
    puts_nl("                   / /      \\ \\               (_) |     | |            | |");
    puts_nl("                  | |_ __ ___| |  _____      ___| |_ ___| |__   ___  __| |");
    puts_nl("                 / /| '__/ _ \\\\ \\/ __\\ \\ /\\ / / | __/ __| '_ \\ / _ \\/ _` |");
    puts_nl("                 \\ \\| | |  __// /\\__ \\\\ V  V /| | || (__| | | |  __/ (_| |");
    puts_nl("                  | |_|  \\___| | |___/ \\_/\\_/ |_|\\__\\___|_| |_|\\___|\\__,_|");
    puts_nl("                   \\_\\      /_/                                           ");

    // Say hello.
    printk("\n\n\n");
    printk(" Hello, NVIDIA & Nintendo!\n");
    printk(" I'm running from the early-bootROM context.\n\n");

    // Read the fuses.


    // Print the fuses.
    printk(" This unit's SBK/DK is: %08x%08x%08x%08x / %08x!\n\n",
        FUSE_CHIP_REGS->FUSE_PRIVATE_KEY[0],
        FUSE_CHIP_REGS->FUSE_PRIVATE_KEY[1],
        FUSE_CHIP_REGS->FUSE_PRIVATE_KEY[2],
        FUSE_CHIP_REGS->FUSE_PRIVATE_KEY[3],
        FUSE_CHIP_REGS->FUSE_DEVICE_KEY
        );

    // print some irom for fun
    printk(" And here's a bit of your protected IROM (at %p):\n   ", irom_to_print);
    print_hex(irom_to_print, 512);
    printk("\n\n\n\n\n");

    // credits
    printk("                          vulnerability discovered & responsibly reported by @ktemkin\n");
    printk("                                          Kate Temkin -- k@ktemkin.com\n");

    // Wait for the power button, and then reset.
    while(btn_read() != BTN_POWER);

    // Reset.
    reset_using_pmc();

    /* Do nothing for now */
    return 0;
}

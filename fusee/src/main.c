#include "utils.h"
#include "hwinit.h"
#include "lib/printk.h"
#include "display/video_fb.h"

#include "fuse.h"
#include "hwinit/btn.h"
#include "hwinit/i2c.h"

void puts_nl(const char *s)
{
  printk("%s\n", s);
}

int main(void) {
    u32 *lfb_base;

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

    const int kI2cBus = 0;
    const int kBq24193SlaveAddress = 0x6b;

    i2c_init(kI2cBus);

    // Make sure I2C works and we're talking to the battery charger.
    uint8_t part_info;
    i2c_recv_buf_small(&part_info, sizeof(part_info),
		       kI2cBus, kBq24193SlaveAddress, 0x0a);
    if (part_info != 0x2f) {
      printk("ERROR: charger part number mismatch got: "
	     "0x%02x want: 0x%02x\n\n   ", part_info, 0x2f);
      while(1);
    }

    // Disable watchdog.
    uint8_t reg5;
    i2c_recv_buf_small(&reg5, sizeof(reg5), kI2cBus,
		       kBq24193SlaveAddress, 0x05);
    reg5 &= ~((1 << 4 | 1 << 5));
    i2c_send_buf_small(kI2cBus, kBq24193SlaveAddress,
		       0x05, &reg5, sizeof(reg5));

    // Disable BATFET.
    uint8_t reg7;
    i2c_recv_buf_small(&reg7, sizeof(reg7), kI2cBus,
		       kBq24193SlaveAddress, 0x07);
    reg7 |= (1 << 5);
    i2c_send_buf_small(kI2cBus, kBq24193SlaveAddress, 0x07,
		       &reg7, sizeof(reg7));
    printk("\n\nBATFET is now disabled, disconnect the USB cable.\n\n");
    while(1);

    return 0;
}

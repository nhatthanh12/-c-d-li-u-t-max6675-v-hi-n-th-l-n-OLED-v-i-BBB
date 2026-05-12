#include <linux/module.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define CONTROL_MODULE_BASE 0x44E10000

/* --- TỌA ĐỘ THANH GHI CHUẨN XÁC BBB REV C --- */
#define CONF_MCD_CS         0x99C  // P9.28
#define CONF_MCD_MISO       0x994  // P9.29
#define CONF_MCD_SCLK       0x998  // P9.31

/* --- SỐ CHÂN GPIO CHUẨN XÁC --- */
#define PIN_CS   113  // P9.28
#define PIN_MISO 111  // P9.29
#define PIN_SCLK 110  // P9.31

static int major;
static struct class *max_class;
void __iomem *base_addr;

static uint16_t read_max6675_soft(void) {
    int i;
    uint16_t data = 0;
    
    gpio_set_value(PIN_CS, 0); 
    udelay(10); 

    for (i = 15; i >= 0; i--) {
        gpio_set_value(PIN_SCLK, 1);
        udelay(10);
        
        // Đọc từ chân P9.29 (PIN 111)
        if (gpio_get_value(PIN_MISO)) {
            data |= (1 << i);
        }
            
        gpio_set_value(PIN_SCLK, 0);
        udelay(10);
    }
    
    gpio_set_value(PIN_CS, 1);
    return data;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t len, loff_t *off) {
    uint16_t val = read_max6675_soft();
    uint8_t res[2];
    
    pr_info("NHATTHANH: MISO Pin 111 Raw Data = 0x%04X\n", val);
    
    res[0] = (val >> 8) & 0xFF;
    res[1] = val & 0xFF;
    return copy_to_user(buf, res, 2) ? -EFAULT : 2;
}

static struct file_operations fops = { .owner = THIS_MODULE, .read = dev_read };

static int __init max_init(void) {
    major = register_chrdev(0, "nhatthanh_max6675", &fops);
    max_class = class_create(THIS_MODULE, "nhatthanh_spi");
    device_create(max_class, NULL, MKDEV(major, 0), NULL, "nhatthanh_max6675");

    /* Phá khóa Thanh ghi đúng tọa độ */
    base_addr = ioremap(CONTROL_MODULE_BASE, 0x2000);
    if (base_addr) {
        iowrite32(0x37, base_addr + CONF_MCD_MISO); // Mode 7, RX Active, Pull-up
        iowrite32(0x07, base_addr + CONF_MCD_SCLK); // Mode 7, TX
        iowrite32(0x07, base_addr + CONF_MCD_CS);   // Mode 7, TX
        iounmap(base_addr);
    }

    /* Chiếm quyền GPIO chuẩn */
    gpio_free(PIN_SCLK); gpio_free(PIN_MISO); gpio_free(PIN_CS);
    
    gpio_request(PIN_SCLK, "SCLK"); gpio_direction_output(PIN_SCLK, 0);
    gpio_request(PIN_CS, "CS");     gpio_direction_output(PIN_CS, 1);
    gpio_request(PIN_MISO, "MISO"); gpio_direction_input(PIN_MISO);

    pr_info("NHATTHANH: Driver Loaded (CS:113, MISO:111, SCLK:110)\n");
    return 0;
}

static void __exit max_exit(void) {
    device_destroy(max_class, MKDEV(major, 0));
    class_destroy(max_class);
    unregister_chrdev(major, "nhatthanh_max6675");
    gpio_free(PIN_SCLK); gpio_free(PIN_MISO); gpio_free(PIN_CS);
}

module_init(max_init);
module_exit(max_exit);
MODULE_LICENSE("GPL");

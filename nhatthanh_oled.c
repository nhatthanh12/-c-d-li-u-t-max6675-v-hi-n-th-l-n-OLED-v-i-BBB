#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/slab.h>

#define DRIVER_NAME "nhatthanh_oled"
#define SSD1306_I2C_ADDR 0x3C

/* Pin Mux cho I2C2 trên BeagleBone Black */
#define CONTROL_MODULE_BASE 0x44E10000
#define CONF_I2C2_SDA       0x978  // P9.20
#define CONF_I2C2_SCL       0x97C  // P9.19

static int major;
static struct class *oled_class;
static struct i2c_client *ssd1306_client;
static struct i2c_client *oled_client_device; // Thực thể thiết bị tự động

/* --- GIAO TIẾP I2C THẤP CẤP --- */

static int oled_write_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd}; 
    return i2c_master_send(ssd1306_client, buf, 2);
}

static int oled_write_data(uint8_t data) {
    uint8_t buf[2] = {0x40, data}; 
    return i2c_master_send(ssd1306_client, buf, 2);
}

/* --- KHỞI TẠO MÀN HÌNH --- */

static void oled_init_display(void) {
    oled_write_cmd(0xAE); // Display OFF
    oled_write_cmd(0xD5); oled_write_cmd(0x80); // Set Clock
    oled_write_cmd(0xA8); oled_write_cmd(0x3F); // Mux ratio
    oled_write_cmd(0xD3); oled_write_cmd(0x00); // Display offset
    oled_write_cmd(0x40); // Start line
    oled_write_cmd(0x8D); oled_write_cmd(0x14); // Charge pump (Bắt buộc)
    oled_write_cmd(0x20); oled_write_cmd(0x00); // Horizontal mode
    oled_write_cmd(0xA1); // Segment remap
    oled_write_cmd(0xC8); // COM scan direction
    oled_write_cmd(0xDA); oled_write_cmd(0x12); // COM pins hardware config
    oled_write_cmd(0x81); oled_write_cmd(0xCF); // Contrast
    oled_write_cmd(0xA4); // Resume RAM content
    oled_write_cmd(0xA6); // Normal display
    oled_write_cmd(0xAF); // Display ON
    
    // Xóa màn hình (Gửi 1024 byte 0 để clear RAM SSD1306)
    int i;
    for(i=0; i<1024; i++) oled_write_data(0x00);
    
    pr_info("NHATTHANH: OLED SSD1306 Reset & Initialized\n");
}

/* --- CHARACTER DEVICE OPERATIONS --- */

static ssize_t oled_write(struct file *file, const char __user *buf, size_t len, loff_t *off) {
    uint8_t *kbuf;
    int i;

    kbuf = kmalloc(len, GFP_KERNEL);
    if (!kbuf) return -ENOMEM;

    if (copy_from_user(kbuf, buf, len)) {
        kfree(kbuf);
        return -EFAULT;
    }

    for (i = 0; i < len; i++) {
        oled_write_data(kbuf[i]);
    }

    kfree(kbuf);
    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = oled_write,
};

/* --- I2C DRIVER PROBE & REMOVE --- */

static int oled_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    ssd1306_client = client;
    oled_init_display();
    return 0;
}

static int oled_remove(struct i2c_client *client) {
    pr_info("NHATTHANH: OLED Driver Removed\n");
    return 0;
}

static const struct i2c_device_id oled_id[] = { {DRIVER_NAME, 0}, {} };
MODULE_DEVICE_TABLE(i2c, oled_id);

static struct i2c_driver nhatthanh_i2c_driver = {
    .driver = { .name = DRIVER_NAME, .owner = THIS_MODULE },
    .probe = oled_probe,
    .remove = oled_remove,
    .id_table = oled_id,
};

/* --- MODULE INIT & EXIT --- */

static int __init oled_init(void) {
    void __iomem *base_addr;
    struct i2c_adapter *adapter;
    struct i2c_board_info oled_info = {
        I2C_BOARD_INFO(DRIVER_NAME, SSD1306_I2C_ADDR)
    };

    // 1. Character Device
    major = register_chrdev(0, DRIVER_NAME, &fops);
    oled_class = class_create(THIS_MODULE, "nhatthanh_oled_class");
    device_create(oled_class, NULL, MKDEV(major, 0), NULL, "nhatthanh_oled");

    // 2. Pin Muxing (I2C2: Mode 3, Pull-up, RX Active)
    base_addr = ioremap(CONTROL_MODULE_BASE, 0x2000);
    if (base_addr) {
        iowrite32(0x33, base_addr + CONF_I2C2_SDA); 
        iowrite32(0x33, base_addr + CONF_I2C2_SCL);
        iounmap(base_addr);
    }

    // 3. Đăng ký I2C Driver
    i2c_add_driver(&nhatthanh_i2c_driver);

    // 4. Tự động gắn thiết bị vào I2C Bus 2
    adapter = i2c_get_adapter(2); 
    if (adapter) {
        oled_client_device = i2c_new_client_device(adapter, &oled_info);
        i2c_put_adapter(adapter);
    }

    pr_info("NHATTHANH: Driver & Device loaded (Bus 2, Addr 0x3C)\n");
    return 0;
}

static void __exit oled_exit(void) {
    if (oled_client_device) i2c_unregister_device(oled_client_device);
    i2c_del_driver(&nhatthanh_i2c_driver);
    device_destroy(oled_class, MKDEV(major, 0));
    class_destroy(oled_class);
    unregister_chrdev(major, DRIVER_NAME);
}

module_init(oled_init);
module_exit(oled_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nhat Thanh");
MODULE_DESCRIPTION("Full SSD1306 I2C Driver for BBB");

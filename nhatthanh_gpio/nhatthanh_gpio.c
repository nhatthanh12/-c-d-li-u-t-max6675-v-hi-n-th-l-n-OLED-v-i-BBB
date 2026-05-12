#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/device.h>

#define DEVICE_NAME "nhatthanh_gpio"
#define CLASS_NAME  "nhatthanh_class"
#define DEBOUNCE_TIME 50 // Giữ 50ms để bắt nhịp bấm nhanh mượt mà

static int led_pin, btn_pin;
static int major;
static int irq_num;
static unsigned long last_interrupt_time = 0;
static int last_btn_state = -1; 

static struct class* nhatthanh_class  = NULL;
static struct device* nhatthanh_device = NULL;

/* ==========================================
 * HÀM XỬ LÝ NGẮT (Chống dội bằng Jiffies)
 * ========================================== */
static irqreturn_t btn_irq_handler(int irq, void *dev_id) {
    unsigned long current_time = jiffies;
    int current_state;

    // 1. Kiểm tra thời gian chống dội
    if (time_after(current_time, last_interrupt_time + msecs_to_jiffies(DEBOUNCE_TIME))) {
        current_state = gpio_get_value(btn_pin);

        // 2. CHỈ IN LOG khi trạng thái thực sự thay đổi (0->1 hoặc 1->0)
        if (current_state != last_btn_state) {
            pr_info("NHATTHANH: Ngat! Nut bam: %d\n", current_state);
            last_btn_state = current_state;
        }

        last_interrupt_time = current_time;
    }
    return IRQ_HANDLED;
}

/* ==========================================
 * CÁC HÀM READ / WRITE TỪ USER SPACE
 * ========================================== */
static ssize_t dev_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    char s[2];
    if (*ppos > 0) return 0;
    
    s[0] = gpio_get_value(btn_pin) ? '1' : '0';
    s[1] = '\n';
    
    if (copy_to_user(buf, s, 2)) return -EFAULT;
    
    *ppos = 2;
    return 2;
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
    char kbuf;
    if (copy_from_user(&kbuf, buf, 1)) return -EFAULT;
    
    gpio_set_value(led_pin, (kbuf == '1'));
    return count;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = dev_read,
    .write = dev_write,
};

/* ==========================================
 * HÀM PROBE (Đã fix Resource Leak & Error Handling)
 * ========================================== */
static int nhatthanh_probe(struct platform_device *pdev) {
    struct device *dev = &pdev->dev;
    int ret;

    // 1. Lấy GPIO từ Device Tree và kiểm tra
    led_pin = of_get_named_gpio(dev->of_node, "led-gpios", 0);
    if (!gpio_is_valid(led_pin)) {
        pr_err("NHATTHANH: Khong tim thay led-gpios trong DT!\n");
        return -ENODEV;
    }

    btn_pin = of_get_named_gpio(dev->of_node, "button-gpios", 0);
    if (!gpio_is_valid(btn_pin)) {
        pr_err("NHATTHANH: Khong tim thay button-gpios trong DT!\n");
        return -ENODEV;
    }
    
    // 2. Request GPIO
    ret = gpio_request(led_pin, "BTL_LED");
    if (ret) {
        pr_err("NHATTHANH: Khong the request LED GPIO\n");
        return ret;
    }
    gpio_direction_output(led_pin, 0);

    ret = gpio_request(btn_pin, "BTL_BTN");
    if (ret) {
        pr_err("NHATTHANH: Khong the request Button GPIO\n");
        goto err_gpio_led; // Nếu lỗi, nhả led_pin
    }
    gpio_direction_input(btn_pin);

    // 3. Request IRQ
    irq_num = gpio_to_irq(btn_pin);
    ret = request_irq(irq_num, btn_irq_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "nhatthanh_btn_irq", NULL);
    if (ret) {
        pr_err("NHATTHANH: Khong the request IRQ %d\n", irq_num);
        goto err_gpio_btn; // Nếu lỗi, nhả cả btn_pin và led_pin
    }

    // 4. Đăng ký Character Device
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        pr_err("NHATTHANH: Loi register_chrdev\n");
        ret = major;
        goto err_irq;
    }

    // 5. Tạo Class
    nhatthanh_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(nhatthanh_class)) {
        ret = PTR_ERR(nhatthanh_class);
        goto err_chrdev;
    }

    // 6. Tạo Device
    nhatthanh_device = device_create(nhatthanh_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(nhatthanh_device)) {
        ret = PTR_ERR(nhatthanh_device);
        goto err_class;
    }

    pr_info("NHATTHANH: Driver Plug&Play MUOT MA san sang tren Kernel 5.10!\n");
    return 0;

// --- KHOẢNG DỌN DẸP KHI CÓ LỖI (GOTO LABELS) ---
err_class:
    class_destroy(nhatthanh_class);
err_chrdev:
    unregister_chrdev(major, DEVICE_NAME);
err_irq:
    free_irq(irq_num, NULL);
err_gpio_btn:
    gpio_free(btn_pin);
err_gpio_led:
    gpio_free(led_pin);
    return ret;
}

/* ==========================================
 * HÀM REMOVE
 * ========================================== */
static int nhatthanh_remove(struct platform_device *pdev) {
    device_destroy(nhatthanh_class, MKDEV(major, 0));
    class_destroy(nhatthanh_class);
    unregister_chrdev(major, DEVICE_NAME);
    
    // Dọn dẹp tài nguyên phần cứng
    free_irq(irq_num, NULL);
    gpio_free(led_pin);
    gpio_free(btn_pin);
    
    pr_info("NHATTHANH: Da go bo Driver an toan.\n");
    return 0; 
}

/* ==========================================
 * KHAI BÁO MODULE VÀ TƯƠNG THÍCH DEVICE TREE
 * ========================================== */
static const struct of_device_id nhatthanh_ids[] = {
    { .compatible = "nhatthanh,gpio-system" },
    { }
};
MODULE_DEVICE_TABLE(of, nhatthanh_ids); // Bổ sung macro này để Kernel tự động map khi cắm thiết bị

static struct platform_driver nhatthanh_driver = {
    .probe = nhatthanh_probe,
    .remove = nhatthanh_remove,
    .driver = { 
        .name = "nhatthanh_gpio_drv", 
        .of_match_table = nhatthanh_ids 
    },
};

module_platform_driver(nhatthanh_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nhat Thanh");
MODULE_DESCRIPTION("Driver GPIO Toi uu Debounce & Memory Leak cho Kernel 5.10");

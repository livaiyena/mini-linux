#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "telemetry_sensor"
#define CLASS_NAME  "telemetry_class"

static int major_number;
static struct class*  telemetry_class  = NULL;
static struct device* telemetry_device = NULL;

static ssize_t telemetry_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset) {
    int error_count = 0;
    char dummy_data[64];
    int len;

    len = snprintf(dummy_data, sizeof(dummy_data), "Telemetry - Temp: 24.5C, Pressure: 1013hPa\n");

    if (*offset >= len) {
        return 0; 
    }

    if (size > len - *offset) {
        size = len - *offset;
    }

    error_count = copy_to_user(user_buffer, dummy_data + *offset, size);

    if (error_count == 0) {
        *offset += size;
        return size;
    } else {
        return -EFAULT;
    }
}

static struct file_operations telemetry_fops = {
    .owner = THIS_MODULE,
    .read = telemetry_read,
};

static int telemetry_probe(struct i2c_client *client) {
    pr_info("Telemetry Sensor: Probing I2C device (Address: 0x%x)\n", client->addr);

    major_number = register_chrdev(0, DEVICE_NAME, &telemetry_fops);
    if (major_number < 0) {
        pr_alert("Telemetry Sensor: Failed to register a major number\n");
        return major_number;
    }

    telemetry_class = class_create(CLASS_NAME);
    if (IS_ERR(telemetry_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        pr_alert("Telemetry Sensor: Failed to register device class\n");
        return PTR_ERR(telemetry_class);
    }

    telemetry_device = device_create(telemetry_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(telemetry_device)) {
        class_destroy(telemetry_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        pr_alert("Telemetry Sensor: Failed to create the device node\n");
        return PTR_ERR(telemetry_device);
    }

    pr_info("Telemetry Sensor: Device successfully created and initialized.\n");
    return 0;
}

static void telemetry_remove(struct i2c_client *client) {
    pr_info("Telemetry Sensor: Removing I2C device\n");

    device_destroy(telemetry_class, MKDEV(major_number, 0));
    class_unregister(telemetry_class);
    class_destroy(telemetry_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    
    pr_info("Telemetry Sensor: Module cleanly removed.\n");
}

static const struct of_device_id telemetry_of_match[] = {
    { .compatible = "custom,telemetry-sensor", },
    { }
};
MODULE_DEVICE_TABLE(of, telemetry_of_match);

static struct i2c_driver telemetry_driver = {
    .driver = {
        .name = DEVICE_NAME,
        .of_match_table = telemetry_of_match,
    },
    .probe = telemetry_probe,
    .remove = telemetry_remove,
};

module_i2c_driver(telemetry_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Telemetry Team");
MODULE_DESCRIPTION("A dummy I2C telemetry sensor LKM with character device interface");
MODULE_VERSION("0.1");

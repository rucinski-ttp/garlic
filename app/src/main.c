#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
    printk("Hello World from nRF52-DK!\n");
    
    int counter = 0;
    while (1) {
        printk("Counter: %d\n", counter++);
        k_msleep(1000);
    }
    return 0;
}
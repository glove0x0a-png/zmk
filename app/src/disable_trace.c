#include <nrfx.h>
#include <hal/nrf_uicr.h>
#include <zephyr/drivers/gpio.h>

static int disable_trace_pins(void)
{
    // TRACECONFIG レジスタを 0 にして TRACE を無効化
    NRF_UICR->TRACECONFIG = 0;

    return 0;
}

SYS_INIT(disable_trace_pins, PRE_KERNEL_1, 0);


static int force_gpio_init(void)
{
    const struct device *gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));

    gpio_pin_configure(gpio0, 7, GPIO_INPUT | GPIO_PULL_UP);
    gpio_pin_configure(gpio0, 8, GPIO_INPUT | GPIO_PULL_UP);

    return 0;
}

SYS_INIT(force_gpio_init, PRE_KERNEL_1, 1);

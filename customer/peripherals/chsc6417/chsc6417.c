/********************************chsc6417 tp********************************/
#include <rtdevice.h>
#include <rtthread.h>

#include "board.h"
#include "chsc6417.h"
#include "drv_io.h"
#include "drv_touch.h"
#include "string.h"

/* GPIO pin definitions for CHSC6417 */
#define CHSC6417_TP_RST 70
#define CHSC6417_TP_INT 80

/* Debug configuration */
#define DBG_LEVEL DBG_ERROR // DBG_LOG //
#define LOG_TAG "drv.chsc6417"
#include <drv_log.h>
#define DEBUG_PRINTF(...) rt_kprintf(__VA_ARGS__)

/* Driver instance variables */
static struct touch_drivers chsc6417tp_driver;
static rt_device_t chsc6417tp_i2c_bus = RT_NULL;

static uint8_t handshake_flag = 0;
static uint8_t tp_inited = 0;

static sm_touch_dev st_dev;

/**
 * @brief Write data to CHSC6417 via I2C
 * @param buf: Data buffer to write
 * @param len: Length of data
 * @return RT_EOK on success, -RT_ERROR on failure
 */
static rt_err_t chsc6417tp_i2c_write(uint8_t *buf, uint16_t len)
{
    rt_int8_t res = 0;
    struct rt_i2c_msg msgs[1];

    msgs[0].addr = CHSC6417_I2C_ADDR; /* slave address */
    msgs[0].flags = 0x5C;             /* write flag */
    msgs[0].buf = buf;                /* send data pointer */
    msgs[0].len = len;

    if (rt_i2c_transfer(
            (struct rt_i2c_bus_device *)chsc6417tp_i2c_bus->user_data, msgs,
            1) == 1)
    {
        res = RT_EOK;
    }
    else
    {
        res = -RT_ERROR;
    }
    return res;
}

/**
 * @brief I2C transfer function for CHSC6417
 * @param reg: Register buffer to write
 * @param wlen: Write buffer length
 * @param rbuf: Read buffer
 * @param rlen: Read buffer length
 * @return RT_EOK on success, -RT_ERROR on failure
 */
static rt_size_t chsc6417tp_i2c_xfer(uint8_t *reg, uint16_t wlen, uint8_t *rbuf,
                                     uint16_t rlen)
{
    struct rt_i2c_msg msgs[2];
    rt_size_t res = 0;

    msgs[0].addr = CHSC6417_I2C_ADDR; /* slave address */
    msgs[0].flags = 0x5C;             /* write flag */
    msgs[0].buf = reg;                /* send data pointer */
    msgs[0].len = wlen;

    msgs[1].addr = CHSC6417_I2C_ADDR; /* slave address */
    msgs[1].flags = 0x5D;             /* read flag */
    msgs[1].buf = rbuf;               /* receive data pointer */
    msgs[1].len = rlen;

    if (rt_i2c_transfer(
            (struct rt_i2c_bus_device *)chsc6417tp_i2c_bus->user_data, msgs,
            2) == 2)
    {
        res = RT_EOK;
    }
    else
    {
        res = -RT_ERROR;
    }

    return res;
}

/**
 * @brief Read bytes from CHSC6417 registers
 * @param reg: Register address (must be 4B aligned)
 * @param buffer: Data buffer
 * @param len: Data length (must be 4B aligned)
 * @return 0 on success, -1 on failure
 */
int32_t semi_touch_read_bytes(uint32_t reg, uint8_t *buffer, uint16_t len)
{
    int32_t ret = RT_EOK;
    uint32_t once;
    uint8_t writeBuff[4];

    while (len > 0)
    {
        once = (len > MAX_IO_BUFFER_LEN) ? MAX_IO_BUFFER_LEN : len;
        writeBuff[0] = (uint8_t)(reg >> 24);
        writeBuff[1] = (uint8_t)(reg >> 16);
        writeBuff[2] = (uint8_t)(reg >> 8);
        writeBuff[3] = (uint8_t)(reg);

        if (chsc6417tp_i2c_xfer(writeBuff, 4, buffer, (uint16_t)once) != RT_EOK)
        {
            ret = -1;
            break;
        }

        reg += once;
        buffer += once;
        len -= once;
    }

    return ret;
}

/**
 * @brief Write bytes to CHSC6417 registers
 * @param reg: Register address (must be 4B aligned)
 * @param buffer: Data buffer
 * @param len: Data length (must be 4B aligned)
 * @return 0 on success, -1 on failure
 */
int32_t semi_touch_write_bytes(uint32_t reg, uint8_t *buffer, uint16_t len)
{
    int32_t ret = RT_EOK;
    uint8_t writeBuff[MAX_IO_BUFFER_LEN];
    uint32_t once, k;

    while (len > 0)
    {
        once = (len < (MAX_IO_BUFFER_LEN - 4)) ? len : (MAX_IO_BUFFER_LEN - 4);

        writeBuff[0] = (uint8_t)(reg >> 24);
        writeBuff[1] = (uint8_t)(reg >> 16);
        writeBuff[2] = (uint8_t)(reg >> 8);
        writeBuff[3] = (uint8_t)(reg);

        for (k = 0; k < once; k++)
        {
            writeBuff[k + 4] = buffer[k];
        }

        if (chsc6417tp_i2c_write(writeBuff, (uint16_t)once + 4) != RT_EOK)
        {
            ret = -1;
            break;
        }

        reg += once;
        buffer += once;
        len -= once;
    }

    return ret;
}

/**
 * @brief Reset CHSC6417 according to datasheet timing requirements
 */
void semi_touch_reset(void)
{
    /* According to datasheet section 8: Power on reset sequence */
    /* RSTN must be low before power on (Trtp > 100us) */
    BSP_TP_Reset(0);
    HAL_Delay(2); /* Hold reset low for at least 1ms (Tvdr) */

    BSP_TP_Reset(1);
    HAL_Delay(50); /* Wait for chip to be ready - increased delay */

    rt_kprintf("chsc6417: reset complete\r\n");
}

/**
 * @brief Detect CHSC6417 presence by reading chip ID
 * @return 0 on success, -1 on failure
 */
int32_t semi_touch_dect(void)
{
    uint32_t retry;
    uint8_t u32Data[4] = {0};
    uint32_t chip_id = 0;

    for (retry = 0; retry < 3; retry++)
    {
        semi_touch_reset();
        HAL_Delay(100); /* Wait longer for chip to be ready after reset */

        /* Read chip ID from CHSC6417 */
        if (!semi_touch_read_bytes(CHSC6417_REG_CHIP_ID, u32Data, 4))
        {
            chip_id = (u32Data[3] << 24) | (u32Data[2] << 16) |
                      (u32Data[1] << 8) | u32Data[0];
            rt_kprintf(
                "chsc6417: detected - ID: 0x%08x (raw: %02x %02x %02x %02x)\n",
                chip_id, u32Data[0], u32Data[1], u32Data[2], u32Data[3]);

            /* Check if chip ID is valid (non-zero) */
            if (chip_id != 0)
            {
                return 0;
            }
            else
            {
                rt_kprintf("chsc6417: invalid chip ID, retrying...\n");
            }
        }
        else
        {
            rt_kprintf("chsc6417: I2C read failed\n");
        }
    }

    rt_kprintf("chsc6417: detection failed after %d retries\n", retry);
    return -1;
}

/**
 * @brief Setup and check CHSC6417 firmware status
 */
void semi_touch_setup_check(void)
{
    int32_t retry = 0;
    uint32_t naFlag = 0;
    img_header_t image_header;
    img_header_t image_confirm;

    /* Clean boot status */
    semi_touch_write_bytes(CHSC6417_REG_STATUS, (uint8_t *)&naFlag, 4);

    semi_touch_reset();

    /* Initialize device structure */
    st_dev.fw_ver = 0;
    st_dev.vid_pid = 0;
    st_dev.setup_ok = 0; /* Default status is failed */
    st_dev.channels = CHSC6417_MAX_CHANNELS;
    st_dev.max_points = CHSC6417_MAX_POINTS;

    image_header.sig = 0;

    for (retry = 0; retry < 10; retry++)
    {
        HAL_Delay(10);

        if (semi_touch_read_bytes(CHSC6417_REG_FW_INFO,
                                  (uint8_t *)&image_header,
                                  sizeof(image_header)) != 0)
        {
            continue;
        }

        HAL_Delay(3);

        /* Double check for data consistency */
        if (semi_touch_read_bytes(CHSC6417_REG_FW_INFO,
                                  (uint8_t *)&image_confirm,
                                  sizeof(image_confirm)) != 0)
        {
            continue;
        }

        if ((image_header.sig != image_confirm.sig) ||
            (image_header.vid_pid != image_confirm.vid_pid) ||
            (image_header.raw_offet != image_confirm.raw_offet) ||
            (image_header.dif_offet != image_confirm.dif_offet) ||
            (image_header.fw_ver != image_confirm.fw_ver))
        {
            rt_kprintf("chsc6417: double check failed, retry\n");
            continue;
        }

        if (image_header.sig == 0x43534843) /* "CHSC" */
        {
            st_dev.fw_ver = image_header.fw_ver;
            st_dev.vid_pid = image_header.vid_pid;
            st_dev.setup_ok = 1; /* Pass */
            rt_kprintf("chsc6417: setup OK - FW Ver: 0x%04x, VID_PID: 0x%08x\n",
                       st_dev.fw_ver, st_dev.vid_pid);
            break;
        }
        else if (image_header.sig == 0x4F525245) /* Boot self check fail */
        {
            rt_kprintf("chsc6417: boot self check failed\n");
            break;
        }
    }

    return;
}

/**
 * @brief CHSC6417 interrupt handler
 */
void chsc6417tp_irq_handler(void *arg)
{
    rt_err_t ret = RT_ERROR;

    rt_touch_irq_pin_enable(0);

    ret = rt_sem_release(chsc6417tp_driver.isr_sem);
    RT_ASSERT(RT_EOK == ret);
}

/**
 * @brief Get touch points from CHSC6417
 * @param p_msg: Touch message structure
 * @return RT_EOK on success, RT_ERROR on failure, RT_EEMPTY when touch down
 */
static rt_err_t semi_touch_get_points(touch_msg_t p_msg)
{
    uint8_t cmd = 0xE0; // “read touch data” command
    uint8_t data[3];    // reads 3 bytes
    rt_err_t ret;

    rt_touch_irq_pin_enable(1);

    /* 1) Send the 0xE0 register address */
    ret = chsc6417tp_i2c_write(&cmd, 1);
    if (ret != RT_EOK)
    {
        rt_kprintf("chsc6417: cmd write failed\n");
        goto _up;
    }

    rt_thread_mdelay(1);

    /* 2) Read back 3 bytes */
    ret = chsc6417tp_i2c_xfer(&cmd, 1, data, 3);
    if (ret != RT_EOK)
    {
        rt_kprintf("chsc6417: data read failed\n");
        goto _up;
    }

    /* Parse */
    uint8_t pts = data[0] & 0x03; // lower 2 bits = number of points
    if (pts == 0)
    {
        p_msg->event = TOUCH_EVENT_UP;
        p_msg->x = p_msg->y = 0;
        return RT_EOK;
    }

    /* data[0] bits 6 & 7 are the high bits of X and Y */
    p_msg->event = TOUCH_EVENT_DOWN;
    p_msg->x = (((data[0] & 0x40) >> 6) << 8) | data[1];
    p_msg->y = (((data[0] & 0x80) >> 7) << 8) | data[2];
    return RT_EEMPTY;

_up:
    p_msg->event = TOUCH_EVENT_UP;
    p_msg->x = p_msg->y = 0;
    return RT_ERROR;
}

/**
 * @brief Initialize CHSC6417 touch controller
 */
static rt_err_t semi_touch_init(void)
{
    rt_err_t ret = RT_EOK;
    uint32_t status = 0;

    rt_kprintf("chsc6417: initializing...\n");

    /* Detect CHSC6417 presence */
    if (semi_touch_dect())
    {
        rt_kprintf("chsc6417: not detected\n");
        return RT_EIO;
    }

    /* Check firmware and setup */
    semi_touch_setup_check();

    /* Check and update firmware if needed */
    // if (st_dev.setup_ok)
    // {
    //     semi_check_and_update(&st_dev);
    // }

    /* Reset after setup */
    semi_touch_reset();

    /* Wait for power-on time according to datasheet (Tpon max 200ms) */
    HAL_Delay(100);

    /* Write to status register to indicate we're ready */
    status = 0x43534843; /* "CHSC" */
    semi_touch_write_bytes(CHSC6417_REG_STATUS, (uint8_t *)&status, 4);

    /* Enable interrupt - CHSC6417 INT is active low */
    rt_touch_irq_pin_attach(PIN_IRQ_MODE_FALLING, chsc6417tp_irq_handler,
                            RT_NULL);
    rt_touch_irq_pin_enable(1);

    rt_kprintf("chsc6417: initialization complete\n");

    return ret;
}

/**
 * @brief Deinitialize CHSC6417
 */
static rt_err_t deinit(void)
{
    rt_kprintf("chsc6417: deinitializing\n");

    rt_touch_irq_pin_enable(0);

    /* Put CHSC6417 into suspend mode to save power */
    /* TODO: Add suspend mode command if needed */

    return RT_EOK;
}

/**
 * @brief Probe for CHSC6417 on I2C bus
 */
static rt_bool_t probe(void)
{
    rt_err_t err;

    /* Find I2C bus device */
    chsc6417tp_i2c_bus = rt_device_find(TOUCH_DEVICE_NAME);
    if (chsc6417tp_i2c_bus)
    {
        rt_device_open(chsc6417tp_i2c_bus, RT_DEVICE_FLAG_RDWR |
                                               RT_DEVICE_FLAG_INT_TX |
                                               RT_DEVICE_FLAG_INT_RX);
    }
    else
    {
        rt_kprintf("chsc6417: I2C bus not found\n");
        return RT_FALSE;
    }

    /* Configure I2C parameters */
    {
        struct rt_i2c_configuration configuration = {
            .mode = 0,
            .addr = 0,
            .timeout = 5000,
            .max_hz = 200000, /* 200kHz I2C speed */
        };

        rt_i2c_configure(
            (struct rt_i2c_bus_device *)chsc6417tp_i2c_bus->user_data,
            &configuration);
    }

    rt_kprintf("chsc6417: probe successful\n");

    return RT_TRUE;
}

/* Touch operations structure */
static struct touch_ops ops = {semi_touch_get_points, semi_touch_init, deinit};

/**
 * @brief Initialize CHSC6417 touch driver
 */
static int rt_chsc6417tp_init(void)
{
    chsc6417tp_driver.probe = probe;
    chsc6417tp_driver.ops = &ops;
    chsc6417tp_driver.user_data = RT_NULL;
    chsc6417tp_driver.isr_sem =
        rt_sem_create("chsc6417tp", 0, RT_IPC_FLAG_FIFO);

    rt_touch_drivers_register(&chsc6417tp_driver);

    rt_kprintf("chsc6417: driver registered\n");

    return 0;
}

INIT_COMPONENT_EXPORT(rt_chsc6417tp_init);

/**
 * @brief ESD (Electrostatic Discharge) handler for CHSC6417
 * Checks if touch controller is still responsive and resets if needed
 */
void touch_esd_handler(void)
{
    int32_t status = 0, retry = 0;

    for (retry = 0; retry < 3; retry++)
    {
        /* Check if CHSC6417 is still alive */
        semi_touch_read_bytes(CHSC6417_REG_STATUS, (uint8_t *)&status, 4);
        if (status == 0x43534843) /* "CHSC" */
        {
            /* Chip is alive, clear status and return */
            status = 0;
            semi_touch_write_bytes(CHSC6417_REG_STATUS, (uint8_t *)&status, 4);
            return;
        }

        /* Chip not responding, perform reset */
        semi_touch_reset();
        rt_thread_mdelay(5); /* Delay 5ms */
    }

    rt_kprintf("chsc6417: ESD recovery failed after %d retries\n", retry);
}

/**
 * @brief Initialize CHSC6417 touch panel (external API)
 */
void CHSC6417TP_INIT(void)
{
    /* This function can be called externally if needed */
    rt_chsc6417tp_init();
}

static int tp_tt(int argc, char **argv)
{
    struct touch_message msg;
    touch_msg_t touch_msg = &msg;

    rt_err_t result = semi_touch_get_points(touch_msg);

    if (result == RT_EOK)
    {
        rt_kprintf("Touch detected: x=%d, y=%d, event=%d\n", touch_msg->x,
                   touch_msg->y, touch_msg->event);
    }
    else
    {
        rt_kprintf("Touch read failed: %d\n", result);
    }

    return 0;
}
MSH_CMD_EXPORT(tp_tt, TouchpadTest);



/************************ (C) COPYRIGHT Sifli Technology *******END OF FILE****/
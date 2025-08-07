#ifndef __CHSC6417_H_
#define __CHSC6417_H_

//chsc6417 specific definitions
#define MAX_IO_BUFFER_LEN 32

/* CHSC6417 specific parameters */
#define CHSC6417_MAX_CHANNELS        17    // S1~S17 channels
#define CHSC6417_MAX_POINTS          2     // Up to 2-point touch
#define CHSC6417_I2C_ADDR            0x2E  /* I2C slave address */

/* Memory map for CHSC6417 */
#define TP_CMD_BUFF_ADDR             0x20000000
#define TP_RSP_BUFF_ADDR             0x20000000
#define TP_WR_BUFF_ADDR              0x20002000
#define TP_RD_BUFF_ADDR              0x20002400

/* CHSC6417 specific registers */
#define CHSC6417_REG_CHIP_ID         0x2000001C  // Chip ID register
#define CHSC6417_REG_STATUS          0x20000018  // Status register
#define CHSC6417_REG_FW_INFO         0x20000014  // Firmware info register
#define CHSC6417_REG_TOUCH_DATA      0x2000002C  // Touch data register

/* Power and timing parameters from datasheet */
#define CHSC6417_RESET_HOLD_TIME     1    // ms (Tvdr min)
#define CHSC6417_RESET_PULSE_TIME    1    // ms (Trst min = 500us)
#define CHSC6417_POWER_ON_TIME       200  // ms (Tpon max)
#define CHSC6417_POWER_DOWN_WAIT     2    // ms (Tpdt min)

/* CTP operation modes */
#define CTP_MODE_NORMAL              0x00
#define CTP_MODE_LOW_POWER           0x01
#define CTP_MODE_SUSPEND             0x02

/* CTP work status */
#define CTP_POINTING_WORK            0x00000000
#define CTP_READY_UPGRADE            (1 << 1)
#define CTP_UPGRAD_RUNING            (1 << 2)
#define CTP_SUSPEND_GATE             (1 << 16)
#define CTP_GUESTURE_GATE            (1 << 17)
#define CTP_PROXIMITY_GATE           (1 << 18)
#define CTP_GLOVE_GATE               (1 << 19)
#define CTP_ORIENTATION_GATE         (1 << 20)

/* Firmware update header structure */
struct chsc_updfile_header {
    uint32_t sig;
    uint32_t resv;
    uint32_t n_cfg;
    uint32_t n_match;
    uint32_t len_cfg;
    uint32_t len_boot;
};

/* Image header structure */
typedef struct _img_header_t
{
    uint16_t fw_ver;
    uint16_t resv;
    uint32_t sig;
    uint32_t vid_pid;
    uint16_t raw_offet;
    uint16_t dif_offet;
} img_header_t;

/* Touch device structure */
typedef struct sm_touch_dev
{
    uint32_t ctp_run_status;
    uint32_t vid_pid;    //0xVID_PID_CFGVER
    uint16_t fw_ver;
    uint8_t  setup_ok;
    uint8_t  channels;   // Number of active channels
    uint8_t  max_points; // Maximum touch points
} sm_touch_dev, *psm_touch_dev;

/* Command structure for mcap */
struct m_ctp_cmd_std_t
{
    uint16_t chk;  // 16 bit checksum
    uint16_t d0;   // data 0
    uint16_t d1;   // data 1
    uint16_t d2;   // data 2
    uint16_t d3;   // data 3
    uint16_t d4;   // data 4
    uint16_t d5;   // data 5
    uint8_t  id;   // offset 15
    uint8_t  tag;  // offset 16
};

/* Response structure for mcap */
struct m_ctp_rsp_std_t
{
    uint16_t chk;  // 16 bit checksum
    uint16_t d0;   // data 0
    uint16_t d1;   // data 1
    uint16_t d2;   // data 2
    uint16_t d3;   // data 3
    uint16_t d4;   // data 4
    uint16_t d5;   // data 5
    uint8_t  cc;   // offset 15
    uint8_t  id;   // offset 16
};

/* Error codes */
enum SEMI_DRV_ERR
{
    SEMI_DRV_ERR_OK = 0,
    SEMI_DRV_ERR_HAL_IO,
    SEMI_DRV_ERR_NO_INIT,
    SEMI_DRV_ERR_TIMEOUT,
    SEMI_DRV_ERR_CHECKSUM,
    SEMI_DRV_ERR_RESPONSE,
    SEMI_DRV_INVALID_CMD,
    SEMI_DRV_INVALID_PARAM,
    SEMI_DRV_ERR_NOT_MATCH,
};

/* Command type IDs */
enum CMD_TYPE_ID
{
    CMD_NA              = 0x0f,
    CMD_IDENTITY        = 0x01,
    CMD_CTP_SSCAN       = 0x02,
    CMD_CTP_IOCTL       = 0x03,
    CMD_MEM_WR          = 0x30,
    CMD_MEM_RD          = 0x31,
    CMD_FLASH_ERASE     = 0x32,
    CMD_FW_SUM          = 0x33,
    CMD_WRITE_REGISTER  = 0X35,
    CMD_READ_REGISTER   = 0X36,
    CMD_BSPR_WRITE      = 0x37,
    CMD_BSPR_READ       = 0x38,
};

/* MCU control addresses and values */
#define TP_HOLD_MCU_ADDR             0x40007000
#define TP_AUTO_FEED_ADDR            0x40007010
#define TP_REMAP_MCU_ADDR            0x40007000
#define TP_RELEASE_MCU_ADDR          0x40007000
#define TP_HOLD_MCU_VAL              0x12044000
#define TP_AUTO_FEED_VAL             0x0000925a
#define TP_REMAP_MCU_VAL             0x12044002
#define TP_RELEASE_MCU_VAL           0x12044003

#define VID_PID_BACKUP_ADDR          (40 * 1024 + 0x10)

/* Platform specific delay */
#define platform_delay_ms rt_thread_mdelay

/* Touch event structure for CHSC6417 */
struct chsc6417_touch_data {
    uint8_t  status;      // 0xFF for valid data
    uint8_t  points;      // Number of touch points (0-2)
    uint16_t x[2];        // X coordinates for up to 2 points
    uint16_t y[2];        // Y coordinates for up to 2 points
    uint8_t  id[2];       // Touch point IDs
};

/* Function prototypes */
int32_t semi_check_and_update(sm_touch_dev *st_dev);
void semi_touch_setup_check(void);
void CHSC6417TP_INIT(void);

#endif /* __CHSC6417_H_ */
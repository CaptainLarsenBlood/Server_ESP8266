#include "i2c_user.h"
#include "esp8266/ets_sys.h"
//------------------------------------------------
LOCAL uint8 m_nLastSDA;
LOCAL uint8 m_nLastSCL;
//------------------------------------------------
LOCAL void ICACHE_FLASH_ATTR
i2c_mas_setDC(uint8 SDA, uint8 SCL)
{
    SDA &= 0x01;
    SCL &= 0x01;
    m_nLastSDA = SDA;
    m_nLastSCL = SCL;

    if ((0 == SDA) && (0 == SCL)) {
        I2C_MASTER_SDA_LOW_SCL_LOW();
    } else if ((0 == SDA) && (1 == SCL)) {
        I2C_MASTER_SDA_LOW_SCL_HIGH();
    } else if ((1 == SDA) && (0 == SCL)) {
        I2C_MASTER_SDA_HIGH_SCL_LOW();
    } else {
        I2C_MASTER_SDA_HIGH_SCL_HIGH();
    }
}
//------------------------------------------------
LOCAL uint8 ICACHE_FLASH_ATTR
i2c_mas_getDC(void)
{
    uint8 sda_out;
    sda_out = GPIO_INPUT_GET(GPIO_ID_PIN(I2C_MASTER_SDA_GPIO));
    return sda_out;
}
//------------------------------------------------
void ICACHE_FLASH_ATTR
i2c_mas_start(void)
{
    i2c_mas_setDC(1, m_nLastSCL);
    i2c_mas_wait(5);
    i2c_mas_setDC(1, 1);
    i2c_mas_wait(5); // sda 1, scl 1
    i2c_mas_setDC(0, 1);
    i2c_mas_wait(5); // sda 0, scl 1
}
//------------------------------------------------
void ICACHE_FLASH_ATTR
i2c_mas_stop(void)
{
    i2c_mas_wait(5);

    i2c_mas_setDC(0, m_nLastSCL);
    i2c_mas_wait(5); // sda 0
    i2c_mas_setDC(0, 1);
    i2c_mas_wait(5); // sda 0, scl 1
    i2c_mas_setDC(1, 1);
    i2c_mas_wait(5); // sda 1, scl 1
}
//------------------------------------------------
void ICACHE_FLASH_ATTR
i2c_mas_setAck(uint8 level)
{
    i2c_mas_setDC(m_nLastSDA, 0);
    i2c_mas_wait(5);
    i2c_mas_setDC(level, 0);
    i2c_mas_wait(5); // sda level, scl 0
    i2c_mas_setDC(level, 1);
    i2c_mas_wait(8); // sda level, scl 1
    i2c_mas_setDC(level, 0);
    i2c_mas_wait(5); // sda level, scl 0
    i2c_mas_setDC(1, 0);
    i2c_mas_wait(5);
}
//------------------------------------------------
uint8 ICACHE_FLASH_ATTR
i2c_mas_getAck(void)
{
    uint8 retVal;
    i2c_mas_setDC(m_nLastSDA, 0);
    i2c_mas_wait(5);
    i2c_mas_setDC(1, 0);
    i2c_mas_wait(5);
    i2c_mas_setDC(1, 1);
    i2c_mas_wait(5);
    retVal = i2c_mas_getDC();
    i2c_mas_wait(5);
    i2c_mas_setDC(1, 0);
    i2c_mas_wait(5);
    return retVal;
}
//------------------------------------------------
void ICACHE_FLASH_ATTR
i2c_mas_send_ack(void)
{
    i2c_mas_setAck(0x0);
}
//------------------------------------------------
bool ICACHE_FLASH_ATTR
i2c_mas_checkAck(void)
{
    if(i2c_mas_getAck()){
        return FALSE;
    }else{
        return TRUE;
    }
}
//------------------------------------------------
void ICACHE_FLASH_ATTR
i2c_mas_writeByte(uint8 wrdata)
{
    uint8 dat;
    sint8 i;
    i2c_mas_wait(5);
    i2c_mas_setDC(m_nLastSDA, 0);
    i2c_mas_wait(5);
    for (i = 7; i >= 0; i--) {
        dat = wrdata >> i;
        i2c_mas_setDC(dat, 0);
        i2c_mas_wait(5);
        i2c_mas_setDC(dat, 1);
        i2c_mas_wait(5);
        if (i == 0) {
            i2c_mas_wait(3);
        }
        i2c_mas_setDC(dat, 0);
        i2c_mas_wait(5);
    }
}
//------------------------------------------------
void ICACHE_FLASH_ATTR
i2c_mas_init(void)
{
    uint8 i;
    i2c_mas_setDC(1, 0);
    i2c_mas_wait(5);
    // when SCL = 0, toggle SDA to clear up
    i2c_mas_setDC(0, 0) ;
    i2c_mas_wait(5);
    i2c_mas_setDC(1, 0) ;
    i2c_mas_wait(5);
    // set data_cnt to max value
    for (i = 0; i < 28; i++) {
        i2c_mas_setDC(1, 0);
        i2c_mas_wait(5); // sda 1, scl 0
        i2c_mas_setDC(1, 1);
        i2c_mas_wait(5); // sda 1, scl 1
    }
    // reset all
    i2c_mas_stop();
    return;
}
//------------------------------------------------
void ICACHE_FLASH_ATTR i2c_mas_gpio_init(void)
{
  ETS_INTR_LOCK();

  PIN_FUNC_SELECT(I2C_MASTER_SDA_MUX, I2C_MASTER_SDA_FUNC);
  PIN_FUNC_SELECT(I2C_MASTER_SCL_MUX, I2C_MASTER_SCL_FUNC);
  GPIO_REG_WRITE(GPIO_PIN_ADDR(GPIO_ID_PIN(I2C_MASTER_SDA_GPIO)), GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(I2C_MASTER_SDA_GPIO))) | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE)); //open drain;
  GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS, GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | (1 << I2C_MASTER_SDA_GPIO));
  GPIO_REG_WRITE(GPIO_PIN_ADDR(GPIO_ID_PIN(I2C_MASTER_SCL_GPIO)), GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(I2C_MASTER_SCL_GPIO))) | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE)); //open drain;
  GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS, GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | (1 << I2C_MASTER_SCL_GPIO));

  I2C_MASTER_SDA_HIGH_SCL_HIGH();

  ETS_INTR_UNLOCK();

  i2c_mas_init();
}
//------------------------------------------------
void I2C_SendByteByADDR(uint8_t c,uint8_t addr)
{
  uint16_t i;
  uint8_t ack;
  i2c_mas_start();
  //Transmit Address SLAVE
  i2c_mas_writeByte(addr);
  ack = i2c_mas_checkAck();
  if(!ack)
  {
    os_printf("ADDR not ack\r\n");
  }
  //Send data
  i2c_mas_writeByte(c);
  i2c_mas_send_ack();
  i2c_mas_stop();
}
//------------------------------------------------

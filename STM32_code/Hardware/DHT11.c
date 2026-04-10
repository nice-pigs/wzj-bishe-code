#include "stm32f10x.h"
#include "DHT11.h"
#include "Delay.h"

/*引脚配置 - DHT11数据引脚连接到PA0*/
#define DHT11_GPIO_PORT     GPIOA
#define DHT11_GPIO_PIN      GPIO_Pin_0
#define DHT11_GPIO_CLK      RCC_APB2Periph_GPIOA

/*引脚操作宏定义*/
#define DHT11_DATA_OUT(x)   GPIO_WriteBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN, (BitAction)(x))
#define DHT11_DATA_IN()     GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN)

/**
  * @brief  DHT11引脚配置为输出模式
  * @param  无
  * @retval 无
  */
static void DHT11_GPIO_OUT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;        // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure);
}

/**
  * @brief  DHT11引脚配置为输入模式
  * @param  无
  * @retval 无
  */
static void DHT11_GPIO_IN(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;           // 上拉输入
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure);
}

/**
  * @brief  DHT11初始化
  * @param  无
  * @retval 无
  */
void DHT11_Init(void)
{
    // 开启GPIO时钟
    RCC_APB2PeriphClockCmd(DHT11_GPIO_CLK, ENABLE);
    
    // 配置为输出模式，初始输出高电平
    DHT11_GPIO_OUT();
    DHT11_DATA_OUT(1);
}

/**
  * @brief  DHT11开始信号
  * @param  无
  * @retval 无
  */
static void DHT11_Start(void)
{
    DHT11_GPIO_OUT();           // 配置为输出
    DHT11_DATA_OUT(0);          // 拉低数据线
    Delay_ms(20);               // 保持至少18ms
    DHT11_DATA_OUT(1);          // 拉高数据线
    Delay_us(30);               // 等待20-40us
    DHT11_GPIO_IN();            // 配置为输入，等待DHT11响应
}

/**
  * @brief  等待DHT11响应
  * @param  无
  * @retval 0-成功, 1-失败
  */
static uint8_t DHT11_Check(void)
{
    uint8_t retry = 0;
    
    // 等待DHT11拉低（响应信号）
    while(DHT11_DATA_IN() && retry < 100)
    {
        retry++;
        Delay_us(1);
    }
    if(retry >= 100) {
        // printf("[DHT11_DEBUG] No response - timeout waiting for LOW\r\n");
        return 1;  // 超时，DHT11无响应
    }
    else retry = 0;
    
    // 等待DHT11拉高（准备发送数据）
    while(!DHT11_DATA_IN() && retry < 100)
    {
        retry++;
        Delay_us(1);
    }
    if(retry >= 100) {
        // printf("[DHT11_DEBUG] No response - timeout waiting for HIGH\r\n");
        return 1;  // 超时
    }
    
    return 0;   // 成功
}

/**
  * @brief  读取DHT11一个位
  * @param  无
  * @retval 读取到的位值 (0或1)
  */
static uint8_t DHT11_ReadBit(void)
{
    uint8_t retry = 0;
    
    // 等待低电平结束
    while(DHT11_DATA_IN() && retry < 100)
    {
        retry++;
        Delay_us(1);
    }
    retry = 0;
    
    // 等待高电平结束
    while(!DHT11_DATA_IN() && retry < 100)
    {
        retry++;
        Delay_us(1);
    }
    
    // 延时40us，如果此时还是高电平，则为1，否则为0
    Delay_us(40);
    
    if(DHT11_DATA_IN()) return 1;
    else return 0;
}

/**
  * @brief  读取DHT11一个字节
  * @param  无
  * @retval 读取到的字节数据
  */
static uint8_t DHT11_ReadByte(void)
{
    uint8_t i, data = 0;
    
    for(i = 0; i < 8; i++)
    {
        data <<= 1;
        data |= DHT11_ReadBit();
    }
    
    return data;
}

/**
  * @brief  读取DHT11温湿度数据
  * @param  temperature 温度数据指针
  * @param  humidity 湿度数据指针
  * @retval 0-成功, 1-失败(无响应), 2-失败(校验错误)
  */
uint8_t DHT11_Read(uint8_t *temperature, uint8_t *humidity)
{
    uint8_t buf[5];
    uint8_t i;
    
    DHT11_Start();      // 发送开始信号
    
    if(DHT11_Check() == 0)  // 检查DHT11响应
    {
        // 读取5个字节数据
        for(i = 0; i < 5; i++)
        {
            buf[i] = DHT11_ReadByte();
        }
        
        // 校验数据
        uint8_t checksum = buf[0] + buf[1] + buf[2] + buf[3];
        if(checksum == buf[4])
        {
            *humidity = buf[0];         // 湿度整数部分
            *temperature = buf[2];      // 温度整数部分
            // printf("[DHT11_DEBUG] Data: H=%d.%d, T=%d.%d, Checksum OK\r\n", buf[0], buf[1], buf[2], buf[3]);
            return 0;   // 读取成功
        }
        else
        {
            // 校验失败
            // printf("[DHT11_DEBUG] Checksum error: calc=%d, recv=%d\r\n", checksum, buf[4]);
            // printf("[DHT11_DEBUG] Raw data: %d %d %d %d %d\r\n", buf[0], buf[1], buf[2], buf[3], buf[4]);
            return 2;  // 校验错误
        }
    }
    
    // DHT11无响应
    // printf("[DHT11_DEBUG] No response from DHT11 sensor\r\n");
    return 1;   // 无响应
}
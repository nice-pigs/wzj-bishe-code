# 软件环境搭建指南

## 一、安装 Keil MDK-ARM

### 1. 下载软件

访问官网下载：https://www.keil.com/download/product/
选择 **MDK-ARM** 版本

### 2. 安装步骤

1. 运行安装程序
2. 选择安装路径（建议默认）
3. 填写用户信息
4. 安装完成

### 3. 安装 STM32F103 支持包

1. 打开 Keil
2. 点击菜单 `Project` → `Manage` → `Pack Installer`
3. 在左侧找到 `STMicroelectronics`
4. 展开，找到 `STM32F1 Series`
5. 点击 `Install` 安装

---

## 二、安装 ST-Link 驱动

### 1. 下载驱动

官网：https://www.st.com/zh/development-tools/stsw-link009.html

### 2. 安装步骤

1. 解压下载的文件
2. 运行 `dpinst_amd64.exe`（64位系统）
3. 按提示完成安装

### 3. 验证安装

1. 将 ST-Link 插入电脑 USB
2. 打开设备管理器
3. 查看 `通用串行总线设备` 下是否有 `STLink`

---

## 三、安装串口调试工具

### 推荐工具

- **SSCOM**（推荐）：简单好用
- **XCOM**：功能全面
- **Putty**：通用串口终端

### 下载地址

搜索 `SSCOM 串口调试助手` 下载

---

## 四、项目导入和配置

### 1. 创建 Keil 工程

由于完整工程文件较大，这里提供简化步骤：

#### 方法一：使用 STM32CubeMX 生成工程（推荐）

1. 下载安装 STM32CubeMX：https://www.st.com/zh/development-tools/stm32cubemx.html
2. 打开 STM32CubeMX
3. 选择 `File` → `New Project`
4. 选择芯片：`STM32F103C8`
5. 配置引脚：

```
系统配置：
├── SYS → Debug: Serial Wire
├── RCC → Crystal/Ceramic Resonator

引脚配置：
├── PA1  → GPIO_Input (红外传感器)
├── PA9  → USART1_TX (WiFi)
├── PA10 → USART1_RX (WiFi)
├── PB0  → GPIO_Output (WiFi RST)
├── PB1  → GPIO_Output (红色LED)
├── PB2  → GPIO_Output (绿色LED)
├── PB6  → I2C1_SCL (OLED)
├── PB7  → I2C1_SDA (OLED)
```

6. 点击 `Project` → `Generate Code`
7. 选择 Keil 作为 IDE

#### 方法二：下载现成模板

搜索 `STM32F103C8T6 Keil工程模板` 下载

### 2. 添加代码

将 `firmware/main.c` 中的代码复制到生成的工程中。

---

## 五、工程配置

### 1. 配置下载方式

1. 点击工具栏的 `Options for Target` 图标（魔术棒）
2. 切换到 `Debug` 标签
3. 选择 `ST-Link Debugger`
4. 点击 `Settings`
5. 确认 SW 模式，频率 4MHz

### 2. 配置串口

1. 在 `Options for Target` 中
2. 切换到 `Utilities` 标签
3. 选择 `ST-Link Debugger`
4. 点击 `Settings` → `Flash Download`
5. 勾选 `Reset and Run`

---

## 六、下载程序

### 1. 连接 ST-Link

```
ST-Link        STM32
────────────────────
SWDIO    →    SWDIO
SWCLK    →    SWCLK
GND      →    GND
3.3V     →    3.3V
```

### 2. 下载步骤

1. 点击 `Build` 按钮（或按 F7）编译工程
2. 确认无错误
3. 点击 `Download` 按钮（或按 F8）
4. 等待下载完成

### 3. 运行程序

下载完成后，STM32 会自动运行。

---

## 七、调试技巧

### 1. 使用 printf 调试

在 `main.c` 中添加重定向代码：

```c
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, 100);
    return ch;
}
```

然后可以用 `printf("test\n");` 输出调试信息。

### 2. 使用 Keil 内置调试器

1. 点击 `Start/Stop Debug Session` (Ctrl+F5)
2. 可以设置断点、单步执行、查看变量

### 3. 查看串口输出

用 USB转TTL 连接 PA2/PA3，在串口调试助手查看输出。

---

## 八、常见问题

### Q1: 下载失败，提示 "Flash Timeout"

**解决**：
1. 检查 ST-Link 连接
2. 尝试降低下载速度
3. 按住 RESET 按钮再点击下载

### Q2: 程序不运行

**解决**：
1. 检查 Boot 引脚配置（Boot0 = 0, Boot1 = 任意）
2. 检查复位电路
3. 检查供电电压

### Q3: 串口无输出

**解决**：
1. 检查波特率是否匹配
2. 检查 TX/RX 是否接反
3. 检查串口驱动是否安装

### Q4: 编译错误找不到头文件

**解决**：
1. 检查 Include Paths 是否正确
2. 确认 HAL 库是否正确添加

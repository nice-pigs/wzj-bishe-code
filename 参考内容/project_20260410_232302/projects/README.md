# 智能座位监测系统

> 基于STM32单片机和物联网技术的座位状态实时监测系统

## 项目简介

本系统是一个完整的物联网应用，能够自动检测座位的使用状态（占用/空闲），并将状态实时显示在Web界面上。系统包含硬件终端、云端服务和用户界面三个部分。

### 核心功能

- ✅ **实时状态检测**：通过压力/红外传感器检测座位占用状态
- ✅ **本地状态显示**：LED指示灯和OLED屏幕实时显示
- ✅ **云端数据上报**：设备状态自动上传至云端服务器
- ✅ **Web界面展示**：直观的座位状态可视化界面
- ✅ **历史记录查询**：查看座位使用历史和趋势分析

---

## 系统架构

```
┌─────────────────┐
│  硬件终端       │
│  (STM32+传感器) │
└────────┬────────┘
         │ Wi-Fi
         ↓
┌─────────────────┐
│  云端服务       │
│  (Next.js API)  │
└────────┬────────┘
         │ HTTP
         ↓
┌─────────────────┐
│  用户界面       │
│  (Web Dashboard)│
└─────────────────┘
```

---

## 技术栈

### 硬件端
- **主控芯片**：STM32F103C8T6 (ARM Cortex-M3)
- **传感器**：压力传感器 FSR402 / 红外传感器 HC-SR501
- **通信模块**：ESP8266 Wi-Fi模块
- **显示模块**：0.96寸 OLED屏幕 (SSD1306)

### 软件端
- **前端框架**：Next.js 16 + React 19
- **UI组件库**：shadcn/ui + Tailwind CSS
- **数据库**：PostgreSQL (Supabase)
- **图表库**：Recharts
- **开发语言**：TypeScript

---

## 项目结构

```
.
├── docs/                    # 文档目录
│   └── hardware-design.md   # 硬件设计文档
├── firmware/                # 单片机代码
│   └── main.c               # STM32主程序
├── src/                     # 前端源码
│   ├── app/                 # Next.js App Router
│   │   ├── api/             # API路由
│   │   │   ├── devices/     # 设备相关API
│   │   │   └── seats/       # 座位相关API
│   │   ├── layout.tsx       # 根布局
│   │   └── page.tsx         # 主页面
│   ├── components/          # React组件
│   │   └── ui/              # UI基础组件
│   └── storage/             # 数据存储
│       └── database/        # 数据库相关
├── package.json             # 项目依赖
└── README.md                # 项目说明
```

---

## 快速开始

### 前提条件

- Node.js 18+
- pnpm 8+
- STM32开发环境（Keil MDK或STM32CubeIDE）

### 安装步骤

1. **克隆项目**
```bash
git clone <repository-url>
cd seat-monitoring-system
```

2. **安装依赖**
```bash
pnpm install
```

3. **配置环境变量**

创建 `.env.local` 文件：
```env
# Supabase配置
NEXT_PUBLIC_SUPABASE_URL=your_supabase_url
NEXT_PUBLIC_SUPABASE_ANON_KEY=your_supabase_anon_key
```

4. **启动开发服务器**
```bash
pnpm dev
```

5. **访问应用**

打开浏览器访问 `http://localhost:5000`

---

## API文档

### 1. 设备注册

**POST** `/api/devices/register`

注册新的物联网设备。

**请求体：**
```json
{
  "deviceKey": "STM32-001",
  "deviceName": "座位监测设备-001"
}
```

**响应：**
```json
{
  "success": true,
  "data": {
    "deviceId": 1,
    "deviceKey": "STM32-001",
    "deviceName": "座位监测设备-001"
  }
}
```

### 2. 状态上报

**POST** `/api/seats/status`

上报座位当前状态。

**请求体：**
```json
{
  "deviceKey": "STM32-001",
  "isOccupied": true
}
```

**响应：**
```json
{
  "success": true,
  "data": {
    "seatId": 1,
    "seatName": "座位-001",
    "isOccupied": true
  }
}
```

### 3. 座位查询

**GET** `/api/seats`

查询所有座位状态。

**查询参数：**
- `location` (可选): 按位置筛选
- `status` (可选): 按状态筛选（occupied/idle）

**响应：**
```json
{
  "success": true,
  "data": [
    {
      "id": 1,
      "name": "座位-001",
      "location": "图书馆一楼",
      "isOccupied": false,
      "deviceStatus": "online",
      "lastUpdate": "2024-01-01T12:00:00Z"
    }
  ],
  "total": 1
}
```

### 4. 历史记录

**GET** `/api/seats/[id]/history`

查询指定座位的历史状态记录。

**查询参数：**
- `limit` (可选): 返回记录数量（默认50，最多200）
- `startTime` (可选): 开始时间（ISO格式）
- `endTime` (可选): 结束时间（ISO格式）

**响应：**
```json
{
  "success": true,
  "data": [
    {
      "id": 1,
      "seatId": 1,
      "isOccupied": true,
      "recordedAt": "2024-01-01T12:00:00Z"
    }
  ],
  "seat": {
    "id": 1,
    "name": "座位-001"
  },
  "total": 1
}
```

---

## 硬件开发指南

### 元器件清单

详细清单请查看 [docs/hardware-design.md](docs/hardware-design.md)

**核心元器件：**
- STM32F103C8T6 最小系统板
- 压力传感器 FSR402 或 红外传感器 HC-SR501
- ESP8266 Wi-Fi模块
- 0.96寸 OLED显示屏
- LED指示灯（红/绿）

**总成本：** 约 ¥100-150

### 电路连接

```
传感器 → STM32 (ADC/GPIO)
STM32 → ESP8266 (UART)
STM32 → OLED (I2C)
STM32 → LED (GPIO)
```

### 单片机程序开发

1. **开发环境搭建**
   - 安装 Keil MDK-ARM 或 STM32CubeIDE
   - 安装 STM32CubeMX（用于代码生成）
   - 安装 ST-Link 驱动程序

2. **编译和下载**
   - 打开 `firmware/main.c`
   - 根据实际硬件修改引脚配置
   - 编译并下载到STM32

3. **配置Wi-Fi参数**
   - 修改 `SERVER_IP` 和 `SERVER_PORT`
   - 修改 `DEVICE_KEY` 和 `DEVICE_NAME`
   - 修改 Wi-Fi SSID 和密码

详细说明请查看 `firmware/main.c` 文件注释。

---

## 部署指南

### 云端部署

本项目可以部署到任何支持Node.js的平台：

**Vercel（推荐）：**
```bash
# 安装Vercel CLI
pnpm add -g vercel

# 部署
vercel
```

**其他平台：**
- Railway
- Render
- AWS EC2
- 阿里云/腾讯云服务器

### 本地部署

```bash
# 构建
pnpm build

# 启动生产服务器
pnpm start
```

---

## 测试指南

### 功能测试

1. **设备注册测试**
```bash
curl -X POST http://localhost:5000/api/devices/register \
  -H "Content-Type: application/json" \
  -d '{"deviceKey":"TEST-001","deviceName":"测试设备"}'
```

2. **状态上报测试**
```bash
curl -X POST http://localhost:5000/api/seats/status \
  -H "Content-Type: application/json" \
  -d '{"deviceKey":"TEST-001","isOccupied":true}'
```

3. **座位查询测试**
```bash
curl http://localhost:5000/api/seats
```

### 性能测试

- 系统支持多个设备同时上报
- 数据库查询响应时间 < 100ms
- Web界面自动刷新间隔 10秒

---

## 常见问题

### Q1: 设备无法连接Wi-Fi？

**解决方案：**
1. 检查Wi-Fi名称和密码是否正确
2. 确保Wi-Fi信号强度足够
3. 检查ESP8266模块供电是否稳定
4. 尝试复位ESP8266模块

### Q2: 数据上报失败？

**解决方案：**
1. 检查服务器IP和端口配置
2. 确认服务器已启动并可访问
3. 检查设备是否已注册
4. 查看串口调试输出

### Q3: Web界面显示异常？

**解决方案：**
1. 清除浏览器缓存
2. 检查网络连接
3. 确认API服务正常运行
4. 查看浏览器控制台错误信息

### Q4: 传感器检测不准确？

**解决方案：**
1. 调整传感器阈值（PRESSURE_THRESHOLD）
2. 检查传感器安装位置
3. 增加采样次数和滤波算法
4. 考虑使用其他类型的传感器

---

## 扩展功能

### 已实现功能
- ✅ 实时状态监测
- ✅ 云端数据存储
- ✅ Web界面展示
- ✅ 历史记录查询

### 待扩展功能
- 🔲 微信小程序界面
- 🔲 座位预约功能
- 🔲 数据统计分析
- 🔲 异常报警通知
- 🔲 电池供电和低功耗设计
- 🔲 OTA远程升级

---

## 贡献指南

欢迎提交 Issue 和 Pull Request！

### 开发规范
- 代码风格：遵循 ESLint 配置
- 提交信息：遵循 Conventional Commits 规范
- 分支管理：feature/xxx, fix/xxx, docs/xxx

---

## 许可证

本项目仅供学习和研究使用。

---

## 参考文献

1. 刘火良, 杨森. STM32库开发实战指南[M]. 机械工业出版社.
2. 马洪连, 丁男. 物联网感知与控制技术[M]. 清华大学出版社.
3. 储继华, 梁民. 基于nRF52832单片机的图书馆自习室座位状态监测系统设计[J]. 微型电脑应用, 2021.
4. O. C. Daniel, et al. "Smart Library Seat, Occupant and Occupancy Information System" (NextComp 2019).

---

## 联系方式

如有问题，请提交 Issue 或联系项目维护者。

---

**最后更新：** 2024年

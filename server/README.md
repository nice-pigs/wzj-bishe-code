# 座位检测系统 - 本地服务器部署指南

## 系统架构

```
STM32 (座位检测)
    ↓ 串口 (UART3, 115200)
ESP32 (WiFi模块)
    ↓ HTTP POST
本地服务器 (Node.js, 端口3000)
    ↓ HTTP GET
Web前端 (浏览器)
```

## 安装步骤

### 1. 安装Node.js

**Windows:**
1. 访问 https://nodejs.org/
2. 下载LTS版本（推荐）
3. 运行安装程序
4. 验证安装：
   ```cmd
   node --version
   npm --version
   ```

**验证成功示例:**
```
v18.17.0
9.6.7
```

### 2. 安装依赖

打开命令提示符(CMD)或PowerShell，进入server目录：

```cmd
cd bishe/server
npm install
```

这会安装以下依赖：
- express: Web服务器框架
- cors: 跨域资源共享

### 3. 启动服务器

```cmd
npm start
```

看到以下输出表示启动成功：
```
========================================
  座位检测系统 - 本地服务器
========================================
[服务器] 运行在 http://localhost:3000
[前端页面] http://localhost:3000
[API文档] http://localhost:3000/api/health
========================================
[系统] 等待设备连接...
```

### 4. 配置ESP32

1. 打开 `ESP32_CloudUpload.ino`
2. 修改WiFi配置：
   ```cpp
   const char* ssid = "你的WiFi名称";
   const char* password = "你的WiFi密码";
   ```

3. 修改服务器地址（改成你电脑的IP）：
   ```cpp
   const char* serverURL = "http://192.168.1.100:3000/api/seat/update";
   const char* heartbeatURL = "http://192.168.1.100:3000/api/device/heartbeat";
   ```

4. 查看电脑IP地址：
   ```cmd
   ipconfig
   ```
   找到"IPv4 地址"，例如：192.168.1.100

5. 上传到ESP32

### 5. 访问Web界面

在浏览器中打开：
```
http://localhost:3000
```

## API接口文档

### 1. 健康检查
```
GET /api/health
```
响应：
```json
{
  "status": "ok",
  "timestamp": "2024-01-01T12:00:00.000Z",
  "uptime": 123.45
}
```

### 2. 获取座位状态
```
GET /api/seat/status
```
响应：
```json
{
  "success": true,
  "data": {
    "occupied": true,
    "temperature": 27,
    "humidity": 66,
    "lightLevel": 85,
    "duration": 120,
    "lastUpdate": "2024-01-01T12:00:00.000Z",
    "deviceOnline": true
  },
  "timestamp": "2024-01-01T12:00:00.000Z"
}
```

### 3. 上传座位数据（ESP32调用）
```
POST /api/seat/update
Content-Type: application/json

{
  "occupied": true,
  "temperature": 27,
  "humidity": 66,
  "lightLevel": 85,
  "duration": 120
}
```

### 4. 设备心跳（ESP32调用）
```
POST /api/device/heartbeat
Content-Type: application/json

{
  "timestamp": 12345678
}
```

### 5. 获取历史数据
```
GET /api/seat/history?limit=50
```

### 6. 获取统计信息
```
GET /api/seat/statistics
```

### 7. 获取设备信息
```
GET /api/device/info
```

## 测试步骤

### 1. 测试服务器
```cmd
# 启动服务器
npm start

# 在另一个终端测试API
curl http://localhost:3000/api/health
```

### 2. 测试ESP32上传
使用Postman或curl模拟ESP32上传数据：
```cmd
curl -X POST http://localhost:3000/api/seat/update ^
  -H "Content-Type: application/json" ^
  -d "{\"occupied\":true,\"temperature\":27,\"humidity\":66,\"lightLevel\":85,\"duration\":120}"
```

### 3. 测试前端
浏览器打开：http://localhost:3000

应该看到座位状态显示为"占用中"

## 故障排查

### 问题1：npm install失败

**解决方法：**
```cmd
# 清除缓存
npm cache clean --force

# 使用淘宝镜像
npm install --registry=https://registry.npmmirror.com
```

### 问题2：端口3000被占用

**解决方法：**
修改 `server.js` 中的端口号：
```javascript
const PORT = 3001;  // 改成其他端口
```

### 问题3：ESP32无法连接服务器

**检查项：**
1. 服务器是否启动？
2. ESP32和电脑在同一WiFi？
3. 防火墙是否阻止？
4. IP地址是否正确？

**解决方法：**
```cmd
# Windows防火墙添加规则
# 控制面板 → Windows Defender防火墙 → 高级设置 → 入站规则 → 新建规则
# 选择"端口" → TCP 3000 → 允许连接
```

### 问题4：前端显示"服务器连接失败"

**检查项：**
1. 服务器是否运行？
2. 浏览器控制台是否有错误？
3. 是否使用了正确的URL？

## 数据持久化

服务器会自动将历史数据保存到 `server/data/history.json`

手动备份：
```cmd
copy server\data\history.json server\data\history_backup.json
```

## 开发模式

使用nodemon自动重启（代码修改后自动重启）：

```cmd
npm run dev
```

## 生产部署

### 使用PM2（推荐）

```cmd
# 安装PM2
npm install -g pm2

# 启动服务
pm2 start server.js --name seat-detection

# 查看状态
pm2 status

# 查看日志
pm2 logs seat-detection

# 停止服务
pm2 stop seat-detection

# 开机自启
pm2 startup
pm2 save
```

## 扩展功能

### 添加数据库支持

可以将数据存储到数据库（如MySQL、MongoDB）：

```javascript
// 安装MySQL
npm install mysql2

// 在server.js中添加数据库连接
const mysql = require('mysql2');
const connection = mysql.createConnection({
  host: 'localhost',
  user: 'root',
  password: 'password',
  database: 'seat_detection'
});
```

### 添加用户认证

```javascript
// 安装JWT
npm install jsonwebtoken

// 添加认证中间件
const jwt = require('jsonwebtoken');
```

### 添加WebSocket实时推送

```javascript
// 安装Socket.IO
npm install socket.io

// 实现实时推送
const io = require('socket.io')(server);
```

## 许可证

本项目仅供学习和研究使用。

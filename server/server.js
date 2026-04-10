/*
 * 座位检测系统 - 本地服务器
 * 功能：
 * 1. 接收ESP32上报的座位状态数据
 * 2. 存储最新的座位状态
 * 3. 提供REST API供前端查询
 * 4. 提供静态文件服务（前端页面）
 */

const express = require('express');
const cors = require('cors');
const path = require('path');
const fs = require('fs');

const app = express();
const PORT = 3000;

// 中间件
app.use(cors());
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// 静态文件服务（前端页面）
app.use(express.static(path.join(__dirname, '../web')));

// 座位状态数据（存储在内存中）
let seatData = {
    occupied: false,
    temperature: 0,
    humidity: 0,
    lightLevel: 0,
    duration: 0,
    lastUpdate: new Date().toISOString(),
    deviceOnline: false
};

// 历史数据（最多保存100条）
let historyData = [];
const MAX_HISTORY = 100;

// 设备信息
let deviceInfo = {
    deviceId: 'seat-001',
    deviceName: '座位检测设备',
    location: '图书馆-1号座位',
    version: '1.0.0',
    uptime: 0,
    lastHeartbeat: null
};

// ==================== API路由 ====================

// 1. 健康检查
app.get('/api/health', (req, res) => {
    res.json({
        status: 'ok',
        timestamp: new Date().toISOString(),
        uptime: process.uptime()
    });
});

// 2. 获取座位状态（供前端查询）
app.get('/api/seat/status', (req, res) => {
    // 检查设备是否在线（超过30秒无更新视为离线）
    const now = new Date();
    const lastUpdate = new Date(seatData.lastUpdate);
    const timeDiff = (now - lastUpdate) / 1000;
    
    seatData.deviceOnline = timeDiff < 30;
    
    res.json({
        success: true,
        data: seatData,
        timestamp: new Date().toISOString()
    });
});

// 3. 接收ESP32上报的数据
app.post('/api/seat/update', (req, res) => {
    try {
        const { occupied, temperature, humidity, lightLevel, duration } = req.body;
        
        // 更新座位数据
        const oldOccupied = seatData.occupied;
        seatData.occupied = occupied;
        seatData.temperature = temperature || 0;
        seatData.humidity = humidity || 0;
        seatData.lightLevel = lightLevel || 0;
        seatData.duration = duration || 0;
        seatData.lastUpdate = new Date().toISOString();
        seatData.deviceOnline = true;
        
        // 记录状态变化
        if (oldOccupied !== occupied) {
            console.log(`[状态变化] ${oldOccupied ? '占用' : '空闲'} → ${occupied ? '占用' : '空闲'}`);
            
            // 保存到历史记录
            addHistory({
                type: 'status_change',
                from: oldOccupied ? 'occupied' : 'vacant',
                to: occupied ? 'occupied' : 'vacant',
                timestamp: new Date().toISOString()
            });
        }
        
        console.log(`[数据更新] 占用:${occupied}, 温度:${temperature}°C, 湿度:${humidity}%, 光照:${lightLevel}%`);
        
        res.json({
            success: true,
            message: '数据更新成功',
            timestamp: new Date().toISOString()
        });
    } catch (error) {
        console.error('[错误] 数据更新失败:', error);
        res.status(500).json({
            success: false,
            message: '数据更新失败',
            error: error.message
        });
    }
});

// 4. 获取历史数据
app.get('/api/seat/history', (req, res) => {
    const limit = parseInt(req.query.limit) || 50;
    const history = historyData.slice(-limit);
    
    res.json({
        success: true,
        data: history,
        total: historyData.length,
        timestamp: new Date().toISOString()
    });
});

// 5. 获取设备信息
app.get('/api/device/info', (req, res) => {
    deviceInfo.uptime = process.uptime();
    
    res.json({
        success: true,
        data: deviceInfo,
        timestamp: new Date().toISOString()
    });
});

// 6. 设备心跳（ESP32定期发送）
app.post('/api/device/heartbeat', (req, res) => {
    deviceInfo.lastHeartbeat = new Date().toISOString();
    seatData.deviceOnline = true;
    
    res.json({
        success: true,
        message: '心跳接收成功',
        timestamp: new Date().toISOString()
    });
});

// 7. 清空历史数据
app.delete('/api/seat/history', (req, res) => {
    historyData = [];
    console.log('[系统] 历史数据已清空');
    
    res.json({
        success: true,
        message: '历史数据已清空',
        timestamp: new Date().toISOString()
    });
});

// 8. 获取统计信息
app.get('/api/seat/statistics', (req, res) => {
    // 计算今日占用次数和时长
    const today = new Date().toDateString();
    const todayHistory = historyData.filter(h => {
        return new Date(h.timestamp).toDateString() === today;
    });
    
    const occupiedCount = todayHistory.filter(h => 
        h.type === 'status_change' && h.to === 'occupied'
    ).length;
    
    res.json({
        success: true,
        data: {
            currentStatus: seatData.occupied ? 'occupied' : 'vacant',
            todayOccupiedCount: occupiedCount,
            totalHistoryRecords: historyData.length,
            deviceOnline: seatData.deviceOnline
        },
        timestamp: new Date().toISOString()
    });
});

// ==================== 辅助函数 ====================

// 添加历史记录
function addHistory(record) {
    historyData.push(record);
    
    // 限制历史记录数量
    if (historyData.length > MAX_HISTORY) {
        historyData.shift();
    }
    
    // 可选：保存到文件
    saveHistoryToFile();
}

// 保存历史数据到文件
function saveHistoryToFile() {
    const dataDir = path.join(__dirname, 'data');
    if (!fs.existsSync(dataDir)) {
        fs.mkdirSync(dataDir);
    }
    
    const filePath = path.join(dataDir, 'history.json');
    fs.writeFileSync(filePath, JSON.stringify(historyData, null, 2));
}

// 从文件加载历史数据
function loadHistoryFromFile() {
    const filePath = path.join(__dirname, 'data', 'history.json');
    if (fs.existsSync(filePath)) {
        try {
            const data = fs.readFileSync(filePath, 'utf8');
            historyData = JSON.parse(data);
            console.log(`[系统] 已加载 ${historyData.length} 条历史记录`);
        } catch (error) {
            console.error('[错误] 加载历史数据失败:', error);
        }
    }
}

// 定期检查设备在线状态
setInterval(() => {
    const now = new Date();
    const lastUpdate = new Date(seatData.lastUpdate);
    const timeDiff = (now - lastUpdate) / 1000;
    
    if (timeDiff > 30 && seatData.deviceOnline) {
        seatData.deviceOnline = false;
        console.log('[警告] 设备离线');
    }
}, 10000); // 每10秒检查一次

// ==================== 启动服务器 ====================

// 加载历史数据
loadHistoryFromFile();

// 启动服务器
app.listen(PORT, () => {
    console.log('========================================');
    console.log('  座位检测系统 - 本地服务器');
    console.log('========================================');
    console.log(`[服务器] 运行在 http://localhost:${PORT}`);
    console.log(`[前端页面] http://localhost:${PORT}`);
    console.log(`[API文档] http://localhost:${PORT}/api/health`);
    console.log('========================================');
    console.log('[系统] 等待设备连接...\n');
});

// 优雅关闭
process.on('SIGINT', () => {
    console.log('\n[系统] 正在关闭服务器...');
    saveHistoryToFile();
    console.log('[系统] 历史数据已保存');
    process.exit(0);
});

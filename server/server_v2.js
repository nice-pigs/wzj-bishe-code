const express = require('express');
const cors = require('cors');
const fs = require('fs');
const path = require('path');

const app = express();
const PORT = 3000;

// 中间件
app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, '../web')));

// 数据存储
let currentSeatData = {
    occupied: false,
    temperature: 0,
    humidity: 0,
    lightLevel: 0,
    pressure: 0,
    distance: 0,
    sound: 0,
    pir: 0,
    duration: 0,
    lastUpdate: new Date().toISOString(),
    deviceOnline: false
};

let deviceInfo = {
    deviceId: 'seat-001',
    deviceName: '座位001',
    location: '图书馆-阅览室A-01号',
    lastHeartbeat: null,
    uptime: 0,
    wifiSignal: 0,
    ipAddress: ''
};

const historyFile = path.join(__dirname, 'history.json');
let historyData = [];

// 统计数据
let statistics = {
    todayOccupiedCount: 0,
    todayOccupiedTime: 0,
    todayVacantTime: 0,
    totalOccupiedCount: 0,
    totalOccupiedTime: 0,
    avgOccupiedDuration: 0,
    peakHours: [],
    lastResetDate: new Date().toISOString().split('T')[0]
};

// 加载历史数据
if (fs.existsSync(historyFile)) {
    try {
        const data = JSON.parse(fs.readFileSync(historyFile, 'utf8'));
        historyData = data.history || [];
        statistics = data.statistics || statistics;
    } catch (err) {
        console.error('[ERROR] Failed to load history:', err);
        historyData = [];
    }
}

// 保存历史数据
function saveHistory() {
    try {
        if (historyData.length > 2000) {
            historyData = historyData.slice(-2000);
        }
        
        const data = {
            history: historyData,
            statistics: statistics,
            lastSaved: new Date().toISOString()
        };
        
        fs.writeFileSync(historyFile, JSON.stringify(data, null, 2));
    } catch (err) {
        console.error('[ERROR] Failed to save history:', err);
    }
}

// 重置每日统计
function checkDailyReset() {
    const today = new Date().toISOString().split('T')[0];
    if (statistics.lastResetDate !== today) {
        statistics.todayOccupiedCount = 0;
        statistics.todayOccupiedTime = 0;
        statistics.todayVacantTime = 0;
        statistics.lastResetDate = today;
        saveHistory();
    }
}

// 定时检查设备在线状态
setInterval(() => {
    if (deviceInfo.lastHeartbeat) {
        const timeSinceHeartbeat = Date.now() - new Date(deviceInfo.lastHeartbeat).getTime();
        currentSeatData.deviceOnline = timeSinceHeartbeat < 30000; // 30秒内有心跳
    }
    checkDailyReset();
}, 5000);

// ==================== API路由 ====================

// 健康检查
app.get('/api/health', (req, res) => {
    res.json({
        success: true,
        message: '服务器运行正常',
        timestamp: new Date().toISOString(),
        uptime: process.uptime()
    });
});

// 接收座位状态更新
app.post('/api/seat/update', (req, res) => {
    try {
        const { occupied, temperature, humidity, lightLevel, pressure, distance, sound, pir, duration } = req.body;
        
        const wasOccupied = currentSeatData.occupied;
        
        // 更新当前数据
        currentSeatData = {
            occupied: occupied !== undefined ? occupied : currentSeatData.occupied,
            temperature: temperature !== undefined ? temperature : currentSeatData.temperature,
            humidity: humidity !== undefined ? humidity : currentSeatData.humidity,
            lightLevel: lightLevel !== undefined ? lightLevel : currentSeatData.lightLevel,
            pressure: pressure !== undefined ? pressure : currentSeatData.pressure,
            distance: distance !== undefined ? distance : currentSeatData.distance,
            sound: sound !== undefined ? sound : currentSeatData.sound,
            pir: pir !== undefined ? pir : currentSeatData.pir,
            duration: duration !== undefined ? duration : currentSeatData.duration,
            lastUpdate: new Date().toISOString(),
            deviceOnline: true
        };
        
        // 状态变化时记录历史
        if (wasOccupied !== currentSeatData.occupied) {
            const historyEntry = {
                ...currentSeatData,
                timestamp: new Date().toISOString(),
                event: currentSeatData.occupied ? 'occupied' : 'vacant'
            };
            
            historyData.push(historyEntry);
            
            // 更新统计
            if (currentSeatData.occupied) {
                statistics.todayOccupiedCount++;
                statistics.totalOccupiedCount++;
            }
            
            saveHistory();
            
            console.log(`[STATE CHANGE] ${wasOccupied ? 'OCCUPIED' : 'VACANT'} -> ${currentSeatData.occupied ? 'OCCUPIED' : 'VACANT'}`);
        }
        
        res.json({
            success: true,
            message: '数据更新成功',
            data: currentSeatData
        });
    } catch (err) {
        console.error('[ERROR] Update failed:', err);
        res.status(500).json({
            success: false,
            message: '数据更新失败',
            error: err.message
        });
    }
});

// 获取当前座位状态
app.get('/api/seat/status', (req, res) => {
    res.json({
        success: true,
        data: currentSeatData
    });
});

// 设备心跳
app.post('/api/device/heartbeat', (req, res) => {
    deviceInfo.lastHeartbeat = new Date().toISOString();
    deviceInfo.uptime = req.body.uptime || deviceInfo.uptime;
    deviceInfo.wifiSignal = req.body.wifiSignal || deviceInfo.wifiSignal;
    deviceInfo.ipAddress = req.body.ipAddress || req.ip;
    
    currentSeatData.deviceOnline = true;
    
    res.json({
        success: true,
        message: '心跳接收成功',
        serverTime: new Date().toISOString()
    });
});

// 获取设备信息
app.get('/api/device/info', (req, res) => {
    res.json({
        success: true,
        data: {
            ...deviceInfo,
            serverUptime: process.uptime()
        }
    });
});

// 获取历史数据
app.get('/api/seat/history', (req, res) => {
    const limit = parseInt(req.query.limit) || 100;
    const offset = parseInt(req.query.offset) || 0;
    
    const paginatedData = historyData.slice(-limit - offset, historyData.length - offset || undefined);
    
    res.json({
        success: true,
        data: paginatedData.reverse(),
        total: historyData.length,
        limit,
        offset
    });
});

// 获取统计数据
app.get('/api/seat/statistics', (req, res) => {
    checkDailyReset();
    
    // 计算平均占用时长
    const occupiedEvents = historyData.filter(h => h.event === 'occupied');
    if (occupiedEvents.length > 0) {
        const totalDuration = occupiedEvents.reduce((sum, h) => sum + (h.duration || 0), 0);
        statistics.avgOccupiedDuration = Math.floor(totalDuration / occupiedEvents.length);
    }
    
    res.json({
        success: true,
        data: statistics
    });
});

// 获取图表数据（最近24小时）
app.get('/api/seat/chart', (req, res) => {
    const hours = parseInt(req.query.hours) || 24;
    const now = new Date();
    const startTime = new Date(now.getTime() - hours * 60 * 60 * 1000);
    
    // 过滤最近N小时的数据
    const recentData = historyData.filter(h => {
        const timestamp = new Date(h.timestamp);
        return timestamp >= startTime;
    });
    
    // 按小时分组统计
    const hourlyStats = {};
    for (let i = 0; i < hours; i++) {
        const hour = new Date(startTime.getTime() + i * 60 * 60 * 1000);
        const hourKey = hour.getHours();
        hourlyStats[hourKey] = {
            hour: hourKey,
            occupiedCount: 0,
            avgTemperature: 0,
            avgHumidity: 0,
            avgLight: 0,
            dataPoints: 0
        };
    }
    
    recentData.forEach(d => {
        const hour = new Date(d.timestamp).getHours();
        if (hourlyStats[hour]) {
            if (d.event === 'occupied') {
                hourlyStats[hour].occupiedCount++;
            }
            hourlyStats[hour].avgTemperature += d.temperature || 0;
            hourlyStats[hour].avgHumidity += d.humidity || 0;
            hourlyStats[hour].avgLight += d.lightLevel || 0;
            hourlyStats[hour].dataPoints++;
        }
    });
    
    // 计算平均值
    Object.values(hourlyStats).forEach(stat => {
        if (stat.dataPoints > 0) {
            stat.avgTemperature = Math.round(stat.avgTemperature / stat.dataPoints);
            stat.avgHumidity = Math.round(stat.avgHumidity / stat.dataPoints);
            stat.avgLight = Math.round(stat.avgLight / stat.dataPoints);
        }
    });
    
    res.json({
        success: true,
        data: Object.values(hourlyStats)
    });
});

// 清空历史数据（管理功能）
app.delete('/api/seat/history', (req, res) => {
    historyData = [];
    statistics = {
        todayOccupiedCount: 0,
        todayOccupiedTime: 0,
        todayVacantTime: 0,
        totalOccupiedCount: 0,
        totalOccupiedTime: 0,
        avgOccupiedDuration: 0,
        peakHours: [],
        lastResetDate: new Date().toISOString().split('T')[0]
    };
    saveHistory();
    
    res.json({
        success: true,
        message: '历史数据已清空'
    });
});

// 启动服务器 - 监听所有网络接口
app.listen(PORT, '0.0.0.0', () => {
    console.log('\n========================================');
    console.log('  座位检测系统 - 本地服务器 v2.0');
    console.log('========================================');
    console.log(`  [服务器] 运行在 http://localhost:${PORT}`);
    console.log(`  [局域网] http://10.150.235.41:${PORT}`);
    console.log(`  [前端页面] http://localhost:${PORT}`);
    console.log(`  [API文档] http://localhost:${PORT}/api/health`);
    console.log('========================================');
    console.log('  [系统] 等待设备连接...\n');
});

/**
 * 座位检测系统 - 多设备服务器 v3.0
 * 支持多个STM32设备同时连接
 */

const express = require('express');
const cors = require('cors');
const fs = require('fs');
const path = require('path');

const app = express();
const PORT = 3000;

app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, '../web')));

// 多设备数据存储
const devices = new Map(); // deviceId -> deviceData

// 历史数据文件
const historyFile = path.join(__dirname, 'history_multi.json');
let historyData = [];

// 加载历史数据
if (fs.existsSync(historyFile)) {
    try {
        historyData = JSON.parse(fs.readFileSync(historyFile, 'utf8'));
    } catch (err) {
        console.error('[ERROR] Failed to load history:', err);
        historyData = [];
    }
}

// 保存历史数据
function saveHistory() {
    try {
        if (historyData.length > 5000) {
            historyData = historyData.slice(-5000);
        }
        fs.writeFileSync(historyFile, JSON.stringify(historyData, null, 2));
    } catch (err) {
        console.error('[ERROR] Failed to save history:', err);
    }
}

// 检查设备在线状态
setInterval(() => {
    const now = Date.now();
    devices.forEach((device, deviceId) => {
        if (device.lastHeartbeat) {
            const timeSinceHeartbeat = now - new Date(device.lastHeartbeat).getTime();
            device.online = timeSinceHeartbeat < 30000;
        } else {
            device.online = false;
        }
    });
}, 5000);

// ==================== API路由 ====================

// 健康检查
app.get('/api/health', (req, res) => {
    res.json({
        success: true,
        message: '多设备服务器运行正常',
        timestamp: new Date().toISOString(),
        deviceCount: devices.size,
        uptime: process.uptime()
    });
});

// 接收设备数据更新
app.post('/api/device/:deviceId/update', (req, res) => {
    try {
        const { deviceId } = req.params;
        const { occupied, temperature, humidity, lightLevel, pressure, distance, sound, pir, duration, deviceName, location } = req.body;
        
        // 获取或创建设备
        if (!devices.has(deviceId)) {
            devices.set(deviceId, {
                deviceId,
                deviceName: deviceName || `设备${deviceId}`,
                location: location || '未知位置',
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
                lastHeartbeat: new Date().toISOString(),
                online: true,
                statistics: {
                    totalOccupiedCount: 0,
                    todayOccupiedCount: 0
                }
            });
            console.log(`[NEW DEVICE] ${deviceId} connected`);
        }
        
        const device = devices.get(deviceId);
        const wasOccupied = device.occupied;
        
        // 更新设备数据
        device.occupied = occupied !== undefined ? occupied : device.occupied;
        device.temperature = temperature !== undefined ? temperature : device.temperature;
        device.humidity = humidity !== undefined ? humidity : device.humidity;
        device.lightLevel = lightLevel !== undefined ? lightLevel : device.lightLevel;
        device.pressure = pressure !== undefined ? pressure : device.pressure;
        device.distance = distance !== undefined ? distance : device.distance;
        device.sound = sound !== undefined ? sound : device.sound;
        device.pir = pir !== undefined ? pir : device.pir;
        device.duration = duration !== undefined ? duration : device.duration;
        device.lastUpdate = new Date().toISOString();
        device.online = true;
        
        if (deviceName) device.deviceName = deviceName;
        if (location) device.location = location;
        
        // 状态变化时记录历史
        if (wasOccupied !== device.occupied) {
            const historyEntry = {
                deviceId,
                deviceName: device.deviceName,
                location: device.location,
                occupied: device.occupied,
                temperature: device.temperature,
                humidity: device.humidity,
                lightLevel: device.lightLevel,
                pressure: device.pressure,
                distance: device.distance,
                sound: device.sound,
                pir: device.pir,
                duration: device.duration,
                timestamp: new Date().toISOString()
            };
            
            historyData.push(historyEntry);
            
            if (device.occupied) {
                device.statistics.totalOccupiedCount++;
                device.statistics.todayOccupiedCount++;
            }
            
            saveHistory();
            
            console.log(`[${deviceId}] ${wasOccupied ? 'OCCUPIED' : 'VACANT'} -> ${device.occupied ? 'OCCUPIED' : 'VACANT'}`);
        }
        
        res.json({
            success: true,
            message: '数据更新成功',
            data: device
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

// 设备心跳
app.post('/api/device/:deviceId/heartbeat', (req, res) => {
    const { deviceId } = req.params;
    
    if (devices.has(deviceId)) {
        const device = devices.get(deviceId);
        device.lastHeartbeat = new Date().toISOString();
        device.online = true;
    }
    
    res.json({
        success: true,
        message: '心跳接收成功',
        serverTime: new Date().toISOString()
    });
});

// 获取所有设备列表
app.get('/api/devices', (req, res) => {
    const deviceList = Array.from(devices.values()).map(device => ({
        deviceId: device.deviceId,
        deviceName: device.deviceName,
        location: device.location,
        occupied: device.occupied,
        online: device.online,
        lastUpdate: device.lastUpdate,
        statistics: device.statistics
    }));
    
    res.json({
        success: true,
        data: deviceList,
        total: deviceList.length
    });
});

// 获取单个设备状态
app.get('/api/device/:deviceId/status', (req, res) => {
    const { deviceId } = req.params;
    
    if (!devices.has(deviceId)) {
        return res.status(404).json({
            success: false,
            message: '设备不存在'
        });
    }
    
    res.json({
        success: true,
        data: devices.get(deviceId)
    });
});

// 获取历史数据（支持按设备过滤）
app.get('/api/history', (req, res) => {
    const { deviceId, limit = 100, offset = 0 } = req.query;
    
    let filteredData = historyData;
    
    if (deviceId) {
        filteredData = historyData.filter(h => h.deviceId === deviceId);
    }
    
    const paginatedData = filteredData.slice(-parseInt(limit) - parseInt(offset), filteredData.length - parseInt(offset) || undefined);
    
    res.json({
        success: true,
        data: paginatedData.reverse(),
        total: filteredData.length
    });
});

// 获取统计数据
app.get('/api/statistics', (req, res) => {
    const stats = {
        totalDevices: devices.size,
        onlineDevices: Array.from(devices.values()).filter(d => d.online).length,
        occupiedDevices: Array.from(devices.values()).filter(d => d.occupied).length,
        totalOccupiedCount: Array.from(devices.values()).reduce((sum, d) => sum + d.statistics.totalOccupiedCount, 0),
        todayOccupiedCount: Array.from(devices.values()).reduce((sum, d) => sum + d.statistics.todayOccupiedCount, 0)
    };
    
    res.json({
        success: true,
        data: stats
    });
});

// 删除设备
app.delete('/api/device/:deviceId', (req, res) => {
    const { deviceId } = req.params;
    
    if (devices.has(deviceId)) {
        devices.delete(deviceId);
        res.json({
            success: true,
            message: '设备已删除'
        });
    } else {
        res.status(404).json({
            success: false,
            message: '设备不存在'
        });
    }
});

// 清空历史数据
app.delete('/api/history', (req, res) => {
    historyData = [];
    saveHistory();
    
    res.json({
        success: true,
        message: '历史数据已清空'
    });
});

// 启动服务器
app.listen(PORT, () => {
    console.log('\n========================================');
    console.log('  座位检测系统 - 多设备服务器 v3.0');
    console.log('========================================');
    console.log(`  [服务器] 运行在 http://localhost:${PORT}`);
    console.log(`  [前端页面] http://localhost:${PORT}/multi.html`);
    console.log(`  [API文档] http://localhost:${PORT}/api/health`);
    console.log('========================================');
    console.log('  [系统] 等待设备连接...\n');
});

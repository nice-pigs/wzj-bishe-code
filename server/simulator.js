/**
 * 座位检测系统 - 数据模拟器
 * 用于测试前后端功能，模拟真实传感器数据
 */

const axios = require('axios');

const SERVER_URL = 'http://localhost:3000';

// 模拟数据生成器
class SeatSimulator {
    constructor() {
        this.occupied = false;
        this.duration = 0;
        this.temperature = 25;
        this.humidity = 60;
        this.lightLevel = 80;
        this.pressure = 0;
        this.distance = 150;
        this.sound = 0;
        this.pir = 0;
    }

    // 生成随机数据
    generateData() {
        // 随机改变状态（10%概率）
        if (Math.random() < 0.1) {
            this.occupied = !this.occupied;
            this.duration = 0;
            console.log(`[状态变化] ${this.occupied ? '空闲 -> 占用' : '占用 -> 空闲'}`);
        } else {
            this.duration += 5; // 每5秒增加
        }

        // 根据占用状态生成传感器数据
        if (this.occupied) {
            // 有人：温度升高、湿度升高、光照降低、有压力、距离近
            this.temperature = 26 + Math.random() * 2; // 26-28°C
            this.humidity = 65 + Math.random() * 10;   // 65-75%
            this.lightLevel = 30 + Math.random() * 20; // 30-50%
            this.pressure = 1;                         // 有压力
            this.distance = 30 + Math.random() * 20;   // 30-50cm
            this.sound = Math.random() < 0.3 ? 1 : 0;  // 30%概率有声音
            this.pir = Math.random() < 0.2 ? 1 : 0;    // 20%概率检测到运动
        } else {
            // 无人：温度正常、湿度正常、光照高、无压力、距离远
            this.temperature = 24 + Math.random() * 2; // 24-26°C
            this.humidity = 55 + Math.random() * 10;   // 55-65%
            this.lightLevel = 70 + Math.random() * 20; // 70-90%
            this.pressure = 0;                         // 无压力
            this.distance = 120 + Math.random() * 50;  // 120-170cm
            this.sound = 0;                            // 无声音
            this.pir = 0;                              // 无运动
        }

        return {
            occupied: this.occupied,
            temperature: Math.round(this.temperature),
            humidity: Math.round(this.humidity),
            lightLevel: Math.round(this.lightLevel),
            pressure: this.pressure,
            distance: Math.round(this.distance),
            sound: this.sound,
            pir: this.pir,
            duration: this.duration
        };
    }

    // 发送数据到服务器
    async sendData() {
        const data = this.generateData();
        
        try {
            const response = await axios.post(`${SERVER_URL}/api/seat/update`, data);
            
            if (response.data.success) {
                console.log(`[数据上传] 状态:${data.occupied ? '占用' : '空闲'}, 温度:${data.temperature}°C, 湿度:${data.humidity}%, 光照:${data.lightLevel}%, 压力:${data.pressure}, 距离:${data.distance}cm`);
            }
        } catch (error) {
            console.error('[错误] 数据上传失败:', error.message);
        }
    }

    // 发送心跳
    async sendHeartbeat() {
        try {
            await axios.post(`${SERVER_URL}/api/device/heartbeat`, {
                uptime: Math.floor(process.uptime()),
                wifiSignal: -40 - Math.random() * 20,
                ipAddress: '192.168.1.100'
            });
            console.log('[心跳] 发送成功');
        } catch (error) {
            console.error('[错误] 心跳发送失败:', error.message);
        }
    }

    // 启动模拟器
    start() {
        console.log('\n========================================');
        console.log('  座位检测系统 - 数据模拟器');
        console.log('========================================');
        console.log(`  [目标服务器] ${SERVER_URL}`);
        console.log('  [模拟频率] 每5秒发送一次数据');
        console.log('  [心跳频率] 每10秒发送一次心跳');
        console.log('========================================\n');

        // 立即发送一次
        this.sendData();
        this.sendHeartbeat();

        // 定时发送数据（每5秒）
        setInterval(() => {
            this.sendData();
        }, 5000);

        // 定时发送心跳（每10秒）
        setInterval(() => {
            this.sendHeartbeat();
        }, 10000);
    }
}

// 启动模拟器
const simulator = new SeatSimulator();
simulator.start();

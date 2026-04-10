/**
 * 多STM32设备模拟器
 * 模拟多个STM32+ESP32设备同时工作
 */

const axios = require('axios');

const SERVER_URL = 'http://localhost:3000';

// 设备配置
const DEVICES = [
    { id: 'STM32-001', name: '座位001', location: '图书馆-阅览室A-01号' },
    { id: 'STM32-002', name: '座位002', location: '图书馆-阅览室A-02号' },
    { id: 'STM32-003', name: '座位003', location: '图书馆-阅览室A-03号' },
    { id: 'STM32-004', name: '座位004', location: '图书馆-阅览室B-01号' },
    { id: 'STM32-005', name: '座位005', location: '图书馆-阅览室B-02号' }
];

class DeviceSimulator {
    constructor(config) {
        this.deviceId = config.id;
        this.deviceName = config.name;
        this.location = config.location;
        this.occupied = Math.random() < 0.3; // 30%初始占用率
        this.duration = 0;
        this.temperature = 25;
        this.humidity = 60;
        this.lightLevel = 80;
        this.pressure = 0;
        this.distance = 150;
        this.sound = 0;
        this.pir = 0;
    }

    generateData() {
        // 5%概率改变状态
        if (Math.random() < 0.05) {
            this.occupied = !this.occupied;
            this.duration = 0;
            console.log(`[${this.deviceName}] 状态变化: ${this.occupied ? '空闲 -> 占用' : '占用 -> 空闲'}`);
        } else {
            this.duration += 5;
        }

        if (this.occupied) {
            this.temperature = 26 + Math.random() * 2;
            this.humidity = 65 + Math.random() * 10;
            this.lightLevel = 30 + Math.random() * 20;
            this.pressure = 1;
            this.distance = 30 + Math.random() * 20;
            this.sound = Math.random() < 0.3 ? 1 : 0;
            this.pir = Math.random() < 0.2 ? 1 : 0;
        } else {
            this.temperature = 24 + Math.random() * 2;
            this.humidity = 55 + Math.random() * 10;
            this.lightLevel = 70 + Math.random() * 20;
            this.pressure = 0;
            this.distance = 120 + Math.random() * 50;
            this.sound = 0;
            this.pir = 0;
        }

        return {
            deviceName: this.deviceName,
            location: this.location,
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

    async sendData() {
        const data = this.generateData();
        
        try {
            await axios.post(`${SERVER_URL}/api/device/${this.deviceId}/update`, data);
            const status = data.occupied ? '占用' : '空闲';
            console.log(`[${this.deviceName}] ${status} | T:${data.temperature}°C H:${data.humidity}% L:${data.lightLevel}%`);
        } catch (error) {
            // 忽略错误
        }
    }

    async sendHeartbeat() {
        try {
            await axios.post(`${SERVER_URL}/api/device/${this.deviceId}/heartbeat`, {});
        } catch (error) {
            // 忽略错误
        }
    }
}

class MultiDeviceManager {
    constructor() {
        this.devices = DEVICES.map(config => new DeviceSimulator(config));
    }

    displayStatus() {
        console.log('\n========== 设备状态总览 ==========');
        this.devices.forEach(device => {
            const data = device.generateData();
            const status = data.occupied ? '🔴 占用' : '🟢 空闲';
            console.log(`${data.deviceName} [${data.location}]: ${status} (${data.duration}秒)`);
        });
        console.log('===================================\n');
    }

    start() {
        console.log('\n========================================');
        console.log('  多STM32设备模拟器');
        console.log('========================================');
        console.log(`  [目标服务器] ${SERVER_URL}`);
        console.log(`  [设备数量] ${DEVICES.length}个STM32设备`);
        console.log('  [更新频率] 每5秒轮询发送');
        console.log('========================================\n');

        this.displayStatus();

        // 初始化：所有设备发送一次数据
        this.devices.forEach(device => {
            device.sendData();
            device.sendHeartbeat();
        });

        // 定时轮询发送数据（每5秒一个设备）
        let deviceIndex = 0;
        setInterval(() => {
            this.devices[deviceIndex].sendData();
            deviceIndex = (deviceIndex + 1) % this.devices.length;
        }, 5000);

        // 定时发送心跳（每10秒所有设备）
        setInterval(() => {
            this.devices.forEach(device => device.sendHeartbeat());
        }, 10000);

        // 定时显示状态（每30秒）
        setInterval(() => {
            this.displayStatus();
        }, 30000);
    }
}

const manager = new MultiDeviceManager();
manager.start();

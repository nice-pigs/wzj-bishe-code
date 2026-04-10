/**
 * 座位检测系统 - 多座位模拟器
 * 模拟图书馆多个座位的数据
 */

const axios = require('axios');

const SERVER_URL = 'http://localhost:3000';

// 座位配置
const SEATS = [
    { id: 'seat-001', name: '座位001', location: '图书馆-阅览室A-01号' },
    { id: 'seat-002', name: '座位002', location: '图书馆-阅览室A-02号' },
    { id: 'seat-003', name: '座位003', location: '图书馆-阅览室A-03号' },
    { id: 'seat-004', name: '座位004', location: '图书馆-阅览室B-01号' },
    { id: 'seat-005', name: '座位005', location: '图书馆-阅览室B-02号' }
];

// 单个座位模拟器
class SeatSimulator {
    constructor(seatInfo) {
        this.seatInfo = seatInfo;
        this.occupied = Math.random() < 0.5; // 随机初始状态
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
        // 随机改变状态（5%概率）
        if (Math.random() < 0.05) {
            this.occupied = !this.occupied;
            this.duration = 0;
            console.log(`[${this.seatInfo.name}] 状态变化: ${this.occupied ? '空闲 -> 占用' : '占用 -> 空闲'}`);
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
            seatId: this.seatInfo.id,
            seatName: this.seatInfo.name,
            location: this.seatInfo.location,
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
            // 注意：当前服务器只支持单座位，这里模拟发送
            // 实际使用需要修改服务器支持多座位
            const response = await axios.post(`${SERVER_URL}/api/seat/update`, data);
            
            if (response.data.success) {
                const status = data.occupied ? '占用' : '空闲';
                console.log(`[${data.seatName}] ${status} | T:${data.temperature}°C H:${data.humidity}% L:${data.lightLevel}% P:${data.pressure} D:${data.distance}cm`);
            }
        } catch (error) {
            // 忽略错误，继续运行
        }
    }
}

// 多座位管理器
class MultiSeatManager {
    constructor() {
        this.simulators = SEATS.map(seat => new SeatSimulator(seat));
    }

    // 显示所有座位状态
    displayStatus() {
        console.log('\n========== 当前座位状态 ==========');
        this.simulators.forEach(sim => {
            const data = sim.generateData();
            const status = data.occupied ? '🔴 占用' : '🟢 空闲';
            console.log(`${data.seatName} [${data.location}]: ${status} (${data.duration}秒)`);
        });
        console.log('===================================\n');
    }

    // 启动所有模拟器
    start() {
        console.log('\n========================================');
        console.log('  座位检测系统 - 多座位模拟器');
        console.log('========================================');
        console.log(`  [目标服务器] ${SERVER_URL}`);
        console.log(`  [座位数量] ${SEATS.length}个`);
        console.log('  [模拟频率] 每5秒更新一次');
        console.log('========================================\n');

        // 显示初始状态
        this.displayStatus();

        // 定时发送数据（每5秒）
        setInterval(() => {
            // 随机选择一个座位发送数据（模拟轮询）
            const randomSeat = this.simulators[Math.floor(Math.random() * this.simulators.length)];
            randomSeat.sendData();
        }, 5000);

        // 定时显示状态（每30秒）
        setInterval(() => {
            this.displayStatus();
        }, 30000);

        // 定时发送心跳
        setInterval(() => {
            this.sendHeartbeat();
        }, 10000);
    }

    async sendHeartbeat() {
        try {
            await axios.post(`${SERVER_URL}/api/device/heartbeat`, {
                uptime: Math.floor(process.uptime()),
                wifiSignal: -40 - Math.random() * 20,
                ipAddress: '192.168.1.100'
            });
        } catch (error) {
            // 忽略错误
        }
    }
}

// 启动多座位管理器
const manager = new MultiSeatManager();
manager.start();

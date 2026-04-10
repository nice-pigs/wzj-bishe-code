// 配置
const SERVER_URL = 'http://localhost:3000';
let refreshInterval = 2000;
let updateTimer = null;
let occupancyChart = null;
let environmentChart = null;

// 页面加载
window.onload = function() {
    initCharts();
    initNavigation();
    connectServer();
    startAutoUpdate();
};

// 初始化导航
function initNavigation() {
    const navLinks = document.querySelectorAll('.nav-link');
    navLinks.forEach(link => {
        link.addEventListener('click', (e) => {
            e.preventDefault();
            const target = link.getAttribute('href').substring(1);
            showPage(target);
            
            navLinks.forEach(l => l.classList.remove('active'));
            link.classList.add('active');
        });
    });
}

// 显示页面
function showPage(pageId) {
    document.querySelectorAll('.page').forEach(page => {
        page.classList.remove('active');
    });
    document.getElementById(pageId).classList.add('active');
    
    // 加载对应页面数据
    if (pageId === 'history') {
        loadHistory();
    } else if (pageId === 'statistics') {
        loadStatistics();
    }
}

// 连接服务器
function connectServer() {
    fetch(`${SERVER_URL}/api/health`)
        .then(response => response.json())
        .then(data => {
            updateConnectionStatus(true);
        })
        .catch(error => {
            updateConnectionStatus(false);
            setTimeout(connectServer, 5000);
        });
}

// 更新连接状态
function updateConnectionStatus(connected) {
    const dot = document.getElementById('connectionDot');
    const text = document.getElementById('connectionText');
    
    if (connected) {
        dot.classList.add('online');
        text.textContent = '已连接';
    } else {
        dot.classList.remove('online');
        text.textContent = '未连接';
    }
}

// 开始自动更新
function startAutoUpdate() {
    if (updateTimer) clearInterval(updateTimer);
    
    updateDashboard();
    updateTimer = setInterval(updateDashboard, refreshInterval);
}

// 更新仪表盘
function updateDashboard() {
    updateStatus();
    updateCharts();
}

// 更新座位状态
function updateStatus() {
    fetch(`${SERVER_URL}/api/seat/status`)
        .then(response => response.json())
        .then(result => {
            if (!result.success) return;
            
            const data = result.data;
            const card = document.getElementById('statusCard');
            const icon = document.getElementById('statusIcon');
            const text = document.getElementById('statusText');
            const duration = document.getElementById('statusDuration');
            
            // 更新状态卡片
            if (!data.deviceOnline) {
                card.className = 'card status-card offline';
                icon.textContent = '⚠️';
                text.textContent = '设备离线';
                duration.textContent = 'STM32未连接';
                document.getElementById('deviceStatus').textContent = '离线';
            } else if (data.occupied) {
                card.className = 'card status-card occupied';
                icon.textContent = '🔴';
                text.textContent = '占用中';
                duration.textContent = '持续: ' + formatDuration(data.duration);
                document.getElementById('deviceStatus').textContent = '在线';
            } else {
                card.className = 'card status-card vacant';
                icon.textContent = '🟢';
                text.textContent = '空闲';
                duration.textContent = '空闲: ' + formatDuration(data.duration);
                document.getElementById('deviceStatus').textContent = '在线';
            }
            
            // 更新传感器数据
            document.getElementById('temperature').textContent = data.temperature || '--';
            document.getElementById('humidity').textContent = data.humidity || '--';
            document.getElementById('lightLevel').textContent = data.lightLevel || '--';
            document.getElementById('pressure').textContent = data.pressure || '--';
            document.getElementById('distance').textContent = data.distance || '--';
            document.getElementById('sound').textContent = data.sound || '--';
            
            updateConnectionStatus(true);
        })
        .catch(error => {
            updateConnectionStatus(false);
        });
    
    // 更新统计数据
    fetch(`${SERVER_URL}/api/seat/statistics`)
        .then(response => response.json())
        .then(result => {
            if (!result.success) return;
            
            const stats = result.data;
            document.getElementById('todayCount').textContent = stats.todayOccupiedCount || 0;
            document.getElementById('avgDuration').textContent = 
                formatDuration(stats.avgOccupiedDuration || 0);
        })
        .catch(error => console.error('Statistics error:', error));
}

// 初始化图表
function initCharts() {
    // 占用趋势图
    const occupancyCtx = document.getElementById('occupancyChart').getContext('2d');
    occupancyChart = new Chart(occupancyCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: '占用次数',
                data: [],
                borderColor: '#667eea',
                backgroundColor: 'rgba(102, 126, 234, 0.1)',
                tension: 0.4,
                fill: true
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: {
                    display: true,
                    position: 'top'
                }
            },
            scales: {
                y: {
                    beginAtZero: true,
                    ticks: {
                        stepSize: 1
                    }
                }
            }
        }
    });
    
    // 环境参数图
    const environmentCtx = document.getElementById('environmentChart').getContext('2d');
    environmentChart = new Chart(environmentCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                {
                    label: '温度 (°C)',
                    data: [],
                    borderColor: '#f5576c',
                    backgroundColor: 'rgba(245, 87, 108, 0.1)',
                    yAxisID: 'y'
                },
                {
                    label: '湿度 (%)',
                    data: [],
                    borderColor: '#4facfe',
                    backgroundColor: 'rgba(79, 172, 254, 0.1)',
                    yAxisID: 'y'
                },
                {
                    label: '光照 (%)',
                    data: [],
                    borderColor: '#feca57',
                    backgroundColor: 'rgba(254, 202, 87, 0.1)',
                    yAxisID: 'y'
                }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: {
                    display: true,
                    position: 'top'
                }
            },
            scales: {
                y: {
                    beginAtZero: true,
                    max: 100
                }
            }
        }
    });
}

// 更新图表
function updateCharts() {
    fetch(`${SERVER_URL}/api/seat/chart?hours=24`)
        .then(response => response.json())
        .then(result => {
            if (!result.success) return;
            
            const data = result.data;
            const labels = data.map(d => d.hour + ':00');
            
            // 更新占用趋势图
            occupancyChart.data.labels = labels;
            occupancyChart.data.datasets[0].data = data.map(d => d.occupiedCount);
            occupancyChart.update();
            
            // 更新环境参数图
            environmentChart.data.labels = labels;
            environmentChart.data.datasets[0].data = data.map(d => d.avgTemperature);
            environmentChart.data.datasets[1].data = data.map(d => d.avgHumidity);
            environmentChart.data.datasets[2].data = data.map(d => d.avgLight);
            environmentChart.update();
        })
        .catch(error => console.error('Chart error:', error));
}

// 加载历史记录
function loadHistory() {
    fetch(`${SERVER_URL}/api/seat/history?limit=50`)
        .then(response => response.json())
        .then(result => {
            if (!result.success) {
                console.error('Failed to load history');
                return;
            }
            
            const tbody = document.getElementById('historyBody');
            tbody.innerHTML = '';
            
            if (!result.data || result.data.length === 0) {
                tbody.innerHTML = '<tr><td colspan="6" class="no-data">暂无历史数据</td></tr>';
                return;
            }
            
            result.data.forEach(record => {
                const row = document.createElement('tr');
                const time = new Date(record.timestamp).toLocaleString('zh-CN');
                const status = record.occupied ? '占用' : '空闲';
                const statusClass = record.occupied ? 'status-occupied' : 'status-vacant';
                
                row.innerHTML = `
                    <td>${time}</td>
                    <td><span class="${statusClass}">${status}</span></td>
                    <td>${record.temperature || '--'}°C</td>
                    <td>${record.humidity || '--'}%</td>
                    <td>${record.lightLevel || '--'}%</td>
                    <td>${formatDuration(record.duration || 0)}</td>
                `;
                tbody.appendChild(row);
            });
        })
        .catch(error => {
            console.error('History error:', error);
            const tbody = document.getElementById('historyBody');
            tbody.innerHTML = '<tr><td colspan="6" class="no-data">加载失败，请检查服务器连接</td></tr>';
        });
}

// 加载统计数据
function loadStatistics() {
    fetch(`${SERVER_URL}/api/seat/statistics`)
        .then(response => response.json())
        .then(result => {
            if (!result.success) return;
            
            const stats = result.data;
            document.getElementById('statTodayCount').textContent = stats.todayOccupiedCount || 0;
            document.getElementById('statTodayTime').textContent = 
                Math.floor((stats.todayOccupiedTime || 0) / 60) + '分钟';
            document.getElementById('statTotalCount').textContent = stats.totalOccupiedCount || 0;
            document.getElementById('statTotalTime').textContent = 
                Math.floor((stats.totalOccupiedTime || 0) / 60) + '分钟';
        })
        .catch(error => console.error('Statistics error:', error));
}

// 保存设置
function saveSettings() {
    const interval = document.getElementById('refreshInterval').value;
    refreshInterval = parseInt(interval);
    startAutoUpdate();
    alert('设置已保存！');
}

// 清空历史数据
function clearHistory() {
    if (!confirm('确定要清空所有历史数据吗？此操作不可恢复！')) {
        return;
    }
    
    fetch(`${SERVER_URL}/api/seat/history`, {
        method: 'DELETE'
    })
    .then(response => response.json())
    .then(result => {
        if (result.success) {
            alert('历史数据已清空！');
            loadHistory();
        }
    })
    .catch(error => {
        alert('清空失败：' + error.message);
    });
}

// 格式化时长
function formatDuration(seconds) {
    if (seconds < 60) return seconds + '秒';
    if (seconds < 3600) return Math.floor(seconds / 60) + '分钟';
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    return hours + '小时' + minutes + '分钟';
}

'use client';

import { useEffect, useState } from 'react';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Badge } from '@/components/ui/badge';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogHeader,
  DialogTitle,
} from '@/components/ui/dialog';
import { Skeleton } from '@/components/ui/skeleton';
import { Progress } from '@/components/ui/progress';
import { 
  MapPin, 
  Users, 
  Wifi, 
  WifiOff, 
  RefreshCw,
  Clock,
  TrendingUp,
  Activity,
  Armchair,
  Zap,
  History,
  CheckCircle2,
  XCircle,
  Timer,
  Thermometer,
  Droplets,
  Sun,
  Gauge,
  Radio
} from 'lucide-react';

interface SensorData {
  temperature: number | null;
  humidity: number | null;
  lightLevel: number | null;
  pressure1: boolean | null;
  pressure2: boolean | null;
  distance: number | null;
  pirState: boolean | null;
  wifiSignal: number | null;
  detectionScore: number | null;
  duration: number | null;
  recordedAt: string | null;
}

interface Seat {
  id: number;
  name: string;
  location: string | null;
  isOccupied: boolean;
  deviceStatus: string;
  lastUpdate: string;
  sensorData: SensorData | null;
}

interface SeatHistory {
  id: number;
  seatId: number;
  isOccupied: boolean;
  recordedAt: string;
}

export default function Home() {
  const [seats, setSeats] = useState<Seat[]>([]);
  const [loading, setLoading] = useState(true);
  const [refreshing, setRefreshing] = useState(false);
  const [filterLocation, setFilterLocation] = useState('');
  const [selectedSeat, setSelectedSeat] = useState<Seat | null>(null);
  const [historyData, setHistoryData] = useState<SeatHistory[]>([]);
  const [historyLoading, setHistoryLoading] = useState(false);
  const [currentTime, setCurrentTime] = useState(new Date());
  const [mounted, setMounted] = useState(false);

  // 客户端挂载后才显示时间（避免 Hydration 错误）
  useEffect(() => {
    setMounted(true);
    const timer = setInterval(() => {
      setCurrentTime(new Date());
    }, 1000);
    return () => clearInterval(timer);
  }, []);

  // 获取座位数据
  const fetchSeats = async () => {
    try {
      const params = new URLSearchParams();
      if (filterLocation) {
        params.append('location', filterLocation);
      }
      
      const response = await fetch(`/api/seats?${params.toString()}`);
      const result = await response.json();
      
      if (result.success) {
        setSeats(result.data);
      }
    } catch (error) {
      console.error('获取座位数据失败:', error);
    } finally {
      setLoading(false);
      setRefreshing(false);
    }
  };

  // 获取历史记录
  const fetchHistory = async (seatId: number) => {
    setHistoryLoading(true);
    try {
      const response = await fetch(`/api/seats/${seatId}/history?limit=20`);
      const result = await response.json();
      
      if (result.success) {
        setHistoryData(result.data);
      }
    } catch (error) {
      console.error('获取历史记录失败:', error);
    } finally {
      setHistoryLoading(false);
    }
  };

  // 初始加载
  useEffect(() => {
    fetchSeats();
  }, [filterLocation]);

  // 自动刷新（每5秒）
  useEffect(() => {
    const interval = setInterval(() => {
      fetchSeats();
    }, 5000);

    return () => clearInterval(interval);
  }, [filterLocation]);

  // 手动刷新
  const handleRefresh = () => {
    setRefreshing(true);
    fetchSeats();
  };

  // 查看座位详情
  const handleViewDetails = (seat: Seat) => {
    setSelectedSeat(seat);
    fetchHistory(seat.id);
  };

  // 统计信息
  const totalSeats = seats.length;
  const occupiedSeats = seats.filter(s => s.isOccupied).length;
  const idleSeats = totalSeats - occupiedSeats;
  const onlineDevices = seats.filter(s => s.deviceStatus === 'online').length;
  const usageRate = totalSeats > 0 ? Math.round((occupiedSeats / totalSeats) * 100) : 0;

  // 格式化时间 - 使用中国时区
  const formatTime = (date: Date) => {
    return date.toLocaleTimeString('zh-CN', {
      hour: '2-digit',
      minute: '2-digit',
      second: '2-digit',
      timeZone: 'Asia/Shanghai',
    });
  };

  const formatDate = (date: Date) => {
    return date.toLocaleDateString('zh-CN', {
      year: 'numeric',
      month: 'long',
      day: 'numeric',
      weekday: 'long',
      timeZone: 'Asia/Shanghai',
    });
  };

  // 计算持续时间
  const getDuration = (lastUpdate: string) => {
    const now = new Date();
    const last = new Date(lastUpdate);
    const diffMs = now.getTime() - last.getTime();
    const diffMins = Math.floor(diffMs / 60000);
    
    if (diffMins < 1) return '刚刚';
    if (diffMins < 60) return `${diffMins}分钟前`;
    const diffHours = Math.floor(diffMins / 60);
    if (diffHours < 24) return `${diffHours}小时前`;
    return `${Math.floor(diffHours / 24)}天前`;
  };

  return (
    <div className="min-h-screen bg-slate-50 dark:bg-slate-950">
      {/* 顶部导航 */}
      <header className="sticky top-0 z-50 border-b bg-white/90 backdrop-blur-md dark:bg-slate-900/90">
        <div className="container mx-auto px-4 py-3">
          <div className="flex items-center justify-between">
            {/* Logo */}
            <div className="flex items-center gap-3">
              <div className="flex h-11 w-11 items-center justify-center rounded-xl bg-gradient-to-br from-blue-600 to-indigo-600 shadow-lg shadow-blue-500/30">
                <Armchair className="h-6 w-6 text-white" />
              </div>
              <div>
                <h1 className="text-lg font-bold text-slate-900 dark:text-white">
                  智能座位监测系统
                </h1>
                <p className="text-xs text-slate-500 dark:text-slate-400">
                  物联网实时监测平台
                </p>
              </div>
            </div>
            
            {/* 右侧工具栏 */}
            <div className="flex items-center gap-3">
              {/* 实时时间 */}
              <div className="hidden md:flex flex-col items-end">
                <div className="text-sm font-medium text-slate-900 dark:text-white">
                  {mounted ? formatTime(currentTime) : '--:--:--'}
                </div>
                <div className="text-xs text-slate-500 dark:text-slate-400">
                  {mounted ? formatDate(currentTime) : '----年--月--日'}
                </div>
              </div>
              
              {/* 搜索 */}
              <div className="relative">
                <Input
                  placeholder="搜索位置..."
                  value={filterLocation}
                  onChange={(e) => setFilterLocation(e.target.value)}
                  className="w-40 md:w-48 bg-slate-100 dark:bg-slate-800 border-0"
                />
              </div>
              
              {/* 刷新按钮 */}
              <Button
                variant="outline"
                size="icon"
                onClick={handleRefresh}
                disabled={refreshing}
                className="relative"
              >
                <RefreshCw className={`h-4 w-4 ${refreshing ? 'animate-spin' : ''}`} />
              </Button>
            </div>
          </div>
        </div>
      </header>

      <main className="container mx-auto px-4 py-6 space-y-6">
        {/* 统计概览 */}
        <div className="grid gap-4 md:grid-cols-2 lg:grid-cols-4">
          {/* 总座位数 */}
          <Card className="relative overflow-hidden border-0 shadow-md bg-gradient-to-br from-blue-500 to-blue-600 text-white">
            <CardContent className="p-5">
              <div className="flex items-center justify-between">
                <div>
                  <p className="text-sm font-medium text-blue-100">总座位数</p>
                  <p className="text-3xl font-bold mt-1">{totalSeats}</p>
                </div>
                <div className="flex h-12 w-12 items-center justify-center rounded-xl bg-white/20">
                  <Armchair className="h-6 w-6" />
                </div>
              </div>
            </CardContent>
          </Card>

          {/* 已占用 */}
          <Card className="relative overflow-hidden border-0 shadow-md bg-gradient-to-br from-rose-500 to-red-500 text-white">
            <CardContent className="p-5">
              <div className="flex items-center justify-between">
                <div>
                  <p className="text-sm font-medium text-rose-100">已占用</p>
                  <p className="text-3xl font-bold mt-1">{occupiedSeats}</p>
                </div>
                <div className="flex h-12 w-12 items-center justify-center rounded-xl bg-white/20">
                  <Users className="h-6 w-6" />
                </div>
              </div>
            </CardContent>
          </Card>

          {/* 空闲中 */}
          <Card className="relative overflow-hidden border-0 shadow-md bg-gradient-to-br from-emerald-500 to-green-500 text-white">
            <CardContent className="p-5">
              <div className="flex items-center justify-between">
                <div>
                  <p className="text-sm font-medium text-emerald-100">空闲中</p>
                  <p className="text-3xl font-bold mt-1">{idleSeats}</p>
                </div>
                <div className="flex h-12 w-12 items-center justify-center rounded-xl bg-white/20">
                  <CheckCircle2 className="h-6 w-6" />
                </div>
              </div>
            </CardContent>
          </Card>

          {/* 使用率 */}
          <Card className="relative overflow-hidden border-0 shadow-md bg-gradient-to-br from-violet-500 to-purple-500 text-white">
            <CardContent className="p-5">
              <div className="flex items-center justify-between">
                <div>
                  <p className="text-sm font-medium text-violet-100">使用率</p>
                  <p className="text-3xl font-bold mt-1">{usageRate}%</p>
                </div>
                <div className="flex h-12 w-12 items-center justify-center rounded-xl bg-white/20">
                  <Activity className="h-6 w-6" />
                </div>
              </div>
              <Progress value={usageRate} className="mt-3 h-1.5 bg-white/20" />
            </CardContent>
          </Card>
        </div>

        {/* 在线设备状态 */}
        <Card className="border-0 shadow-md">
          <CardContent className="p-4">
            <div className="flex items-center justify-between">
              <div className="flex items-center gap-3">
                <div className="flex h-10 w-10 items-center justify-center rounded-xl bg-slate-100 dark:bg-slate-800">
                  {onlineDevices === totalSeats && totalSeats > 0 ? (
                    <Wifi className="h-5 w-5 text-green-500" />
                  ) : (
                    <WifiOff className="h-5 w-5 text-amber-500" />
                  )}
                </div>
                <div>
                  <p className="font-medium text-slate-900 dark:text-white">设备在线状态</p>
                  <p className="text-sm text-slate-500 dark:text-slate-400">
                    {onlineDevices} / {totalSeats} 设备在线
                  </p>
                </div>
              </div>
              <div className="flex items-center gap-2">
                <div className={`h-2.5 w-2.5 rounded-full ${onlineDevices === totalSeats && totalSeats > 0 ? 'bg-green-500' : 'bg-amber-500'}`}></div>
                <span className="text-sm font-medium text-slate-600 dark:text-slate-300">
                  {onlineDevices === totalSeats && totalSeats > 0 ? '全部在线' : '部分离线'}
                </span>
              </div>
            </div>
          </CardContent>
        </Card>

        {/* 座位列表标题 */}
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-2">
            <Zap className="h-5 w-5 text-amber-500" />
            <h2 className="text-lg font-semibold text-slate-900 dark:text-white">
              实时状态
            </h2>
          </div>
          <Badge variant="outline" className="gap-1">
            <span className="h-1.5 w-1.5 rounded-full bg-green-500"></span>
            每5秒自动刷新
          </Badge>
        </div>

        {/* 座位网格 */}
        {loading ? (
          <div className="grid gap-4 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4">
            {[...Array(4)].map((_, i) => (
              <Card key={i} className="border-0 shadow-md">
                <CardContent className="p-4">
                  <Skeleton className="h-24 w-full" />
                </CardContent>
              </Card>
            ))}
          </div>
        ) : seats.length === 0 ? (
          <Card className="border-0 shadow-md">
            <CardContent className="flex flex-col items-center justify-center py-16">
              <div className="flex h-16 w-16 items-center justify-center rounded-2xl bg-slate-100 dark:bg-slate-800 mb-4">
                <MapPin className="h-8 w-8 text-slate-400" />
              </div>
              <p className="text-lg font-medium text-slate-900 dark:text-white mb-1">
                暂无座位数据
              </p>
              <p className="text-sm text-slate-500 dark:text-slate-400">
                请先连接 ESP32 设备进行注册
              </p>
            </CardContent>
          </Card>
        ) : (
          <div className="grid gap-4 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4">
            {seats.map((seat) => (
              <Card
                key={seat.id}
                className={`group cursor-pointer border-0 shadow-md transition-all duration-300 hover:shadow-xl ${
                  seat.isOccupied
                    ? 'bg-gradient-to-br from-red-50 to-rose-50 dark:from-red-950/50 dark:to-rose-950/50 ring-1 ring-red-200 dark:ring-red-800'
                    : 'bg-gradient-to-br from-green-50 to-emerald-50 dark:from-green-950/50 dark:to-emerald-950/50 ring-1 ring-green-200 dark:ring-green-800'
                }`}
                onClick={() => handleViewDetails(seat)}
              >
                <CardContent className="p-4">
                  {/* 状态指示器 */}
                  <div className="flex items-start justify-between mb-3">
                    <div className="flex-1 min-w-0">
                      <h3 className="font-semibold text-slate-900 dark:text-white truncate">
                        {seat.name}
                      </h3>
                      {seat.location && (
                        <div className="flex items-center gap-1 text-sm text-slate-500 dark:text-slate-400 mt-0.5">
                          <MapPin className="h-3 w-3 flex-shrink-0" />
                          <span className="truncate">{seat.location}</span>
                        </div>
                      )}
                    </div>
                    <div className={`flex h-10 w-10 items-center justify-center rounded-xl ${
                      seat.isOccupied 
                        ? 'bg-red-500 text-white' 
                        : 'bg-green-500 text-white'
                    }`}>
                      {seat.isOccupied ? (
                        <XCircle className="h-5 w-5" />
                      ) : (
                        <CheckCircle2 className="h-5 w-5" />
                      )}
                    </div>
                  </div>

                  {/* 状态标签 */}
                  <div className="flex items-center justify-between mb-3">
                    <Badge 
                      variant="outline" 
                      className={`font-medium ${
                        seat.isOccupied 
                          ? 'border-red-300 text-red-700 dark:border-red-700 dark:text-red-300' 
                          : 'border-green-300 text-green-700 dark:border-green-700 dark:text-green-300'
                      }`}
                    >
                      {seat.isOccupied ? '占用中' : '空闲'}
                    </Badge>
                    <div className="flex items-center gap-1 text-xs">
                      {seat.deviceStatus === 'online' ? (
                        <>
                          <div className="h-2 w-2 rounded-full bg-green-500"></div>
                          <span className="text-green-600 dark:text-green-400">在线</span>
                        </>
                      ) : (
                        <>
                          <div className="h-2 w-2 rounded-full bg-red-500"></div>
                          <span className="text-red-600 dark:text-red-400">离线</span>
                        </>
                      )}
                    </div>
                  </div>

                  {/* 传感器数据 */}
                  {seat.sensorData && (
                    <div className="flex flex-wrap gap-2 mb-3">
                      {seat.sensorData.temperature !== null && (
                        <div className="flex items-center gap-1 text-xs bg-blue-50 dark:bg-blue-950/30 px-2 py-1 rounded-md">
                          <Thermometer className="h-3 w-3 text-blue-500" />
                          <span className="text-blue-700 dark:text-blue-300">{seat.sensorData.temperature}°C</span>
                        </div>
                      )}
                      {seat.sensorData.humidity !== null && (
                        <div className="flex items-center gap-1 text-xs bg-cyan-50 dark:bg-cyan-950/30 px-2 py-1 rounded-md">
                          <Droplets className="h-3 w-3 text-cyan-500" />
                          <span className="text-cyan-700 dark:text-cyan-300">{seat.sensorData.humidity}%</span>
                        </div>
                      )}
                      {seat.sensorData.lightLevel !== null && (
                        <div className="flex items-center gap-1 text-xs bg-amber-50 dark:bg-amber-950/30 px-2 py-1 rounded-md">
                          <Sun className="h-3 w-3 text-amber-500" />
                          <span className="text-amber-700 dark:text-amber-300">{seat.sensorData.lightLevel}</span>
                        </div>
                      )}
                      {seat.sensorData.distance !== null && (
                        <div className="flex items-center gap-1 text-xs bg-purple-50 dark:bg-purple-950/30 px-2 py-1 rounded-md">
                          <Gauge className="h-3 w-3 text-purple-500" />
                          <span className="text-purple-700 dark:text-purple-300">{seat.sensorData.distance}cm</span>
                        </div>
                      )}
                    </div>
                  )}

                  {/* 更新时间 */}
                  <div className="flex items-center justify-between text-xs text-slate-400 dark:text-slate-500">
                    <div className="flex items-center gap-1">
                      <Timer className="h-3 w-3" />
                      <span>{getDuration(seat.lastUpdate)}</span>
                    </div>
                    <span className="opacity-0 group-hover:opacity-100 transition-opacity">
                      点击查看详情
                    </span>
                  </div>
                </CardContent>
              </Card>
            ))}
          </div>
        )}
      </main>

      {/* 底部版权 */}
      <footer className="border-t bg-white dark:bg-slate-900 mt-8">
        <div className="container mx-auto px-4 py-4">
          <div className="flex items-center justify-between text-sm text-slate-500 dark:text-slate-400">
            <span>© 2024 智能座位监测系统 - 毕业设计作品</span>
            <span>ESP32-S3 + 物联网技术</span>
          </div>
        </div>
      </footer>

      {/* 座位详情对话框 */}
      <Dialog open={!!selectedSeat} onOpenChange={() => setSelectedSeat(null)}>
        <DialogContent className="max-w-lg">
          <DialogHeader>
            <DialogTitle className="flex items-center gap-2">
              <div className={`flex h-8 w-8 items-center justify-center rounded-lg ${
                selectedSeat?.isOccupied ? 'bg-red-100 text-red-600 dark:bg-red-900 dark:text-red-300' : 'bg-green-100 text-green-600 dark:bg-green-900 dark:text-green-300'
              }`}>
                <Armchair className="h-4 w-4" />
              </div>
              {selectedSeat?.name}
            </DialogTitle>
            <DialogDescription>
              座位详细信息和使用记录
            </DialogDescription>
          </DialogHeader>

          {selectedSeat && (
            <div className="space-y-4">
              {/* 当前状态 */}
              <div className={`rounded-xl p-4 ${
                selectedSeat.isOccupied 
                  ? 'bg-red-50 dark:bg-red-950/50' 
                  : 'bg-green-50 dark:bg-green-950/50'
              }`}>
                <div className="flex items-center justify-between">
                  <div className="flex items-center gap-3">
                    {selectedSeat.isOccupied ? (
                      <XCircle className="h-8 w-8 text-red-500" />
                    ) : (
                      <CheckCircle2 className="h-8 w-8 text-green-500" />
                    )}
                    <div>
                      <p className="text-sm text-slate-500 dark:text-slate-400">当前状态</p>
                      <p className={`text-lg font-semibold ${
                        selectedSeat.isOccupied ? 'text-red-600 dark:text-red-400' : 'text-green-600 dark:text-green-400'
                      }`}>
                        {selectedSeat.isOccupied ? '占用中' : '空闲'}
                      </p>
                    </div>
                  </div>
                  <div className="text-right">
                    <p className="text-sm text-slate-500 dark:text-slate-400">设备状态</p>
                    <div className="flex items-center gap-1 justify-end">
                      {selectedSeat.deviceStatus === 'online' ? (
                        <>
                          <Wifi className="h-4 w-4 text-green-500" />
                          <span className="text-green-600 dark:text-green-400 font-medium">在线</span>
                        </>
                      ) : (
                        <>
                          <WifiOff className="h-4 w-4 text-red-500" />
                          <span className="text-red-600 dark:text-red-400 font-medium">离线</span>
                        </>
                      )}
                    </div>
                  </div>
                </div>
              </div>

              {/* 详细信息 */}
              <div className="grid grid-cols-2 gap-3">
                {selectedSeat.location && (
                  <div className="rounded-lg border p-3">
                    <p className="text-xs text-slate-500 dark:text-slate-400 mb-1">位置</p>
                    <p className="font-medium text-sm">{selectedSeat.location}</p>
                  </div>
                )}
                <div className="rounded-lg border p-3">
                  <p className="text-xs text-slate-500 dark:text-slate-400 mb-1">座位ID</p>
                  <p className="font-medium text-sm">#{selectedSeat.id}</p>
                </div>
                <div className="rounded-lg border p-3 col-span-2">
                  <p className="text-xs text-slate-500 dark:text-slate-400 mb-1">最后更新</p>
                  <p className="font-medium text-sm">
                    {new Date(selectedSeat.lastUpdate).toLocaleString('zh-CN')}
                  </p>
                </div>
              </div>

              {/* 传感器数据详情 */}
              {selectedSeat.sensorData && (
                <div className="space-y-3">
                  <div className="flex items-center gap-2">
                    <Radio className="h-4 w-4 text-slate-500" />
                    <span className="text-sm font-medium">传感器数据</span>
                  </div>
                  <div className="grid grid-cols-3 gap-2">
                    {selectedSeat.sensorData.temperature !== null && (
                      <div className="rounded-lg bg-blue-50 dark:bg-blue-950/30 p-3 text-center">
                        <Thermometer className="h-5 w-5 mx-auto text-blue-500 mb-1" />
                        <p className="text-lg font-bold text-blue-700 dark:text-blue-300">{selectedSeat.sensorData.temperature}°C</p>
                        <p className="text-xs text-blue-500">温度</p>
                      </div>
                    )}
                    {selectedSeat.sensorData.humidity !== null && (
                      <div className="rounded-lg bg-cyan-50 dark:bg-cyan-950/30 p-3 text-center">
                        <Droplets className="h-5 w-5 mx-auto text-cyan-500 mb-1" />
                        <p className="text-lg font-bold text-cyan-700 dark:text-cyan-300">{selectedSeat.sensorData.humidity}%</p>
                        <p className="text-xs text-cyan-500">湿度</p>
                      </div>
                    )}
                    {selectedSeat.sensorData.lightLevel !== null && (
                      <div className="rounded-lg bg-amber-50 dark:bg-amber-950/30 p-3 text-center">
                        <Sun className="h-5 w-5 mx-auto text-amber-500 mb-1" />
                        <p className="text-lg font-bold text-amber-700 dark:text-amber-300">{selectedSeat.sensorData.lightLevel}</p>
                        <p className="text-xs text-amber-500">光照</p>
                      </div>
                    )}
                    {selectedSeat.sensorData.pressure1 !== null && (
                      <div className="rounded-lg bg-red-50 dark:bg-red-950/30 p-3 text-center">
                        <Gauge className="h-5 w-5 mx-auto text-red-500 mb-1" />
                        <p className="text-lg font-bold text-red-700 dark:text-red-300">
                          {selectedSeat.sensorData.pressure1 ? '有' : '无'}
                        </p>
                        <p className="text-xs text-red-500">压力1</p>
                      </div>
                    )}
                    {selectedSeat.sensorData.pressure2 !== null && (
                      <div className="rounded-lg bg-orange-50 dark:bg-orange-950/30 p-3 text-center">
                        <Gauge className="h-5 w-5 mx-auto text-orange-500 mb-1" />
                        <p className="text-lg font-bold text-orange-700 dark:text-orange-300">
                          {selectedSeat.sensorData.pressure2 ? '有' : '无'}
                        </p>
                        <p className="text-xs text-orange-500">压力2</p>
                      </div>
                    )}
                    {selectedSeat.sensorData.distance !== null && (
                      <div className="rounded-lg bg-purple-50 dark:bg-purple-950/30 p-3 text-center">
                        <Activity className="h-5 w-5 mx-auto text-purple-500 mb-1" />
                        <p className="text-lg font-bold text-purple-700 dark:text-purple-300">{selectedSeat.sensorData.distance}cm</p>
                        <p className="text-xs text-purple-500">距离</p>
                      </div>
                    )}
                    {selectedSeat.sensorData.pirState !== null && (
                      <div className="rounded-lg bg-green-50 dark:bg-green-950/30 p-3 text-center">
                        <Users className="h-5 w-5 mx-auto text-green-500 mb-1" />
                        <p className="text-lg font-bold text-green-700 dark:text-green-300">
                          {selectedSeat.sensorData.pirState ? '检测' : '无'}
                        </p>
                        <p className="text-xs text-green-500">PIR</p>
                      </div>
                    )}
                    {selectedSeat.sensorData.wifiSignal !== null && (
                      <div className="rounded-lg bg-slate-50 dark:bg-slate-950/30 p-3 text-center">
                        <Wifi className="h-5 w-5 mx-auto text-slate-500 mb-1" />
                        <p className="text-lg font-bold text-slate-700 dark:text-slate-300">{selectedSeat.sensorData.wifiSignal}dBm</p>
                        <p className="text-xs text-slate-500">信号</p>
                      </div>
                    )}
                    {selectedSeat.sensorData.duration !== null && (
                      <div className="rounded-lg bg-indigo-50 dark:bg-indigo-950/30 p-3 text-center">
                        <Timer className="h-5 w-5 mx-auto text-indigo-500 mb-1" />
                        <p className="text-lg font-bold text-indigo-700 dark:text-indigo-300">{selectedSeat.sensorData.duration}s</p>
                        <p className="text-xs text-indigo-500">持续</p>
                      </div>
                    )}
                  </div>
                </div>
              )}

              {/* 历史记录 */}
              <div>
                <div className="flex items-center gap-2 mb-3">
                  <History className="h-4 w-4 text-slate-500" />
                  <span className="text-sm font-medium">最近状态变化</span>
                </div>
                
                {historyLoading ? (
                  <div className="space-y-2">
                    {[...Array(3)].map((_, i) => (
                      <Skeleton key={i} className="h-10 w-full" />
                    ))}
                  </div>
                ) : historyData.length === 0 ? (
                  <p className="text-sm text-slate-500 dark:text-slate-400 text-center py-4">
                    暂无历史记录
                  </p>
                ) : (
                  <div className="space-y-2 max-h-48 overflow-y-auto">
                    {historyData.slice(0, 10).map((record, index) => (
                      <div 
                        key={record.id}
                        className={`flex items-center justify-between p-2 rounded-lg ${
                          record.isOccupied 
                            ? 'bg-red-50 dark:bg-red-950/30' 
                            : 'bg-green-50 dark:bg-green-950/30'
                        }`}
                      >
                        <div className="flex items-center gap-2">
                          {record.isOccupied ? (
                            <XCircle className="h-4 w-4 text-red-500" />
                          ) : (
                            <CheckCircle2 className="h-4 w-4 text-green-500" />
                          )}
                          <span className="text-sm">
                            {record.isOccupied ? '变为占用' : '变为空闲'}
                          </span>
                        </div>
                        <span className="text-xs text-slate-500 dark:text-slate-400">
                          {new Date(record.recordedAt).toLocaleString('zh-CN', {
                            month: 'numeric',
                            day: 'numeric',
                            hour: '2-digit',
                            minute: '2-digit',
                          })}
                        </span>
                      </div>
                    ))}
                  </div>
                )}
              </div>
            </div>
          )}
        </DialogContent>
      </Dialog>
    </div>
  );
}

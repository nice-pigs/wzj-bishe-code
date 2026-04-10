import { NextRequest, NextResponse } from 'next/server';
import { getSupabaseClient } from '@/storage/database/supabase-client';

/**
 * 座位查询接口
 * GET /api/seats
 * 
 * 查询参数:
 * - location: 位置筛选（可选）
 * - status: 状态筛选（occupied/idle，可选）
 * 
 * 响应:
 * {
 *   "success": true,
 *   "data": [
 *     {
 *       "id": 1,
 *       "name": "A区-001",
 *       "location": "图书馆一楼",
 *       "isOccupied": false,
 *       "deviceStatus": "online",
 *       "lastUpdate": "2024-01-01T12:00:00Z",
 *       "sensorData": {
 *         "temperature": 26,
 *         "humidity": 44,
 *         "lightLevel": 25605,
 *         "pressure1": true,
 *         "pressure2": false,
 *         "distance": 15,
 *         "pirState": false,
 *         "wifiSignal": -55,
 *         "detectionScore": 95,
 *         "duration": 120
 *       }
 *     }
 *   ]
 * }
 */
export async function GET(request: NextRequest) {
  try {
    const { searchParams } = new URL(request.url);
    const location = searchParams.get('location');
    const status = searchParams.get('status');

    const client = getSupabaseClient();

    // 查询座位信息
    let query = client
      .from('seats')
      .select(`
        id,
        name,
        location,
        is_occupied,
        created_at,
        updated_at,
        device_id
      `)
      .order('name', { ascending: true });

    // 位置筛选
    if (location) {
      query = query.ilike('location', `%${location}%`);
    }

    // 状态筛选
    if (status === 'occupied') {
      query = query.eq('is_occupied', true);
    } else if (status === 'idle') {
      query = query.eq('is_occupied', false);
    }

    const { data: seats, error } = await query;

    if (error) {
      console.error('查询座位失败:', error);
      return NextResponse.json(
        { success: false, error: '查询座位失败' },
        { status: 500 }
      );
    }

    // 查询设备状态
    const deviceIds = seats?.map(s => s.device_id).filter(Boolean) || [];
    let devices: any[] = [];
    
    if (deviceIds.length > 0) {
      const { data: deviceData } = await client
        .from('devices')
        .select('id, status, last_heartbeat')
        .in('id', deviceIds);
      
      devices = deviceData || [];
    }

    // 查询最新传感器数据
    const seatIds = seats?.map(s => s.id) || [];
    let sensorDataMap: Record<number, any> = {};
    
    if (seatIds.length > 0) {
      // 查询每个座位的最新传感器数据
      const { data: allSensorData, error: sensorError } = await client
        .from('seat_sensor_data')
        .select('*')
        .in('seat_id', seatIds)
        .order('recorded_at', { ascending: false });

      if (sensorError) {
        console.error('查询传感器数据失败:', sensorError);
      }

      // 按座位ID分组，只取每个座位的最新记录
      if (allSensorData && Array.isArray(allSensorData)) {
        for (const sensor of allSensorData) {
          if (!sensorDataMap[sensor.seat_id]) {
            sensorDataMap[sensor.seat_id] = {
              temperature: sensor.temperature,
              humidity: sensor.humidity,
              lightLevel: sensor.light_level,
              pressure1: sensor.pressure1,
              pressure2: sensor.pressure2,
              distance: sensor.distance,
              pirState: sensor.pir_state,
              wifiSignal: sensor.wifi_signal,
              detectionScore: sensor.detection_score,
              duration: sensor.duration,
              recordedAt: sensor.recorded_at,
            };
          }
        }
      }
    }

    // 组装响应数据
    const result = seats?.map(seat => {
      const device = devices.find(d => d.id === seat.device_id);
      
      // 判断设备是否离线（超过30秒没有心跳）
      let deviceStatus = device?.status || 'offline';
      if (device?.last_heartbeat) {
        const lastHeartbeat = new Date(device.last_heartbeat).getTime();
        const now = Date.now();
        if (now - lastHeartbeat > 30000) {
          deviceStatus = 'offline';
        }
      }

      return {
        id: seat.id,
        name: seat.name,
        location: seat.location,
        isOccupied: seat.is_occupied,
        deviceStatus,
        lastUpdate: seat.updated_at || seat.created_at,
        sensorData: sensorDataMap[seat.id] || null,
      };
    }) || [];

    return NextResponse.json({
      success: true,
      data: result,
      total: result.length,
    });
  } catch (error) {
    console.error('查询座位异常:', error);
    return NextResponse.json(
      { success: false, error: '服务器内部错误' },
      { status: 500 }
    );
  }
}

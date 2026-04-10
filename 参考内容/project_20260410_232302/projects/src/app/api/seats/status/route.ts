import { NextRequest, NextResponse } from 'next/server';
import { getSupabaseClient } from '@/storage/database/supabase-client';

/**
 * 座位状态上报接口
 * POST /api/seats/status
 * 
 * 请求体:
 * {
 *   "deviceKey": "设备唯一密钥",
 *   "isOccupied": true/false,
 *   "temperature": 26,        // 温度(可选)
 *   "humidity": 44,           // 湿度(可选)
 *   "lightLevel": 25605,      // 光照强度(可选)
 *   "pressure1": true,         // 压力传感器1(可选)
 *   "pressure2": true,         // 压力传感器2(可选)
 *   "distance": 15,            // 超声波距离(可选)
 *   "pirState": false,         // PIR状态(可选)
 *   "wifiSignal": -55,         // WiFi信号强度(可选)
 *   "detectionScore": 95,      // 检测置信度(可选)
 *   "duration": 120            // 持续时间(可选)
 * }
 * 
 * 响应:
 * {
 *   "success": true,
 *   "data": {
 *     "seatId": 1,
 *     "seatName": "A区-001",
 *     "isOccupied": true
 *   }
 * }
 */
export async function POST(request: NextRequest) {
  try {
    const body = await request.json();
    const { 
      deviceKey, 
      isOccupied,
      temperature,
      humidity,
      lightLevel,
      pressure1,
      pressure2,
      distance,
      pirState,
      wifiSignal,
      detectionScore,
      duration
    } = body;

    // 参数验证
    if (!deviceKey || typeof isOccupied !== 'boolean') {
      return NextResponse.json(
        { success: false, error: '缺少必要参数: deviceKey, isOccupied' },
        { status: 400 }
      );
    }

    const client = getSupabaseClient();

    // 查询设备信息
    const { data: devices, error: deviceError } = await client
      .from('devices')
      .select('*')
      .eq('device_key', deviceKey)
      .limit(1);

    if (deviceError) {
      console.error('查询设备错误:', deviceError);
      return NextResponse.json(
        { success: false, error: '数据库查询错误' },
        { status: 500 }
      );
    }

    if (!devices || devices.length === 0) {
      return NextResponse.json(
        { success: false, error: '设备未注册' },
        { status: 404 }
      );
    }

    const device = devices[0];

    // 更新设备心跳时间
    await client
      .from('devices')
      .update({
        status: 'online',
        last_heartbeat: new Date().toISOString(),
        updated_at: new Date().toISOString(),
      })
      .eq('id', device.id);

    // 查询设备关联的座位
    let seatId = device.seat_id;
    let isNewSeat = false;

    // 如果设备没有关联座位，自动创建一个
    if (!seatId) {
      const { data: newSeat, error: seatError } = await client
        .from('seats')
        .insert({
          name: `座位-${device.device_name}`,
          device_id: device.id,
          is_occupied: isOccupied,
        })
        .select()
        .single();

      if (seatError) {
        console.error('创建座位失败:', seatError);
        return NextResponse.json(
          { success: false, error: '创建座位失败' },
          { status: 500 }
        );
      }

      seatId = newSeat.id;
      isNewSeat = true;

      // 更新设备的座位关联
      await client
        .from('devices')
        .update({ seat_id: seatId })
        .eq('id', device.id);
    } else {
      // 更新座位状态
      await client
        .from('seats')
        .update({
          is_occupied: isOccupied,
          updated_at: new Date().toISOString(),
        })
        .eq('id', seatId);
    }

    // 记录状态变化到日志表
    const { error: logError } = await client.from('seat_status_logs').insert({
      seat_id: seatId,
      is_occupied: isOccupied,
    });

    if (logError) {
      console.error('记录状态日志失败:', logError);
    }

    // 存储传感器数据(如果有任何传感器数据)
    const hasSensorData = [
      temperature, humidity, lightLevel, 
      pressure1, pressure2, distance, 
      pirState, wifiSignal, detectionScore, duration
    ].some(v => v !== undefined);

    if (hasSensorData) {
      const sensorData: Record<string, any> = {
        seat_id: seatId,
        recorded_at: new Date().toISOString(),
      };

      if (temperature !== undefined) sensorData.temperature = temperature;
      if (humidity !== undefined) sensorData.humidity = humidity;
      if (lightLevel !== undefined) sensorData.light_level = lightLevel;
      if (pressure1 !== undefined) sensorData.pressure1 = pressure1;
      if (pressure2 !== undefined) sensorData.pressure2 = pressure2;
      if (distance !== undefined) sensorData.distance = distance;
      if (pirState !== undefined) sensorData.pir_state = pirState;
      if (wifiSignal !== undefined) sensorData.wifi_signal = wifiSignal;
      if (detectionScore !== undefined) sensorData.detection_score = detectionScore;
      if (duration !== undefined) sensorData.duration = duration;

      const { error: sensorError } = await client
        .from('seat_sensor_data')
        .insert(sensorData);

      if (sensorError) {
        console.error('存储传感器数据失败:', sensorError);
      }
    }

    // 查询座位完整信息
    const { data: seat } = await client
      .from('seats')
      .select('*')
      .eq('id', seatId)
      .single();

    return NextResponse.json({
      success: true,
      data: {
        seatId: seat?.id,
        seatName: seat?.name,
        isOccupied: seat?.is_occupied,
        sensorDataSaved: hasSensorData,
      },
    });
  } catch (error) {
    console.error('状态上报异常:', error);
    return NextResponse.json(
      { success: false, error: '服务器内部错误' },
      { status: 500 }
    );
  }
}

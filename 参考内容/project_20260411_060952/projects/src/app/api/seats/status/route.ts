import { NextRequest, NextResponse } from 'next/server';
import { getSupabaseClient } from '@/storage/database/supabase-client';

/**
 * 座位状态上报接口
 * POST /api/seats/status
 * 
 * 请求体:
 * {
 *   "deviceKey": "设备唯一密钥",
 *   "deviceName": "设备名称(可选)",
 *   "isOccupied": true/false,
 *   "temperature": 26,        // 温度(可选)
 *   "humidity": 44,           // 湿度(可选)
 *   "lightLevel": 25605,      // 光照强度(可选)
 *   "pressure1": 0/1,         // 压力传感器1(可选)
 *   "pressure2": 0/1,         // 压力传感器2(可选)
 *   "distance": 15,           // 超声波距离(可选)
 *   "pirState": 0/1,           // PIR状态(可选)
 *   "wifiSignal": -55,         // WiFi信号强度(可选)
 *   "detectionScore": 95,      // 检测置信度(可选)
 *   "duration": 120            // 持续时间(可选)
 * }
 */
export async function POST(request: NextRequest) {
  try {
    const body = await request.json();
    const { 
      deviceKey, 
      deviceName,
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
    let device: DeviceRecord | null = null;
    let seatId: number | null = null;
    let sensorDataSaved = false;

    interface DeviceRecord {
      id: number;
      device_key: string;
      device_name: string | null;
      seat_id: number | null;
    }

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

    if (devices && devices.length > 0) {
      // 设备已存在
      device = devices[0];
      console.log(`[DEVICE] 找到已注册设备: ${deviceKey}`);
    } else {
      // 设备不存在，尝试upsert
      const deviceData = {
        device_key: deviceKey,
        device_name: deviceName || `设备-${deviceKey}`,
        status: 'online',
        last_heartbeat: new Date().toISOString(),
      };
      
      const { data: newDevice, error: createError } = await client
        .from('devices')
        .upsert(deviceData, { onConflict: 'device_key' })
        .select()
        .single();

      if (createError) {
        console.error('创建设备失败:', createError);
        // 尝试直接插入
        const { data: insertResult, error: insertError } = await client
          .from('devices')
          .insert(deviceData)
          .select()
          .single();
          
        if (insertError) {
          console.error('插入设备失败:', insertError);
          // 继续处理，使用临时数据
        }
        device = insertResult || { id: 0, device_key: deviceKey, device_name: deviceName, seat_id: null };
      } else {
        console.log(`[DEVICE] 自动创建设备: ${deviceKey}`);
        device = newDevice;
      }
    }

    // 更新设备心跳时间
    if (device && device.id > 0) {
      await client
        .from('devices')
        .update({
          status: 'online',
          last_heartbeat: new Date().toISOString(),
          updated_at: new Date().toISOString(),
        })
        .eq('id', device.id);
      
      seatId = device.seat_id;
    }

    // 如果设备没有关联座位，自动创建一个
    if (!seatId) {
      const seatName = `座位-${deviceName || deviceKey}`;
      
      const { data: newSeat, error: seatError } = await client
        .from('seats')
        .insert({
          name: seatName,
          device_id: device?.id || null,
          is_occupied: isOccupied,
        })
        .select()
        .single();

      if (seatError) {
        console.error('创建座位失败:', seatError);
        // 尝试使用现有座位
        const { data: existingSeats } = await client
          .from('seats')
          .select('id')
          .limit(1);
          
        if (existingSeats && existingSeats.length > 0) {
          seatId = existingSeats[0].id;
        } else {
          return NextResponse.json(
            { success: false, error: '无法创建或查找座位' },
            { status: 500 }
          );
        }
      } else {
        seatId = newSeat.id;
      }

      // 更新设备的座位关联
      if (device && device.id > 0) {
        await client
          .from('devices')
          .update({ seat_id: seatId })
          .eq('id', device.id);
      }
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
    if (seatId) {
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
        const sensorData: Record<string, number | boolean | string> = {
          seat_id: seatId,
          recorded_at: new Date().toISOString(),
        };

        if (temperature !== undefined) sensorData.temperature = temperature;
        if (humidity !== undefined) sensorData.humidity = humidity;
        if (lightLevel !== undefined) sensorData.light_level = lightLevel;
        if (pressure1 !== undefined) sensorData.pressure1 = pressure1 === 1 || pressure1 === true;
        if (pressure2 !== undefined) sensorData.pressure2 = pressure2 === 1 || pressure2 === true;
        if (distance !== undefined) sensorData.distance = distance;
        if (pirState !== undefined) sensorData.pir_state = pirState === 1 || pirState === true;
        if (wifiSignal !== undefined) sensorData.wifi_signal = wifiSignal;
        if (detectionScore !== undefined) sensorData.detection_score = detectionScore;
        if (duration !== undefined) sensorData.duration = duration;

        const { error: sensorError } = await client
          .from('seat_sensor_data')
          .insert(sensorData);

        if (sensorError) {
          console.error('存储传感器数据失败:', sensorError);
        } else {
          console.log(`[SENSOR] 传感器数据已保存`);
          sensorDataSaved = true;
        }
      }
    }

    // 查询座位完整信息
    let seat = null;
    if (seatId) {
      const { data: seatData } = await client
        .from('seats')
        .select('*')
        .eq('id', seatId)
        .single();
      seat = seatData;
    }

    return NextResponse.json({
      success: true,
      data: {
        seatId: seat?.id,
        seatName: seat?.name,
        isOccupied: seat?.is_occupied,
        sensorDataSaved,
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

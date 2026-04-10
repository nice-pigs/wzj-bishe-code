import { NextRequest, NextResponse } from 'next/server';
import { getSupabaseClient } from '@/storage/database/supabase-client';

/**
 * 设备注册接口
 * POST /api/devices/register
 * 
 * 请求体:
 * {
 *   "deviceKey": "设备唯一密钥",
 *   "deviceName": "设备名称"
 * }
 * 
 * 响应:
 * {
 *   "success": true,
 *   "data": {
 *     "deviceId": 1,
 *     "deviceKey": "xxx",
 *     "deviceName": "设备名称"
 *   }
 * }
 */
export async function POST(request: NextRequest) {
  try {
    const body = await request.json();
    const { deviceKey, deviceName } = body;

    // 参数验证
    if (!deviceKey || !deviceName) {
      return NextResponse.json(
        { success: false, error: '缺少必要参数: deviceKey, deviceName' },
        { status: 400 }
      );
    }

    const client = getSupabaseClient();

    // 检查设备是否已存在
    const { data: existingDevices, error: checkError } = await client
      .from('devices')
      .select('*')
      .eq('device_key', deviceKey)
      .limit(1);

    const existingDevice = existingDevices && existingDevices.length > 0 ? existingDevices[0] : null;

    if (existingDevice) {
      // 设备已存在，更新心跳时间
      const { data, error } = await client
        .from('devices')
        .update({
          status: 'online',
          last_heartbeat: new Date().toISOString(),
          updated_at: new Date().toISOString(),
        })
        .eq('id', existingDevice.id)
        .select()
        .single();

      if (error) {
        console.error('更新设备失败:', error);
        return NextResponse.json(
          { success: false, error: '更新设备失败' },
          { status: 500 }
        );
      }

      return NextResponse.json({
        success: true,
        data: {
          deviceId: data.id,
          deviceKey: data.device_key,
          deviceName: data.device_name,
        },
      });
    }

    // 创建新设备
    const { data, error } = await client
      .from('devices')
      .insert({
        device_key: deviceKey,
        device_name: deviceName,
        status: 'online',
        last_heartbeat: new Date().toISOString(),
      })
      .select()
      .single();

    if (error) {
      console.error('创建设备失败:', error);
      return NextResponse.json(
        { success: false, error: '创建设备失败' },
        { status: 500 }
      );
    }

    return NextResponse.json({
      success: true,
      data: {
        deviceId: data.id,
        deviceKey: data.device_key,
        deviceName: data.device_name,
      },
    });
  } catch (error) {
    console.error('设备注册异常:', error);
    return NextResponse.json(
      { success: false, error: '服务器内部错误' },
      { status: 500 }
    );
  }
}

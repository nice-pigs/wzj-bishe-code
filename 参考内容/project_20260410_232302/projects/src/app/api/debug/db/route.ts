import { NextRequest, NextResponse } from 'next/server';
import { getSupabaseClient } from '@/storage/database/supabase-client';

/**
 * 数据库数据查看接口
 * GET /api/debug/db
 */
export async function GET(request: NextRequest) {
  try {
    const client = getSupabaseClient();
    
    // 查询座位
    const { data: seats, error: seatsError } = await client
      .from('seats')
      .select('*')
      .order('updated_at', { ascending: false });
    
    // 查询设备
    const { data: devices, error: devicesError } = await client
      .from('devices')
      .select('*')
      .order('created_at', { ascending: false });
    
    // 查询最近的状态日志
    const { data: logs, error: logsError } = await client
      .from('seat_status_logs')
      .select('*')
      .order('recorded_at', { ascending: false })
      .limit(20);

    return NextResponse.json({
      success: true,
      data: {
        seats: {
          count: seats?.length || 0,
          items: seats,
          error: seatsError?.message
        },
        devices: {
          count: devices?.length || 0,
          items: devices,
          error: devicesError?.message
        },
        logs: {
          count: logs?.length || 0,
          items: logs,
          error: logsError?.message
        }
      }
    });
  } catch (error) {
    return NextResponse.json({
      success: false,
      error: error instanceof Error ? error.message : 'Unknown error'
    }, { status: 500 });
  }
}

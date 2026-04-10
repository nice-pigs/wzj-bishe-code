import { NextRequest, NextResponse } from 'next/server';
import { getSupabaseClient, getSupabaseCredentials } from '@/storage/database/supabase-client';

/**
 * 测试接口：直接查询设备
 * GET /api/debug/query?deviceKey=xxx
 */
export async function GET(request: NextRequest) {
  const deviceKey = request.nextUrl.searchParams.get('deviceKey');
  
  try {
    const { url: dbUrl } = getSupabaseCredentials();
    const client = getSupabaseClient();
    
    console.log('调试查询:', { deviceKey, dbUrl: dbUrl?.substring(0, 50) });
    
    // 方法1：直接查询
    const { data: method1, error: error1 } = await client
      .from('devices')
      .select('*')
      .eq('device_key', deviceKey);
    
    console.log('查询结果:', { deviceKey, count: method1?.length });
    
    // 方法2：获取所有设备
    const { data: allDevices } = await client
      .from('devices')
      .select('id, device_key')
      .order('id', { ascending: false })
      .limit(5);
    
    return NextResponse.json({
      query: { deviceKey },
      dbUrl: dbUrl?.substring(0, 50),
      method1: { data: method1, error: error1?.message },
      latestDevices: allDevices
    });
  } catch (error) {
    return NextResponse.json({
      error: error instanceof Error ? error.message : 'Unknown error'
    }, { status: 500 });
  }
}

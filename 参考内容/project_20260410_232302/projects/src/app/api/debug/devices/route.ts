import { NextRequest, NextResponse } from 'next/server';
import { getSupabaseClient } from '@/storage/database/supabase-client';

/**
 * 调试接口：查询所有设备
 * GET /api/debug/devices
 */
export async function GET(request: NextRequest) {
  try {
    const client = getSupabaseClient();
    
    // 查询所有设备
    const { data: devices, error } = await client
      .from('devices')
      .select('*')
      .order('created_at', { ascending: false })
      .limit(20);

    if (error) {
      return NextResponse.json({ 
        success: false, 
        error: error.message,
        details: error 
      }, { status: 500 });
    }

    return NextResponse.json({
      success: true,
      count: devices?.length || 0,
      data: devices
    });
  } catch (error) {
    return NextResponse.json({
      success: false,
      error: error instanceof Error ? error.message : 'Unknown error'
    }, { status: 500 });
  }
}

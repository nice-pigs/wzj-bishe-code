import { NextRequest, NextResponse } from 'next/server';
import { getSupabaseClient } from '@/storage/database/supabase-client';

/**
 * 座位历史记录查询接口
 * GET /api/seats/[id]/history
 * 
 * 查询参数:
 * - limit: 返回记录数量（默认50，最多200）
 * - startTime: 开始时间（ISO格式，可选）
 * - endTime: 结束时间（ISO格式，可选）
 * 
 * 响应:
 * {
 *   "success": true,
 *   "data": [
 *     {
 *       "id": 1,
 *       "seatId": 1,
 *       "isOccupied": true,
 *       "recordedAt": "2024-01-01T12:00:00Z"
 *     }
 *   ]
 * }
 */
export async function GET(
  request: NextRequest,
  { params }: { params: Promise<{ id: string }> }
) {
  try {
    const { id } = await params;
    const seatId = parseInt(id);

    if (isNaN(seatId)) {
      return NextResponse.json(
        { success: false, error: '无效的座位ID' },
        { status: 400 }
      );
    }

    const { searchParams } = new URL(request.url);
    const limit = Math.min(parseInt(searchParams.get('limit') || '50'), 200);
    const startTime = searchParams.get('startTime');
    const endTime = searchParams.get('endTime');

    const client = getSupabaseClient();

    // 验证座位是否存在
    const { data: seat, error: seatError } = await client
      .from('seats')
      .select('id, name')
      .eq('id', seatId)
      .single();

    if (!seat) {
      return NextResponse.json(
        { success: false, error: '座位不存在' },
        { status: 404 }
      );
    }

    // 查询历史记录
    let query = client
      .from('seat_status_logs')
      .select('*')
      .eq('seat_id', seatId)
      .order('recorded_at', { ascending: false })
      .limit(limit);

    // 时间范围筛选
    if (startTime) {
      query = query.gte('recorded_at', startTime);
    }
    if (endTime) {
      query = query.lte('recorded_at', endTime);
    }

    const { data: logs, error } = await query;

    if (error) {
      console.error('查询历史记录失败:', error);
      return NextResponse.json(
        { success: false, error: '查询历史记录失败' },
        { status: 500 }
      );
    }

    const result = logs?.map(log => ({
      id: log.id,
      seatId: log.seat_id,
      isOccupied: log.is_occupied,
      recordedAt: log.recorded_at,
    })) || [];

    return NextResponse.json({
      success: true,
      data: result,
      seat: {
        id: seat.id,
        name: seat.name,
      },
      total: result.length,
    });
  } catch (error) {
    console.error('查询历史记录异常:', error);
    return NextResponse.json(
      { success: false, error: '服务器内部错误' },
      { status: 500 }
    );
  }
}

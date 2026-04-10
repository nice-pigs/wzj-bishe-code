import { createClient, SupabaseClient } from '@supabase/supabase-js';
import { execSync } from 'child_process';
import path from 'path';

const supabaseClient: SupabaseClient | null = null;
let envLoaded = false;

interface SupabaseCredentials {
  url: string;
  anonKey: string;
}

/**
 * 从 Coze 工作负载身份获取环境变量
 */
function loadEnvFromCoze(): void {
  if (envLoaded) return;

  // 首先检查是否已经有环境变量
  if (process.env.COZE_SUPABASE_URL && process.env.COZE_SUPABASE_ANON_KEY) {
    envLoaded = true;
    return;
  }

  // 尝试从 .env 文件加载
  try {
    // dotenv config is handled elsewhere
    if (process.env.NEXT_PUBLIC_SUPABASE_URL && process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY) {
      envLoaded = true;
      return;
    }
  } catch {
    // dotenv 不可用
  }

  // 尝试通过 Python 脚本从 Coze 工作负载身份获取环境变量
  try {
    const scriptPath = path.join(process.cwd(), 'scripts', 'get-env.py');
    const output = execSync(`python3 "${scriptPath}"`, {
      encoding: 'utf-8',
      timeout: 10000,
      stdio: ['pipe', 'pipe', 'pipe'],
    });

    const lines = output.trim().split('\n');
    for (const line of lines) {
      if (line.startsWith('#') || !line.includes('=')) continue;

      const eqIndex = line.indexOf('=');
      const key = line.substring(0, eqIndex);
      let value = line.substring(eqIndex + 1);

      // 处理转义的换行符
      value = value.replace(/\\n/g, '\n');

      // 移除引号
      if ((value.startsWith("'") && value.endsWith("'")) ||
          (value.startsWith('"') && value.endsWith('"'))) {
        value = value.slice(1, -1);
      }

      if (!process.env[key]) {
        process.env[key] = value;
      }
    }

    envLoaded = true;
  } catch (error) {
    console.error('Failed to load env from Coze:', error);
    // 不抛出错误，让后续代码尝试其他方式
  }
}

function getSupabaseCredentials(): SupabaseCredentials {
  // 尝试加载环境变量
  loadEnvFromCoze();

  // 获取 Supabase URL 和 Key
  // 优先使用 NEXT_PUBLIC_ 前缀的变量（生产环境）
  // 然后尝试 COZE_ 前缀的变量（开发环境）
  const url = process.env.NEXT_PUBLIC_SUPABASE_URL || process.env.COZE_SUPABASE_URL;
  const anonKey = process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY || process.env.COZE_SUPABASE_ANON_KEY;

  if (!url) {
    throw new Error('Supabase URL is not configured. Please set NEXT_PUBLIC_SUPABASE_URL');
  }
  if (!anonKey) {
    throw new Error('Supabase anon key is not configured. Please set NEXT_PUBLIC_SUPABASE_ANON_KEY');
  }

  return { url, anonKey };
}

function getSupabaseClient(token?: string): SupabaseClient {
  // 每次都创建新客户端，避免缓存问题
  const { url, anonKey } = getSupabaseCredentials();

  const client = createClient(url, anonKey, {
    global: token ? {
      headers: { Authorization: `Bearer ${token}` },
    } : undefined,
    db: {
      timeout: 60000,
    },
    auth: {
      autoRefreshToken: false,
      persistSession: false,
    },
  });

  return client;
}

export { loadEnvFromCoze, getSupabaseCredentials, getSupabaseClient };

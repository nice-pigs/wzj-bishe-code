#!/bin/bash
set -Eeuo pipefail

COZE_WORKSPACE_PATH="${COZE_WORKSPACE_PATH:-$(pwd)}"
PORT=5000
DEPLOY_RUN_PORT="${DEPLOY_RUN_PORT:-$PORT}"

# 加载 Coze 环境变量到 .env 文件
load_coze_env() {
    echo "Loading environment variables from Coze..."
    
    # 尝试使用 Python 脚本获取环境变量
    if command -v python3 &> /dev/null; then
        python3 "${COZE_WORKSPACE_PATH}/scripts/get-env.py" > "${COZE_WORKSPACE_PATH}/.env" 2>/dev/null || true
        
        if [ -f "${COZE_WORKSPACE_PATH}/.env" ]; then
            echo "Environment variables loaded successfully"
            # 导出环境变量
            set -a
            source "${COZE_WORKSPACE_PATH}/.env"
            set +a
        fi
    fi
}

start_service() {
    cd "${COZE_WORKSPACE_PATH}"
    
    # 加载环境变量
    load_coze_env
    
    echo "Starting HTTP service on port ${DEPLOY_RUN_PORT} for deploy..."
    npx next start --port ${DEPLOY_RUN_PORT}
}

echo "Starting HTTP service on port ${DEPLOY_RUN_PORT} for deploy..."
start_service

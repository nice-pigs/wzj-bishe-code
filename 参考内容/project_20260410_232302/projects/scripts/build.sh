#!/bin/bash
set -Eeuo pipefail

COZE_WORKSPACE_PATH="${COZE_WORKSPACE_PATH:-$(pwd)}"

cd "${COZE_WORKSPACE_PATH}"

# 加载 Coze 环境变量
load_coze_env() {
    echo "Loading environment variables from Coze..."
    
    if command -v python3 &> /dev/null; then
        python3 "${COZE_WORKSPACE_PATH}/scripts/get-env.py" > "${COZE_WORKSPACE_PATH}/.env" 2>/dev/null || true
        
        if [ -f "${COZE_WORKSPACE_PATH}/.env" ]; then
            echo "Environment variables loaded successfully"
            set -a
            source "${COZE_WORKSPACE_PATH}/.env"
            set +a
        fi
    fi
}

# 在构建前加载环境变量
load_coze_env

echo "Installing dependencies..."
pnpm install --prefer-frozen-lockfile --prefer-offline --loglevel debug --reporter=append-only

echo "Building the project..."
npx next build

echo "Build completed successfully!"

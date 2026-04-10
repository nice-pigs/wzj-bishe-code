#!/usr/bin/env python3
"""
获取 Coze 项目环境变量的 Python 脚本
用于在 Node.js 启动时注入环境变量
"""
import sys
import os

try:
    from coze_workload_identity import Client
    client = Client()
    env_vars = client.get_project_env_vars()
    client.close()

    for env_var in env_vars:
        # 输出格式: KEY=VALUE
        # 使用特殊字符分隔，避免值中包含换行符的问题
        value = env_var.value.replace('\n', '\\n')
        print(f"{env_var.key}={value}")

except Exception as e:
    print(f"# Error: {e}", file=sys.stderr)
    sys.exit(1)

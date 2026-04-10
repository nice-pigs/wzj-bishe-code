#!/bin/bash
# GitHub 推送脚本
# 请在 GitHub 创建仓库后运行

# ===== 配置区（请修改） =====
GITHUB_USERNAME="你的GitHub用户名"
REPO_NAME="seat-monitoring-system"

# ===== 执行推送 =====
echo "正在推送代码到 GitHub..."

# 添加远程仓库
git remote add origin https://github.com/${GITHUB_USERNAME}/${REPO_NAME}.git

# 推送代码
git push -u origin main

echo "推送完成！"
echo "仓库地址: https://github.com/${GITHUB_USERNAME}/${REPO_NAME}"

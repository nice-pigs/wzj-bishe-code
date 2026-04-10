#!/bin/bash
# 服务器部署脚本
# 使用方法：bash deploy.sh

echo "=========================================="
echo "     智能座位监测系统 - 服务器部署脚本"
echo "=========================================="

# 配置变量
PROJECT_DIR="/home/seat-monitor"
DOMAIN="your-domain.com"  # 修改为你的域名

echo ""
echo "[1/6] 检查环境..."
node -v || { echo "请先安装 Node.js"; exit 1; }
pnpm -v || { echo "请先安装 pnpm"; exit 1; }

echo ""
echo "[2/6] 进入项目目录..."
cd $PROJECT_DIR || { echo "项目目录不存在"; exit 1; }

echo ""
echo "[3/6] 安装依赖..."
pnpm install

echo ""
echo "[4/6] 构建项目..."
pnpm build

echo ""
echo "[5/6] 使用 PM2 启动服务..."
pm2 delete seat-monitor 2>/dev/null
pm2 start pnpm --name "seat-monitor" -- start

echo ""
echo "[6/6] 配置 Nginx..."
cat > /etc/nginx/conf.d/seat-monitor.conf << EOF
server {
    listen 80;
    server_name $DOMAIN;
    
    location / {
        proxy_pass http://localhost:5000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade \$http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host \$host;
        proxy_cache_bypass \$http_upgrade;
        proxy_set_header X-Real-IP \$remote_addr;
        proxy_set_header X-Forwarded-For \$proxy_add_x_forwarded_for;
    }
}
EOF

# 测试 Nginx 配置
nginx -t && systemctl reload nginx

echo ""
echo "=========================================="
echo "✅ 部署完成！"
echo ""
echo "访问地址: http://$DOMAIN"
echo "API地址: http://$DOMAIN/api"
echo ""
echo "常用命令："
echo "  查看日志: pm2 logs seat-monitor"
echo "  重启服务: pm2 restart seat-monitor"
echo "  停止服务: pm2 stop seat-monitor"
echo "=========================================="

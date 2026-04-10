# 部署指南

## 方案一：Vercel 部署（推荐，免费）

### 步骤 1：注册 Vercel 账号
1. 访问 https://vercel.com
2. 使用 GitHub 账号登录（如果没有，需要先注册 GitHub）

### 步骤 2：准备 GitHub 仓库
```bash
# 在项目目录下
git add .
git commit -m "feat: 完成座位监测系统"
git push origin main
```

### 步骤 3：导入项目到 Vercel
1. 登录 Vercel，点击 "Add New" → "Project"
2. 选择你的 GitHub 仓库
3. 点击 "Import"
4. 保持默认配置，点击 "Deploy"
5. 等待 1-2 分钟部署完成

### 步骤 4：获取公网地址
部署成功后，Vercel 会给你一个地址，例如：
```
https://seat-monitoring-system.vercel.app
```

### 步骤 5：配置环境变量
在 Vercel 项目设置中添加环境变量：
- `NEXT_PUBLIC_SUPABASE_URL`
- `NEXT_PUBLIC_SUPABASE_ANON_KEY`

---

## 方案二：Railway 部署（免费额度）

1. 访问 https://railway.app
2. 使用 GitHub 登录
3. 新建项目，选择 GitHub 仓库
4. 自动部署

---

## 方案三：自己的服务器

如果你有阿里云、腾讯云等服务器：

```bash
# 1. 上传代码到服务器
scp -r ./* user@your-server:/home/seat-monitor/

# 2. 在服务器上安装 Node.js 和 pnpm
curl -fsSL https://get.pnpm.io/install.sh | sh -

# 3. 安装依赖并构建
cd /home/seat-monitor
pnpm install
pnpm build

# 4. 使用 PM2 守护进程
pnpm add -g pm2
pm2 start pnpm --name "seat-monitor" -- start
```

---

## 单片机配置更新

部署完成后，修改 firmware/main.c：

```c
// 方案一：Vercel 部署后
#define SERVER_IP       "seat-monitoring-system.vercel.app"  // Vercel 域名
#define SERVER_PORT     "443"  // HTTPS 端口
// 注意：需要修改 WiFi_SendHTTPRequest 函数使用 HTTPS

// 方案二：自己的服务器
#define SERVER_IP       "你的服务器公网IP"
#define SERVER_PORT     "5000"
```

---

## 测试部署是否成功

部署后，用浏览器访问你的公网地址，应该能看到座位监测界面。

然后可以用 curl 测试 API：
```bash
# 测试设备注册
curl -X POST https://你的域名/api/devices/register \
  -H "Content-Type: application/json" \
  -d '{"deviceKey":"TEST-001","deviceName":"测试设备"}'

# 测试状态上报
curl -X POST https://你的域名/api/seats/status \
  -H "Content-Type: application/json" \
  -d '{"deviceKey":"TEST-001","isOccupied":true}'

# 测试座位查询
curl https://你的域名/api/seats
```

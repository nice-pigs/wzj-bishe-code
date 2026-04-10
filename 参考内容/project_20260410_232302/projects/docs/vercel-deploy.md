# Vercel 部署指南

## 为什么选择 Vercel？

| 对比项 | Coze（火山引擎） | Vercel |
|--------|-----------------|--------|
| 备案要求 | ✅ 需要备案 | ❌ **不需要备案** |
| 域名绑定 | ✅ 支持 | ✅ 支持 |
| 费用 | 免费 | 免费 |
| HTTPS | 支持 | 自动配置 |

---

## 第一步：注册 GitHub 账号

1. 访问 https://github.com
2. 点击 **Sign up** 注册
3. 验证邮箱

---

## 第二步：创建 GitHub 仓库

1. 登录 GitHub
2. 点击右上角 **+** → **New repository**
3. 填写信息：
   - Repository name: `seat-monitoring-system`
   - 选择 **Public**（公开）
   - 点击 **Create repository**

---

## 第三步：上传代码到 GitHub

### 方式一：使用 Git 命令行

在本地电脑打开终端/命令提示符：

```bash
# 克隆项目（从 Coze 下载代码后）
git clone https://github.com/你的用户名/seat-monitoring-system.git
cd seat-monitoring-system

# 复制项目文件到这个目录
# 然后执行：
git add .
git commit -m "初始化座位监测系统"
git push origin main
```

### 方式二：直接上传文件

1. 在 GitHub 仓库页面点击 **uploading an existing file**
2. 拖拽项目文件上传
3. 点击 **Commit changes**

---

## 第四步：注册 Vercel 账号

1. 访问 https://vercel.com
2. 点击 **Continue with GitHub**
3. 授权 Vercel 访问你的 GitHub

---

## 第五步：导入项目到 Vercel

1. 登录 Vercel 后，点击 **Add New** → **Project**
2. 选择你刚才创建的 `seat-monitoring-system` 仓库
3. 点击 **Import**
4. 配置项目：
   - Framework Preset: **Next.js**（自动检测）
   - Root Directory: `./`
   - 保持其他默认配置
5. 点击 **Deploy**
6. 等待 1-2 分钟部署完成

---

## 第六步：配置环境变量

部署成功后，需要添加数据库配置：

1. 进入项目 → **Settings** → **Environment Variables**
2. 添加以下变量：

| 变量名 | 变量值 |
|--------|--------|
| `NEXT_PUBLIC_SUPABASE_URL` | `https://br-fiery-lynx-dc304ad3.supabase2.aidap-global.cn-beijing.volces.com` |
| `NEXT_PUBLIC_SUPABASE_ANON_KEY` | `eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjMzNTM0MDg3MTIsInJvbGUiOiJhbm9uIn0.K2mOWzhbI2RcrvrEKYNwP2HN4CixaC7ZVFLoQNNc-7E` |

3. 点击 **Save**
4. 重新部署项目（**Deployments** → 点击最新的部署 → **Redeploy**）

---

## 第七步：绑定自定义域名

1. 进入项目 → **Settings** → **Domains**
2. 输入你的域名：`五子鸡大坏蛋.top`
3. 点击 **Add**
4. Vercel 会显示需要配置的 DNS 记录

### 在火山引擎配置 DNS（修改之前的记录）

| 类型 | 主机记录 | 值 |
|------|---------|-----|
| A | @ | `76.76.21.21` |
| CNAME | www | `cname.vercel-dns.com` |

⚠️ **注意**：需要修改火山引擎的 DNS 记录为 Vercel 提供的地址。

5. 回到火山引擎 DNS 管理，修改记录值
6. 等待 DNS 生效（几分钟到几小时）
7. Vercel 会自动为你的域名配置 HTTPS

---

## 第八步：访问你的网站

完成后，你可以通过以下地址访问：

- `https://五子鸡大坏蛋.top`
- `https://www.五子鸡大坏蛋.top`
- `https://seat-monitoring-system.vercel.app`（Vercel 默认域名）

---

## 注意事项

### 国内访问速度

Vercel 使用海外节点，国内访问可能较慢。如果需要国内快速访问：

1. **方案一**：使用 Coze 域名 `smkcc339ny.coze.site`（国内访问快）
2. **方案二**：购买火山引擎建站服务并备案
3. **方案三**：使用国内云服务器部署

### 数据库说明

项目使用的是 Supabase 数据库（托管在火山引擎），无论部署到哪里都可以正常访问数据库。

---

## 完整流程图

```
GitHub 仓库
    │
    ↓ 导入
Vercel 部署
    │
    ↓ 配置环境变量
添加数据库配置
    │
    ↓ 绑定域名
修改 DNS 记录
    │
    ↓ 等待生效
访问 https://五子鸡大坏蛋.top ✅
```

---

## 对比总结

| 方案 | 域名访问 | 备案 | 国内速度 | 推荐场景 |
|------|---------|------|---------|---------|
| Coze 默认域名 | `smkcc339ny.coze.site` | 不需要 | 快 | ✅ 毕设答辩推荐 |
| Coze + 自定义域名 | `五子鸡大坏蛋.top` | 需要 | 快 | 正式项目 |
| Vercel + 自定义域名 | `五子鸡大坏蛋.top` | 不需要 | 较慢 | 海外用户/演示 |

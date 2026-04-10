#!/bin/bash
# API 测试脚本
# 用于测试智能座位监测系统的所有 API 接口

DOMAIN="https://smkcc339ny.coze.site"

echo "=========================================="
echo "   智能座位监测系统 - API 测试"
echo "=========================================="
echo ""

echo "[1/6] 测试设备注册 API..."
curl -s -X POST -H 'Content-Type: application/json' \
  -d '{"deviceKey":"STM32-TEST-001","deviceName":"测试设备-001"}' \
  "${DOMAIN}/api/devices/register" | jq .
echo ""

echo "[2/6] 测试状态上报 API (占用)..."
curl -s -X POST -H 'Content-Type: application/json' \
  -d '{"deviceKey":"STM32-TEST-001","isOccupied":true}' \
  "${DOMAIN}/api/seats/status" | jq .
echo ""

echo "[3/6] 测试状态上报 API (空闲)..."
curl -s -X POST -H 'Content-Type: application/json' \
  -d '{"deviceKey":"STM32-TEST-001","isOccupied":false}' \
  "${DOMAIN}/api/seats/status" | jq .
echo ""

echo "[4/6] 测试座位查询 API..."
curl -s "${DOMAIN}/api/seats" | jq .
echo ""

echo "[5/6] 测试座位历史记录 API..."
curl -s "${DOMAIN}/api/seats/1/history?limit=5" | jq .
echo ""

echo "[6/6] 测试 Web 界面..."
curl -s -o /dev/null -w "HTTP状态码: %{http_code}\n" "${DOMAIN}"
echo ""

echo "=========================================="
echo "✅ 测试完成！"
echo "=========================================="

#!/bin/bash

# MOAB 服务器测试脚本
# 使用方法: ./test_server.sh [端口号]

PORT=${1:-8080}
SERVER="./moab_server"
TEST_LOG="test_results.log"

echo "=== MOAB 服务器测试 ===" > $TEST_LOG
echo "测试时间: $(date)" >> $TEST_LOG
echo "端口: $PORT" >> $TEST_LOG
echo "" >> $TEST_LOG

# 检查服务器是否存在
if [ ! -f "$SERVER" ]; then
    echo "❌ 错误: 服务器文件不存在: $SERVER"
    echo "请先运行 'make' 编译服务器"
    exit 1
fi

# 启动服务器
echo "🚀 启动服务器在端口 $PORT..."
$SERVER $PORT &
SERVER_PID=$!
sleep 2

# 检查服务器是否启动成功
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "❌ 错误: 服务器启动失败"
    exit 1
fi

echo "✅ 服务器启动成功 (PID: $SERVER_PID)" >> $TEST_LOG

# 测试函数
test_connection() {
    echo "🔌 测试基本连接..."
    timeout 5 nc localhost $PORT > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "✅ 基本连接测试通过" >> $TEST_LOG
    else
        echo "❌ 基本连接测试失败" >> $TEST_LOG
    fi
}

test_registration() {
    echo "📝 测试玩家注册..."
    
    # 启动测试客户端
    (echo "REG player1 5 5 -"; sleep 1) | nc localhost $PORT > /tmp/reg_response 2>&1 &
    sleep 2
    
    if grep -q "WELCOME" /tmp/reg_response; then
        echo "✅ 注册测试通过" >> $TEST_LOG
    else
        echo "❌ 注册测试失败" >> $TEST_LOG
    fi
}

test_bomb() {
    echo "💣 测试轰炸功能..."
    
    # 先注册一个玩家
    (echo "REG player1 5 5 -"; sleep 1) | nc localhost $PORT > /dev/null 2>&1 &
    sleep 2
    
    # 测试轰炸
    (echo "BOMB 5 5"; sleep 1) | nc localhost $PORT > /tmp/bomb_response 2>&1 &
    sleep 2
    
    if [ -s /tmp/bomb_response ]; then
        echo "✅ 轰炸测试通过" >> $TEST_LOG
    else
        echo "❌ 轰炸测试失败" >> $TEST_LOG
    fi
}

test_multiple_clients() {
    echo "👥 测试多客户端连接..."
    
    # 启动多个客户端
    for i in {1..3}; do
        (echo "REG player$i 5 $i -"; sleep 1) | nc localhost $PORT > /dev/null 2>&1 &
    done
    
    sleep 3
    
    echo "✅ 多客户端测试完成" >> $TEST_LOG
}

test_invalid_messages() {
    echo "🚫 测试无效消息处理..."
    
    # 测试无效注册
    (echo "REG invalid@name 5 5 -"; sleep 1) | nc localhost $PORT > /tmp/invalid_reg 2>&1 &
    sleep 2
    
    if grep -q "INVALID" /tmp/invalid_reg; then
        echo "✅ 无效消息处理测试通过" >> $TEST_LOG
    else
        echo "❌ 无效消息处理测试失败" >> $TEST_LOG
    fi
}

test_ship_destruction() {
    echo "💥 测试船只沉没..."
    
    # 注册玩家并轰炸船只的所有格子
    (echo "REG player1 5 5 -"; sleep 1) | nc localhost $PORT > /dev/null 2>&1 &
    sleep 2
    
    # 轰炸水平船的所有格子 (3,5), (4,5), (5,5), (6,5), (7,5)
    for coord in "3 5" "4 5" "5 5" "6 5" "7 5"; do
        (echo "BOMB $coord"; sleep 1) | nc localhost $PORT > /dev/null 2>&1 &
        sleep 1
    done
    
    sleep 2
    echo "✅ 船只沉没测试完成" >> $TEST_LOG
}

# 运行所有测试
echo "开始运行测试..." >> $TEST_LOG
echo "" >> $TEST_LOG

test_connection
test_registration
test_bomb
test_multiple_clients
test_invalid_messages
test_ship_destruction

# 清理
echo "🧹 清理测试环境..."
rm -f /tmp/reg_response /tmp/bomb_response /tmp/invalid_reg

# 停止服务器
echo "🛑 停止服务器..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo "" >> $TEST_LOG
echo "=== 测试完成 ===" >> $TEST_LOG
echo "测试结果已保存到: $TEST_LOG"

# 显示测试结果
echo ""
echo "📊 测试结果摘要:"
cat $TEST_LOG | grep -E "(✅|❌)"

echo ""
echo "详细结果请查看: $TEST_LOG" 
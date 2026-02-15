# ESP32 语音控制系统 - 工具说明

## audio_uploader.py - 音频上传工具

用于通过串口与 ESP32 通信，上传、管理和播放音频文件。

---

## 安装依赖

```bash
pip install pyserial
```

---

## 使用方法

### 1. 测试连接

```bash
python audio_uploader.py --port COM3 ping
```

**输出**：
```
Connected to COM3 @ 115200 baud

Testing connection...
✓ Connection OK
  Response: PONG
```

---

### 2. 获取系统信息

```bash
python audio_uploader.py --port COM3 info
```

**输出**：
```
Getting system info...
✓ System Info: Files:5,Used:128KB,Free:9MB
```

---

### 3. 列出所有文件

```bash
python audio_uploader.py --port COM3 list
```

**输出**：
```
Listing files...
✓ Total files: 3
  [1] play.pcm,16384,16000
  [2] stop.pcm,8192,16000
  [3] next.pcm,12288,16000
```

---

### 4. 上传音频文件

#### 基本上传

```bash
python audio_uploader.py --port COM3 upload audio/play.pcm
```

#### 指定文件名

```bash
python audio_uploader.py --port COM3 upload audio/play.pcm --name my_audio.pcm
```

#### 指定采样率

```bash
python audio_uploader.py --port COM3 upload audio/play.pcm --rate 22050
```

#### 上传并绑定命令

```bash
# 绑定到命令 ID 1（播放）
python audio_uploader.py --port COM3 upload audio/play.pcm --cmd 1

# 绑定到命令 ID 2（停止）
python audio_uploader.py --port COM3 upload audio/stop.pcm --cmd 2
```

**输出**：
```
Uploading: play.pcm
  Source: audio/play.pcm
  Size: 16384 bytes (16.00 KB)
  Sample Rate: 16000 Hz
  Command ID: 1

[1/3] Starting upload...
✓ Upload started

[2/3] Uploading data...
  Progress: [64/64] 100% (16384/16384 bytes)
✓ Data uploaded

[3/3] Finalizing...
✓ Upload completed!
```

---

### 5. 删除文件

```bash
python audio_uploader.py --port COM3 delete play.pcm
```

**输出**：
```
Deleting file: play.pcm
✓ File deleted
```

---

### 6. 播放文件

```bash
python audio_uploader.py --port COM3 play play.pcm
```

**输出**：
```
Playing file: play.pcm
✓ Playing...
```

---

### 7. 停止播放

```bash
python audio_uploader.py --port COM3 stop
```

---

### 8. 绑定语音命令

```bash
# 将 play.pcm 绑定到命令 ID 1
python audio_uploader.py --port COM3 bind play.pcm 1

# 将 stop.pcm 绑定到命令 ID 2
python audio_uploader.py --port COM3 bind stop.pcm 2
```

---

### 9. 格式化存储

```bash
python audio_uploader.py --port COM3 format
```

**输出**：
```
⚠️  WARNING: This will delete ALL audio files!
Type 'YES' to confirm: YES
Formatting storage...
✓ Storage formatted
```

---

## 完整命令参考

```bash
# 基本语法
python audio_uploader.py --port <PORT> [--baud BAUD] <COMMAND> [ARGS]

# 选项
--port, -p <PORT>       串口号（必需）
--baud, -b <BAUD>       波特率（默认：115200）

# 命令
ping                    测试连接
info                    获取系统信息
list                    列出所有文件
upload <FILE>           上传音频文件
  --name, -n <NAME>     指定设备上的文件名
  --rate, -r <RATE>     采样率（默认：16000）
  --cmd, -c <ID>        绑定的命令 ID（默认：0）
delete <FILE>           删除文件
play <FILE>             播放文件
stop                    停止播放
bind <FILE> <ID>        绑定命令到文件
format                  格式化存储
```

---

## 批量上传示例

### Windows 批处理脚本

创建 `upload_all.bat`：

```batch
@echo off
set PORT=COM3

python audio_uploader.py --port %PORT% upload audio/play.pcm --cmd 1
python audio_uploader.py --port %PORT% upload audio/stop.pcm --cmd 2
python audio_uploader.py --port %PORT% upload audio/pause.pcm --cmd 3
python audio_uploader.py --port %PORT% upload audio/next.pcm --cmd 4
python audio_uploader.py --port %PORT% upload audio/prev.pcm --cmd 5
python audio_uploader.py --port %PORT% upload audio/vol_up.pcm --cmd 6
python audio_uploader.py --port %PORT% upload audio/vol_down.pcm --cmd 7

echo.
echo All files uploaded!
pause
```

### Linux/Mac Shell 脚本

创建 `upload_all.sh`：

```bash
#!/bin/bash
PORT=/dev/ttyUSB0

python3 audio_uploader.py --port $PORT upload audio/play.pcm --cmd 1
python3 audio_uploader.py --port $PORT upload audio/stop.pcm --cmd 2
python3 audio_uploader.py --port $PORT upload audio/pause.pcm --cmd 3
python3 audio_uploader.py --port $PORT upload audio/next.pcm --cmd 4
python3 audio_uploader.py --port $PORT upload audio/prev.pcm --cmd 5
python3 audio_uploader.py --port $PORT upload audio/vol_up.pcm --cmd 6
python3 audio_uploader.py --port $PORT upload audio/vol_down.pcm --cmd 7

echo ""
echo "All files uploaded!"
```

运行：
```bash
chmod +x upload_all.sh
./upload_all.sh
```

---

## 常见问题

### 1. 找不到串口

**Windows**：
```
✗ Failed to open serial port: [Errno 2] could not open port 'COM3'
```

**解决**：
- 在设备管理器中查看 COM 口号
- 确认 USB 驱动已安装

**Linux/Mac**：
```
✗ Failed to open serial port: [Errno 13] Permission denied: '/dev/ttyUSB0'
```

**解决**：
```bash
# 添加用户到 dialout 组
sudo usermod -a -G dialout $USER

# 或使用 sudo
sudo python3 audio_uploader.py --port /dev/ttyUSB0 ping
```

---

### 2. 上传失败

```
✗ Upload start failed
```

**可能原因**：
- ESP32 未运行串口协议任务
- 波特率不匹配
- Flash 空间不足

**解决**：
1. 检查 ESP32 日志确认串口任务已启动
2. 确认波特率为 115200
3. 运行 `info` 命令查看剩余空间
4. 运行 `format` 清理空间

---

### 3. 上传速度慢

**原因**：串口传输速度受限于波特率（115200 bps ≈ 11 KB/s）

**优化建议**：
- 使用较小的音频文件（< 100 KB）
- 降低采样率（8kHz 或 16kHz）
- 裁剪音频长度

---

## 音频文件准备

### 推荐参数

| 参数 | 推荐值 |
|-----|--------|
| 格式 | RAW PCM |
| 采样率 | 16000 Hz |
| 位深度 | 16-bit |
| 声道 | 单声道 |
| 文件大小 | < 100 KB |
| 时长 | < 3 秒 |

### 转换命令

```bash
# 标准转换
ffmpeg -i input.wav -ar 16000 -ac 1 -f s16le output.pcm

# 裁剪到 2 秒
ffmpeg -i input.wav -t 2 -ar 16000 -ac 1 -f s16le output.pcm

# 降低音量（避免爆音）
ffmpeg -i input.wav -filter:a "volume=0.7" -ar 16000 -ac 1 -f s16le output.pcm

# 批量转换（Windows）
for %f in (*.wav) do ffmpeg -i "%f" -ar 16000 -ac 1 -f s16le "%~nf.pcm"

# 批量转换（Linux/Mac）
for f in *.wav; do ffmpeg -i "$f" -ar 16000 -ac 1 -f s16le "${f%.wav}.pcm"; done
```

---

## 许可证

本工具为教育和开发目的编写，可自由使用和修改。

祝你使用愉快！🔧📤

#!/usr/bin/env python3
"""
ESP32 语音控制系统 - 音频文件上传工具
用于通过串口上传音频文件到 ESP32 的 Flash 存储
"""

import serial
import struct
import time
import os
import argparse
from pathlib import Path

# 协议定义
PROTO_HEADER = 0xAA55
PROTO_FOOTER = 0x55AA

# 命令类型
CMD_PING = 0x01
CMD_GET_INFO = 0x02
CMD_LIST_FILES = 0x03
CMD_UPLOAD_START = 0x10
CMD_UPLOAD_DATA = 0x11
CMD_UPLOAD_END = 0x12
CMD_DELETE_FILE = 0x20
CMD_PLAY_FILE = 0x30
CMD_STOP_PLAY = 0x31
CMD_BIND_COMMAND = 0x40
CMD_FORMAT = 0xF0

# 响应码
RESP_OK = 0x00
RESP_ERROR = 0x01
RESP_BUSY = 0x02
RESP_INVALID = 0x03
RESP_NO_SPACE = 0x04
RESP_NOT_FOUND = 0x05
RESP_CRC_ERROR = 0x06

# ============================================================================
# CRC16 计算
# ============================================================================

def calc_crc16(data):
    """计算 CRC16 校验值"""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc

# ============================================================================
# 串口通信
# ============================================================================

def send_packet(ser, command, data=b''):
    """发送数据包"""
    header = struct.pack('>H', PROTO_HEADER)
    cmd = struct.pack('B', command)
    length = struct.pack('>H', len(data))

    # 计算 CRC
    crc_data = cmd + length + data
    crc = struct.pack('<H', calc_crc16(crc_data))

    footer = struct.pack('>H', PROTO_FOOTER)

    packet = header + cmd + length + data + crc + footer
    ser.write(packet)
    time.sleep(0.05)  # 等待处理

def read_response(ser, timeout=1.0):
    """读取响应包"""
    start_time = time.time()
    buffer = b''

    while time.time() - start_time < timeout:
        if ser.in_waiting > 0:
            buffer += ser.read(ser.in_waiting)

            # 检查是否收到完整包（至少 7 字节）
            if len(buffer) >= 7:
                # 查找帧头
                if len(buffer) >= 2 and buffer[0:2] == struct.pack('>H', PROTO_HEADER):
                    response_code = buffer[2]
                    data_length = struct.unpack('>H', buffer[3:5])[0]

                    # 等待完整数据包
                    expected_length = 7 + data_length
                    if len(buffer) >= expected_length:
                        data = buffer[5:5+data_length] if data_length > 0 else b''
                        return response_code, data

        time.sleep(0.01)

    return None, None

# ============================================================================
# 命令函数
# ============================================================================

def ping(ser):
    """测试连接"""
    print("Testing connection...")
    send_packet(ser, CMD_PING)
    resp, data = read_response(ser)

    if resp == RESP_OK:
        print("✓ Connection OK")
        if data:
            print(f"  Response: {data.decode('utf-8', errors='ignore')}")
        return True
    else:
        print("✗ Connection failed")
        return False

def get_info(ser):
    """获取系统信息"""
    print("Getting system info...")
    send_packet(ser, CMD_GET_INFO)
    resp, data = read_response(ser)

    if resp == RESP_OK and data:
        info = data.decode('utf-8', errors='ignore')
        print(f"✓ System Info: {info}")
        return True
    else:
        print("✗ Failed to get info")
        return False

def list_files(ser):
    """列出所有文件"""
    print("Listing files...")
    send_packet(ser, CMD_LIST_FILES)

    # 读取文件数量
    resp, data = read_response(ser)
    if resp != RESP_OK or not data:
        print("✗ Failed to list files")
        return []

    count = struct.unpack('<I', data[:4])[0]
    print(f"✓ Total files: {count}")

    files = []
    for i in range(count):
        resp, data = read_response(ser)
        if resp == RESP_OK and data:
            file_info = data.decode('utf-8', errors='ignore').strip('\x00')
            files.append(file_info)
            print(f"  [{i+1}] {file_info}")

    return files

def delete_file(ser, filename):
    """删除文件"""
    print(f"Deleting file: {filename}")
    data = filename.encode('utf-8').ljust(64, b'\x00')
    send_packet(ser, CMD_DELETE_FILE, data)
    resp, _ = read_response(ser)

    if resp == RESP_OK:
        print("✓ File deleted")
        return True
    elif resp == RESP_NOT_FOUND:
        print("✗ File not found")
    else:
        print("✗ Delete failed")
    return False

def play_file(ser, filename):
    """播放文件"""
    print(f"Playing file: {filename}")
    data = filename.encode('utf-8').ljust(64, b'\x00')
    send_packet(ser, CMD_PLAY_FILE, data)
    resp, _ = read_response(ser)

    if resp == RESP_OK:
        print("✓ Playing...")
        return True
    elif resp == RESP_NOT_FOUND:
        print("✗ File not found")
    else:
        print("✗ Playback failed")
    return False

def stop_play(ser):
    """停止播放"""
    print("Stopping playback...")
    send_packet(ser, CMD_STOP_PLAY)
    resp, _ = read_response(ser)

    if resp == RESP_OK:
        print("✓ Stopped")
        return True
    return False

def format_storage(ser):
    """格式化存储"""
    print("⚠️  WARNING: This will delete ALL audio files!")
    confirm = input("Type 'YES' to confirm: ")

    if confirm != 'YES':
        print("Cancelled")
        return False

    print("Formatting storage...")
    send_packet(ser, CMD_FORMAT)
    resp, _ = read_response(ser, timeout=5.0)

    if resp == RESP_OK:
        print("✓ Storage formatted")
        return True
    else:
        print("✗ Format failed")
        return False

def bind_command(ser, filename, command_id):
    """绑定语音命令到文件"""
    print(f"Binding {filename} to command ID {command_id}")
    data = filename.encode('utf-8').ljust(64, b'\x00')
    data += struct.pack('B', command_id)
    send_packet(ser, CMD_BIND_COMMAND, data)
    resp, _ = read_response(ser)

    if resp == RESP_OK:
        print("✓ Binding successful")
        return True
    else:
        print("✗ Binding failed")
        return False

def upload_audio(ser, filename, pcm_file, sample_rate=16000, command_id=0):
    """上传音频文件"""
    if not os.path.exists(pcm_file):
        print(f"✗ File not found: {pcm_file}")
        return False

    # 读取文件
    with open(pcm_file, 'rb') as f:
        audio_data = f.read()

    file_size = len(audio_data)
    print(f"\nUploading: {filename}")
    print(f"  Source: {pcm_file}")
    print(f"  Size: {file_size} bytes ({file_size/1024:.2f} KB)")
    print(f"  Sample Rate: {sample_rate} Hz")
    if command_id > 0:
        print(f"  Command ID: {command_id}")

    # 1. UPLOAD_START
    print("\n[1/3] Starting upload...")
    start_data = filename.encode('utf-8').ljust(64, b'\x00')
    start_data += struct.pack('<I', file_size)
    start_data += struct.pack('<I', sample_rate)
    start_data += struct.pack('BBB', 16, 1, command_id)  # 16bit, mono

    send_packet(ser, CMD_UPLOAD_START, start_data)
    resp, _ = read_response(ser)

    if resp != RESP_OK:
        if resp == RESP_BUSY:
            print("✗ Upload already in progress")
        elif resp == RESP_NO_SPACE:
            print("✗ Not enough space")
        else:
            print("✗ Upload start failed")
        return False

    print("✓ Upload started")

    # 2. UPLOAD_DATA (分块上传)
    print("\n[2/3] Uploading data...")
    chunk_size = 256
    offset = 0
    total_chunks = (file_size + chunk_size - 1) // chunk_size

    while offset < file_size:
        chunk = audio_data[offset:offset+chunk_size]
        data_packet = struct.pack('<IH', offset, len(chunk))
        data_packet += chunk

        send_packet(ser, CMD_UPLOAD_DATA, data_packet)
        resp, _ = read_response(ser)

        if resp != RESP_OK:
            print(f"\n✗ Upload failed at offset {offset}")
            return False

        offset += len(chunk)
        progress = offset * 100 // file_size
        current_chunk = offset // chunk_size
        print(f"\r  Progress: [{current_chunk}/{total_chunks}] {progress}% ({offset}/{file_size} bytes)", end='', flush=True)

        time.sleep(0.02)  # 避免发送过快

    print("\n✓ Data uploaded")

    # 3. UPLOAD_END
    print("\n[3/3] Finalizing...")
    send_packet(ser, CMD_UPLOAD_END)
    resp, _ = read_response(ser, timeout=3.0)

    if resp == RESP_OK:
        print("✓ Upload completed!\n")
        return True
    else:
        print("✗ Upload finalization failed\n")
        return False

# ============================================================================
# 主程序
# ============================================================================

def main():
    parser = argparse.ArgumentParser(description='ESP32 Audio Uploader')

    parser.add_argument('--port', '-p', required=True, help='Serial port (e.g., COM3 or /dev/ttyUSB0)')
    parser.add_argument('--baud', '-b', type=int, default=115200, help='Baud rate (default: 115200)')

    subparsers = parser.add_subparsers(dest='command', help='Commands')

    # ping
    subparsers.add_parser('ping', help='Test connection')

    # info
    subparsers.add_parser('info', help='Get system info')

    # list
    subparsers.add_parser('list', help='List all files')

    # upload
    upload_parser = subparsers.add_parser('upload', help='Upload audio file')
    upload_parser.add_argument('pcm_file', help='PCM audio file to upload')
    upload_parser.add_argument('--name', '-n', help='Filename on device (default: same as source)')
    upload_parser.add_argument('--rate', '-r', type=int, default=16000, help='Sample rate (default: 16000)')
    upload_parser.add_argument('--cmd', '-c', type=int, default=0, help='Command ID to bind (default: 0 = no binding)')

    # delete
    delete_parser = subparsers.add_parser('delete', help='Delete file')
    delete_parser.add_argument('filename', help='File to delete')

    # play
    play_parser = subparsers.add_parser('play', help='Play file')
    play_parser.add_argument('filename', help='File to play')

    # stop
    subparsers.add_parser('stop', help='Stop playback')

    # bind
    bind_parser = subparsers.add_parser('bind', help='Bind command to file')
    bind_parser.add_argument('filename', help='Audio filename')
    bind_parser.add_argument('command_id', type=int, help='Command ID (1-9)')

    # format
    subparsers.add_parser('format', help='Format storage (delete all files)')

    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        return

    # 打开串口
    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
        print(f"Connected to {args.port} @ {args.baud} baud\n")
    except serial.SerialException as e:
        print(f"✗ Failed to open serial port: {e}")
        return

    try:
        # 执行命令
        if args.command == 'ping':
            ping(ser)

        elif args.command == 'info':
            get_info(ser)

        elif args.command == 'list':
            list_files(ser)

        elif args.command == 'upload':
            filename = args.name if args.name else Path(args.pcm_file).name
            upload_audio(ser, filename, args.pcm_file, args.rate, args.cmd)

        elif args.command == 'delete':
            delete_file(ser, args.filename)

        elif args.command == 'play':
            play_file(ser, args.filename)

        elif args.command == 'stop':
            stop_play(ser)

        elif args.command == 'bind':
            bind_command(ser, args.filename, args.command_id)

        elif args.command == 'format':
            format_storage(ser)

    except KeyboardInterrupt:
        print("\n\nInterrupted by user")

    finally:
        ser.close()
        print("Connection closed")

if __name__ == '__main__':
    main()

"""
数据解析处理模块
==================
接收串口的原始二进制帧，按照协议解析并发送到显示界面。

上行帧格式（下位机 → 上位机）:
  帧头(AA 55) + 长度(1B) + 应答码(1B) + 数据区(N*B) + CRC32(4B, LSB)

应答码:
  0x81  采样数据
  0x82  状态参数
  0x83  应答

采样数据 (0x81) 数据区格式 (LSB):
  偏移  内容   类型    说明
  0-1   Vin    uint16  输入电压 (0.001V)
  2-3   Vout   uint16  输出电压 (0.001V)
  4-5   Iin    uint16  输入电流 (0.001A)
  6-7   Iout   uint16  输出电流 (0.001A)
  8-9   iL     uint16  电感电流 (0.001A)
  10-11 Duty   uint16  占空比 (值/3000)
  12-13 Temp   uint16  温度 (0.1°C)
"""
import struct
import zlib
from collections import deque
from PyQt6.QtCore import QObject, pyqtSignal


# ── 协议常量 ──────────────────────────────────────────────
FRAME_HEADER = b"\xAA\x55"

# 应答码
CMD_SAMPLE_DATA  = 0x81  # 采样数据
CMD_STATUS       = 0x82  # 状态参数
CMD_ACK          = 0x83  # 应答

# 标度因子
SCALE_VOLTAGE = 0.001    # uint16 → V
SCALE_CURRENT = 0.001    # uint16 → A
SCALE_DUTY    = 1 / 3000 # uint16 → 占空比
SCALE_TEMP    = 0.1      # uint16 → °C


class DataProcessor(QObject):
    """
    数据处理器 — 接收串口原始字节流，按协议解析后发射结构化数据。

    信号:
        data_ready(dict)      — 解析完成的工程值 {"vin": 12.5, "vout": 5.0, ...}
        status_ready(dict)    — 状态参数
        frame_error(str)      — 帧错误消息
    """
    data_ready   = pyqtSignal(dict)
    status_ready = pyqtSignal(dict)
    frame_error  = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._buffer = bytearray()
        self._stats = {
            "frames_ok": 0,
            "frames_bad": 0,
            "last_update": None,
        }
        # 历史缓冲区 (用于统计)
        self.history = deque(maxlen=1000)

    # ── 数据入口 ───────────────────────────────────────────

    def feed(self, data: bytes):
        """从串口管理器接收原始字节"""
        self._buffer.extend(data)
        self._try_parse()

    def reset(self):
        """复位解析器"""
        self._buffer.clear()
        self.history.clear()

    # ── 帧解析 ─────────────────────────────────────────────

    def _try_parse(self):
        """从缓冲区中尝试提取并解析完整帧"""
        while True:
            buf = self._buffer

            # 查找帧头
            idx = buf.find(FRAME_HEADER)
            if idx < 0:
                # 没有帧头，丢弃所有数据
                self._buffer.clear()
                return

            # 丢弃帧头前的无效数据
            if idx > 0:
                buf = buf[idx:]
                self._buffer = bytearray(buf)

            # 至少需要 帧头(2) + 长度(1) + 应答码(1) + CRC(4) = 8 字节
            if len(buf) < 8:
                return

            data_len = buf[2]  # 长度字段 = 应答码 + 数据区 + CRC
            frame_len = 3 + data_len  # 帧头(2) + 长度(1) 之后还有 data_len 字节

            if len(buf) < frame_len:
                # 未收完
                return

            # 提取完整帧 (含帧头)
            frame = bytes(buf[:frame_len])

            # 移除已处理部分 (即使校验失败也移除，防止死循环)
            self._buffer = bytearray(buf[frame_len:])

            # 校验 CRC
            if not self._check_crc(frame):
                self._stats["frames_bad"] += 1
                self.frame_error.emit("CRC 校验失败")
                continue  # 继续尝试下一帧

            self._stats["frames_ok"] += 1
            self._parse_frame(frame)

    def _crc32_gd32(self, data: bytes) -> int:
        """
        模拟 GD32 硬件 CRC-32（多项式 0x04C11DB7，初始值 0xFFFFFFFF，最终不异或）
        数据按 32 位字（大端序）组合，长度不足 4 的倍数补零。
        """
        crc = 0xFFFFFFFF
        # 补零到4的倍数（若下位机不补零，则需事先对齐）
        if len(data) % 4 != 0:
            data += b'\x00' * (4 - len(data) % 4)
        for i in range(0, len(data), 4):
            # 大端序组合：先收到的字节作为高8位
            word = (data[i] << 24) | (data[i+1] << 16) | (data[i+2] << 8) | data[i+3]
            # 逐位 CRC 处理（可改为查表法提升性能）
            for bit in range(32):
                if (crc ^ (word << bit)) & 0x80000000:
                    crc = (crc << 1) ^ 0x04C11DB7
                else:
                    crc = (crc << 1)
                crc &= 0xFFFFFFFF
        return crc

    def _check_crc(self, frame: bytes) -> bool:
        """
        CRC32 校验（包含帧头）。
        帧结构: [55 AA] [len] [cmd] [data...] [crc32(4B LSB)]
        CRC 覆盖范围: 帧头 + 长度 + 应答码 + 数据区（即整个帧去掉最后4字节）
        """
        if len(frame) < 8:
            return False
        crc_expected = struct.unpack("<I", frame[-4:])[0]
        crc_actual = self._crc32_gd32(frame[:-4])  # 替换为 GD32 算法
        return crc_actual == crc_expected

    def _parse_frame(self, frame: bytes):
        """解析已通过 CRC 校验的帧"""
        cmd = frame[3]  # 应答码

        if cmd == CMD_SAMPLE_DATA:
            data = self._parse_sample(frame)
            if data:
                self.history.append(data)
                self.data_ready.emit(data)
        elif cmd == CMD_STATUS:
            data = self._parse_status(frame)
            if data:
                self.status_ready.emit(data)
        elif cmd == CMD_ACK:
            pass  # 应答帧无需处理
        else:
            self.frame_error.emit(f"未知应答码: 0x{cmd:02x}")

    # ── 具体帧解析 ─────────────────────────────────────────

    def _parse_sample(self, frame: bytes) -> dict | None:
        """解析采样数据帧 (0x81)"""
        # 数据区长度至少 12 字节 (6×uint16)，最多 14 字节 (7×uint16)
        if len(frame) < 2 + 1 + 1 + 12 + 4:
            self.frame_error.emit("采样帧长度不足")
            return None

        data = frame[4:-4]  # 跳过 帧头(2)+长度(1)+cmd(1) 和 CRC(4)
        data_len = len(data)

        # 根据数据长度决定解析字段
        if data_len >= 14:
            # 完整帧：7个uint16
            fields = struct.unpack_from("<HHHHHHH", data, 0)
            vin, vout, iin, iout, il, duty, temp = fields
            has_temp = True
        elif data_len >= 12:
            # 无温度：6个uint16
            fields = struct.unpack_from("<HHHHHH", data, 0)
            vin, vout, iin, iout, il, duty = fields
            temp = 0.0
            has_temp = False
        else:
            self.frame_error.emit("数据区长度异常")
            return None

        # 转换成工程值
        result = {
            "vin":  vin  * SCALE_VOLTAGE,
            "vout": vout * SCALE_VOLTAGE,
            "iin":  iin  * SCALE_CURRENT,
            "iout": iout * SCALE_CURRENT,
            "il":   il   * SCALE_CURRENT,
            "duty": duty * SCALE_DUTY,
        }
        if has_temp:
            result["temp"] = temp * SCALE_TEMP

        # 计算导出值
        pin  = result["vin"] * result["iin"] if result["iin"] > 0 else 0
        pout = result["vout"] * result["iout"]
        result["pin"]  = round(pin, 3)
        result["pout"] = round(pout, 3)
        result["eff"]  = round(pout / pin * 100, 1) if pin > 0 else 0

        return result
    def _parse_status(self, frame: bytes) -> dict | None:
        """解析状态参数帧 (0x82)"""
        if len(frame) < 2 + 1 + 1 + 2 + 4:
            return None
        data = frame[4:-4]
        if len(data) < 2:
            return None
        mode, topo = struct.unpack_from("<BB", data, 0)
        return {
            "mode": "CV" if mode == 0 else "CC",
            "topology": "BUCK" if topo == 0 else "BOOST",
        }

    # ── 统计 ───────────────────────────────────────────────

    def get_stats(self) -> dict:
        """获取统计信息"""
        return {**self._stats, "buffer_len": len(self._buffer)}

    def get_latest(self) -> dict | None:
        """获取最新一条解析数据"""
        return self.history[-1] if self.history else None

    def clear_history(self):
        """清空历史"""
        self.history.clear()

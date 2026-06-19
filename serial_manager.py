"""
串口通信管理模块
==================
独立文件，负责串口扫描、连接、数据收发与解析。
通过 SerialManager 提供高级 API，主程序只需连接信号即可使用。

典型用法:
    mgr = SerialManager()
    mgr.data_received.connect(on_data)
    mgr.connect('COM3')
"""
import re
import serial
import serial.tools.list_ports
from PyQt6.QtCore import QObject, QThread, pyqtSignal



# ── 支持的命名映射 (接收到的 key → 标准 key) ──────────────
KEY_ALIAS = {
    # 电压
    "vin": "vin", "v_in": "vin", "input_voltage": "vin",
    "vout": "vout", "v_out": "vout", "output_voltage": "vout",
    "voltage": "vout",
    # 电流
    "iin": "iin", "i_in": "iin", "input_current": "iin",
    "iout": "iout", "i_out": "iout", "output_current": "iout",
    "current": "iout",
    # 功率
    "pin": "pin", "p_in": "pin", "input_power": "pin",
    "pout": "pout", "p_out": "pout", "output_power": "pout",
    "power": "pout",
    # 其它
    "il": "il", "inductor_current": "il", "i_l": "il",
    "eff": "eff", "efficiency": "eff",
    "mode": "mode",
    "topo": "topology", "topology": "topology",
    "temp": "temp", "temperature": "temp",
}


class SerialWorker(QObject):
    """串口工作线程 — 在后台线程中持续读取数据，逐行解析并发射"""

    data_received = pyqtSignal(dict)           # 解析后的数据字典
    raw_data_received = pyqtSignal(bytes)      # 原始字节流 (供二进制协议解析)
    connection_changed = pyqtSignal(bool)       # True=已连接, False=已断开
    error_occurred = pyqtSignal(str)            # 错误信息

    def __init__(self):
        super().__init__()
        self._port = None
        self._running = False
        self._buffer = ""
        self._baudrate = 4000000

    # ── 连接 / 断开 ────────────────────────────────────────

    def connect(self, port_name: str, baudrate: int = 4000000) -> bool:
        """打开串口，启动后将运行 run()"""
        try:
            self._port = serial.Serial(
                port=port_name,
                baudrate=baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.05,
            )
            self._baudrate = baudrate
            self._running = True
            self._buffer = ""
            self.connection_changed.emit(True)
            return True
        except Exception as e:
            self.error_occurred.emit(str(e))
            return False

    def disconnect(self):
        self._running = False
        if self._port and self._port.is_open:
            try:
                self._port.close()
            except Exception:
                pass
        self.connection_changed.emit(False)

    @property
    def is_open(self) -> bool:
        return self._port is not None and self._port.is_open

    def send(self, cmd: str):
        """发送文本命令 (自动追加换行)"""
        if self._port and self._port.is_open:
            self._port.write((cmd + "\n").encode("utf-8"))

    # ── 主循环 ─────────────────────────────────────────────

    def run(self):
        """读取循环 — 由 QThread.started 触发"""
        while self._running:
            try:
                if self._port and self._port.is_open and self._port.in_waiting > 0:
                    raw = self._port.read(self._port.in_waiting)

                    # 发射原始字节供二进制协议解析器使用
                    self.raw_data_received.emit(raw)

                    # 原有文本解析流程
                    text = raw.decode("utf-8", errors="replace")
                    self._buffer += text

                    # 逐行切割处理
                    while "\n" in self._buffer:
                        line, self._buffer = self._buffer.split("\n", 1)
                        line = line.strip()
                        if line:
                            parsed = self._parse(line)
                            if parsed:
                                self.data_received.emit(parsed)
                else:
                    QThread.msleep(10)
            except Exception as e:
                self.error_occurred.emit(str(e))
                self._running = False
                break

    # ── 解析器 ─────────────────────────────────────────────

    @staticmethod
    def _parse(line: str):
        """
        解析一行文本数据，返回规范化字典或 None。

        支持格式 (自动检测):
          1. key=value[,key=value...]     e.g.  Vin=12.5,Iin=2.0,Vout=5.0
          2. CSV 数值序列 (V, I, P)        e.g.  12.5, 2.0, 25.0
          3. JSON 对象                    e.g.  {"voltage":12.5,"current":2.0}
        """
        line = line.strip()
        if not line:
            return None

        data = {}

        # ── 尝试 JSON ──
        if line.startswith("{"):
            try:
                import json
                obj = json.loads(line)
                if isinstance(obj, dict):
                    for k, v in obj.items():
                        key = KEY_ALIAS.get(k.lower(), k.lower())
                        data[key] = float(v) if isinstance(v, (int, float)) else v
                    return data
            except Exception:
                pass

        # ── 尝试 key=value 格式 ──
        pairs = re.findall(r"([a-zA-Z_]\w*)\s*=\s*([-\d.]+(?:e[+-]?\d+)?)", line)
        if pairs:
            for k, v in pairs:
                key = KEY_ALIAS.get(k.lower(), k.lower())
                try:
                    data[key] = float(v)
                except ValueError:
                    data[key] = v
            return data

        # ── 尝试纯 CSV 数字 (v, i, p) ──
        nums = re.findall(r"[-\d.]+(?:e[+-]?\d+)?", line)
        if len(nums) >= 3:
            return {"vout": float(nums[0]), "iout": float(nums[1]), "pout": float(nums[2])}

        return None


class SerialManager(QObject):
    """
    串口管理器 — 高级 API 封装。

    信号:
        data_received(dict)        — 收到解析后的数据
        raw_data_received(bytes)   — 原始字节流 (供二进制协议解析)
        connection_changed(bool)   — 连接状态变化
        error_occurred(str)        — 错误消息
    """
    data_received = pyqtSignal(dict)
    raw_data_received = pyqtSignal(bytes)
    connection_changed = pyqtSignal(bool)
    error_occurred = pyqtSignal(str)

    def __init__(self):
        super().__init__()
        self._worker = SerialWorker()
        self._thread = QThread()
        self._worker.moveToThread(self._thread)

        # 跨线程信号转发
        self._worker.data_received.connect(self.data_received)
        self._worker.raw_data_received.connect(self.raw_data_received)
        self._worker.connection_changed.connect(self.connection_changed)
        self._worker.error_occurred.connect(self.error_occurred)
        self._thread.started.connect(self._worker.run)

    # ── 静态工具 ───────────────────────────────────────────

    @staticmethod
    def scan_ports():
        """扫描可用串口 → ['COM1', 'COM3', ...]"""
        return [p.device for p in serial.tools.list_ports.comports()]

    @staticmethod
    def scan_ports_detailed():
        """扫描可用串口 → ['COM3 (USB Serial Port)', ...]"""
        return [
            f"{p.device} ({p.description})"
            for p in serial.tools.list_ports.comports()
        ]

    # ── 连接管理 ───────────────────────────────────────────

    def connect(self, port, baudrate=115200):
        """连接到指定串口 (自动断开已有连接)"""
        self.disconnect()
        # 兼容 "COM3 (描述)" 格式
        if " (" in port:
            port = port.split(" (")[0]
        ok = self._worker.connect(port, baudrate)
        if ok and not self._thread.isRunning():
            self._thread.start()
        return ok

    def disconnect(self):
        """断开当前连接"""
        if self._thread.isRunning():
            self._thread.quit()
            self._thread.wait(2000)
        self._worker.disconnect()

    def send(self, cmd: str):
        """发送命令到设备"""
        self._worker.send(cmd)

    @property
    def is_connected(self) -> bool:
        return self._worker.is_open

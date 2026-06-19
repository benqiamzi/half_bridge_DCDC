"""
串口通信管理模块
==================
独立文件，负责串口扫描、连接与原始字节收发。
数据解析由 data_process.py 中的 DataProcessor 处理。

典型用法:
    mgr = SerialManager()
    mgr.raw_data_received.connect(data_processor.feed)
    mgr.connect('COM3')
"""
import serial
import serial.tools.list_ports
from PyQt6.QtCore import QObject, QThread, pyqtSignal


class SerialWorker(QObject):
    """串口工作线程 — 在后台线程中持续读取原始字节并发射"""

    raw_data_received = pyqtSignal(bytes)      # 原始字节流 (供二进制协议解析)
    connection_changed = pyqtSignal(bool)       # True=已连接, False=已断开
    error_occurred = pyqtSignal(str)            # 错误信息

    def __init__(self):
        super().__init__()
        self._port = None
        self._running = False
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

    def send(self, data: bytes):
        """发送原始字节"""
        if self._port and self._port.is_open:
            self._port.write(data)

    # ── 主循环 ─────────────────────────────────────────────

    def run(self):
        """读取循环 — 由 QThread.started 触发"""
        while self._running:
            try:
                if self._port and self._port.is_open and self._port.in_waiting > 0:
                    raw = self._port.read(self._port.in_waiting)
                    self.raw_data_received.emit(raw)
                else:
                    QThread.msleep(10)
            except Exception as e:
                self.error_occurred.emit(str(e))
                self._running = False
                break


class SerialManager(QObject):
    """
    串口管理器 — 高级 API 封装。

    信号:
        raw_data_received(bytes)   — 原始字节流 (供二进制协议解析)
        connection_changed(bool)   — 连接状态变化
        error_occurred(str)        — 错误消息
    """
    raw_data_received = pyqtSignal(bytes)
    connection_changed = pyqtSignal(bool)
    error_occurred = pyqtSignal(str)

    def __init__(self):
        super().__init__()
        self._worker = SerialWorker()
        self._thread = QThread()
        self._worker.moveToThread(self._thread)

        # 跨线程信号转发
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

    def connect(self, port, baudrate=4000000):
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

    def send(self, data: bytes):
        """发送原始字节到设备"""
        self._worker.send(data)

    @property
    def is_connected(self) -> bool:
        return self._worker.is_open

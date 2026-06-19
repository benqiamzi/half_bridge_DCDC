"""
高速绘图组件
==============
针对 40kHz 数据率优化：
- 内部使用 collections.deque 实现 O(1) FIFO
- 提供 batch_update() 批量追加，避免逐点 set_data
- 限制显示窗口（默认 4000 点），自动降采样
"""
from collections import deque

import matplotlib.pyplot as plt
from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
from matplotlib import font_manager as fm

from PyQt6.QtWidgets import QWidget, QVBoxLayout

# ── 中文字体 ──────────────────────────────────────────────
for name in ("Microsoft YaHei", "SimHei", "SimSun",
             "Source Han Sans SC", "Noto Sans CJK SC"):
    if name in {f.name for f in fm.fontManager.ttflist}:
        plt.rcParams["font.family"] = name
        break
plt.rcParams["axes.unicode_minus"] = False


class PlotWidget(QWidget):
    """高速实时绘图控件 — 支持批量追加、滚动窗口、自动降采样"""

    DISPLAY_MAX = 4000          # 屏幕最大显示点数
    DT = 25e-6                  # 默认采样间隔 25µs (40kHz)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._times   = deque(maxlen=self.DISPLAY_MAX)
        self._volts   = deque(maxlen=self.DISPLAY_MAX)
        self._currents = deque(maxlen=self.DISPLAY_MAX)
        self._powers  = deque(maxlen=self.DISPLAY_MAX)
        self._init_ui()

    # ── UI ─────────────────────────────────────────────────

    def _init_ui(self):
        self.figure = plt.figure(figsize=(12, 8), facecolor="#2b2b2b")
        self.canvas = FigureCanvas(self.figure)
        layout = QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self.canvas)
        self.setLayout(layout)

        # 子图 1：电压 / 电流
        self.ax1 = self.figure.add_subplot(211)
        self.ax1.set_facecolor("#1e1e1e")
        self.ax1.tick_params(axis="x", colors="white")
        self.ax1.tick_params(axis="y", colors="white")
        self.ax1.set_xlabel("时间 (s)", color="white")
        self.ax1.set_ylabel("电压 / 电流", color="white")
        self.line_voltage, = self.ax1.plot([], [], color="#00ff88",
                                           linewidth=1.0, label="电压 (V)")
        self.line_current, = self.ax1.plot([], [], color="#ff6666",
                                           linewidth=1.0, label="电流 (A)")
        self.ax1.legend(loc="upper right", facecolor="#2b2b2b", labelcolor="white")
        self.ax1.grid(True, alpha=0.15)

        # 子图 2：功率
        self.ax2 = self.figure.add_subplot(212)
        self.ax2.set_facecolor("#1e1e1e")
        self.ax2.tick_params(axis="x", colors="white")
        self.ax2.tick_params(axis="y", colors="white")
        self.ax2.set_xlabel("时间 (s)", color="white")
        self.ax2.set_ylabel("功率 (W)", color="white")
        self.line_power, = self.ax2.plot([], [], color="#0088ff",
                                         linewidth=1.0, label="功率 (W)")
        self.ax2.legend(loc="upper right", facecolor="#2b2b2b", labelcolor="white")
        self.ax2.grid(True, alpha=0.15)

        self.figure.subplots_adjust(
            left=0.06, right=0.97, bottom=0.08, top=0.97, hspace=0.20
        )

    # ── 批量追加（主入口 — 被定时器调用） ──────────────────

    def batch_update(self, times, voltages, currents, powers):
        """
        批量追加一批数据点，自动 FIFO + 降采样后重绘。
        times: 绝对时间戳列表（秒）
        """
        self._times.extend(times)
        self._volts.extend(voltages)
        self._currents.extend(currents)
        self._powers.extend(powers)

        # 取内部数据并降采样
        arr_t, arr_v, arr_c, arr_p = self._decimate()
        self.line_voltage.set_data(arr_t, arr_v)
        self.line_current.set_data(arr_t, arr_c)
        self.line_power.set_data(arr_t, arr_p)

        self.ax1.relim()
        self.ax1.autoscale_view()
        self.ax2.relim()
        self.ax2.autoscale_view()
        self.canvas.draw_idle()

    # ── 文件导入 ───────────────────────────────────────────

    def load_data(self, times, voltages, currents, powers):
        """批量加载（文件导入），替换当前缓冲区"""
        self._times   = deque(times[-self.DISPLAY_MAX:], maxlen=self.DISPLAY_MAX)
        self._volts   = deque(voltages[-self.DISPLAY_MAX:], maxlen=self.DISPLAY_MAX)
        self._currents = deque(currents[-self.DISPLAY_MAX:], maxlen=self.DISPLAY_MAX)
        self._powers  = deque(powers[-self.DISPLAY_MAX:], maxlen=self.DISPLAY_MAX)

        arr_t, arr_v, arr_c, arr_p = self._decimate()
        self.line_voltage.set_data(arr_t, arr_v)
        self.line_current.set_data(arr_t, arr_c)
        self.line_power.set_data(arr_t, arr_p)
        self.ax1.relim()
        self.ax1.autoscale_view()
        self.ax2.relim()
        self.ax2.autoscale_view()
        self.canvas.draw_idle()

    def clear_plot(self):
        """清空"""
        self._times.clear()
        self._volts.clear()
        self._currents.clear()
        self._powers.clear()
        self.line_voltage.set_data([], [])
        self.line_current.set_data([], [])
        self.line_power.set_data([], [])
        self.ax1.relim()
        self.ax1.autoscale_view()
        self.ax2.relim()
        self.ax2.autoscale_view()
        self.canvas.draw_idle()

    # ── 降采样 ─────────────────────────────────────────────

    def _decimate(self):
        """
        当数据点数超过 DISPLAY_MAX 时均匀降采样，
        返回 (times, voltages, currents, powers) 四个 list。
        """
        n = len(self._times)
        if n <= self.DISPLAY_MAX:
            return list(self._times), list(self._volts), \
                   list(self._currents), list(self._powers)

        step = n // self.DISPLAY_MAX
        # 将所有 deque 转 list 后切片
        t = list(self._times)[::step]
        v = list(self._volts)[::step]
        c = list(self._currents)[::step]
        p = list(self._powers)[::step]
        return t, v, c, p
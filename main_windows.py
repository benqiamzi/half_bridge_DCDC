"""
主窗口 — 半桥 DC-DC 电源监控系统
=====================================
集成了串口通信、实时绘图、数据导入导出功能。
"""
import csv
import os
import sys
from collections import deque

from PyQt6.QtCore import QTimer
from PyQt6.QtGui import QAction
from PyQt6.QtWidgets import (
    QApplication,
    QFileDialog,
    QMainWindow,
    QMessageBox,
    QVBoxLayout,
    QWidget,
)

from ui_main import Ui_MainWindow
from data_process import DataProcessor
from matlab_plot import PlotWidget
from set_param_widget import ParamSetWidget
from serial_manager import SerialManager


class MainWindow(QMainWindow):
    """半桥 DC-DC 电源监控主窗口"""

    def __init__(self):
        super().__init__()
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)

        # ── 核心组件 ──
        self.serial_mgr = SerialManager()
        self.data_processor = DataProcessor()
        self.plot_widget = PlotWidget()
        self._serial_connected = False

        # ── 40kHz 显示节流 ─────────────────────────────────
        # _on_data_received 只做缓冲，不碰 UI
        self._display_buffer = []          # 累积待刷新的数据点
        self._frame_count = 0              # 总帧数（用于时间戳）
        self._display_timer = QTimer()
        self._display_timer.setInterval(30)   # ~33 Hz 刷新
        self._display_timer.timeout.connect(self._flush_display)

        self._init_ui()
        self._init_serial()
        self._init_settings()
        self._init_menu()
        self._apply_style()

    # ================================================================
    #  1. 界面布局 — 控制面板 + 绘图（跟随缩放）
    # ================================================================

    def _init_ui(self):
        """布局：上方控制面板 + 下方绘图（窗口缩放时自动跟随）"""
        self.setWindowTitle("半桥 DC-DC 电源监控系统")
        self.setMinimumSize(1100, 850)

        # 用布局接管 centralwidget，原 widget 不动，plot 追加到下方
        layout = QVBoxLayout(self.ui.centralwidget)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(4)
        layout.addWidget(self.ui.layoutWidget)
        layout.addWidget(self.plot_widget, 1)

        self._cleanup_grid()

    def _cleanup_grid(self):
        """统一设置数据值标签的样式"""
        for name in ("label_2", "Vin", "Vout", "Iin", "Iout",
                     "iL", "out_mode", "efficiency", "out_power", "duty"):
            lbl = getattr(self.ui, name)
            lbl.setStyleSheet("color: #00ff88; font-weight: bold; font-size: 14px;")

    # ================================================================
    #  2. 串口功能
    # ================================================================

    def _init_serial(self):
        # 数据处理器管线: 串口原始字节 → 二进制帧解析 → 显示
        self.serial_mgr.raw_data_received.connect(self.data_processor.feed)
        self.data_processor.data_ready.connect(self._on_data_received)
        self.data_processor.frame_error.connect(
            lambda msg: self.statusBar().showMessage(f"⚠️ {msg}")
        )

        self.serial_mgr.connection_changed.connect(self._on_connection_changed)
        self.serial_mgr.error_occurred.connect(self._on_serial_error)

        self.ui.pushButton.clicked.connect(self._toggle_serial)       # 开启/关闭
        self.ui.pushButton_2.clicked.connect(self._refresh_ports)     # 刷新

        # 启动时自动填入定时器做端口轮询
        self._refresh_ports()

    def _refresh_ports(self):
        self.ui.comboBox.clear()
        ports = SerialManager.scan_ports_detailed()
        if ports:
            self.ui.comboBox.addItems(ports)
        else:
            self.ui.comboBox.addItem("未检测到串口")

    def _toggle_serial(self):
        if not self._serial_connected:
            port = self.ui.comboBox.currentText()
            if not port or port == "未检测到串口":
                QMessageBox.warning(self, "提示", "请先选择一个串口")
                return
            if self.serial_mgr.connect(port):
                self.ui.pushButton.setText("⏏  关闭串口")
                self.statusBar().showMessage(f"✅ 已连接 {port}")
                self._display_buffer.clear()
                self._frame_count = 0
                self._display_timer.start()
            else:
                QMessageBox.warning(self, "连接失败", f"无法打开串口 {port}")
        else:
            self.serial_mgr.disconnect()
            self.ui.pushButton.setText("▶  开启串口")
            self.statusBar().showMessage("⏹ 串口已断开")

    # ── 数据回调（40kHz 线程 — 只缓冲，不碰 UI） ──────────

    def _on_data_received(self, data: dict):
        """串口数据到达 → 放入缓冲，定时器负责刷新显示"""
        self._display_buffer.append(data)
        self._frame_count += 1

    # ── 显示刷新（~33 Hz 在主线程执行） ────────────────────

    def _flush_display(self):
        """定时器触发：将缓冲数据批量推送到标签和绘图"""
        if not self._display_buffer:
            return

        # 取最新值更新数字标签
        latest = self._display_buffer[-1]
        vin  = float(latest.get("vin", 0))
        iin  = float(latest.get("iin", 0))
        vout = float(latest.get("vout", 0))
        iout = float(latest.get("iout", 0))
        pout = float(latest.get("pout", 0))
        il   = float(latest.get("il", 0))
        eff  = latest.get("eff")

        self.ui.Vin.setText(f"{vin:.3f}V")
        self.ui.Iin.setText(f"{iin:.3f}A")
        self.ui.Vout.setText(f"{vout:.3f}V")
        self.ui.Iout.setText(f"{iout:.3f}A")
        self.ui.iL.setText(f"{il:.3f}A")
        self.ui.out_power.setText(f"{pout:.2f}W")
        if eff is not None:
            self.ui.efficiency.setText(f"{float(eff):.1f}%")
        duty = latest.get("duty", 0)
        self.ui.duty.setText(f"{float(duty)*100:.1f}%")
        mode = latest.get("mode", "")
        topo = latest.get("topology", "")
        if mode:
            self.ui.out_mode.setText(str(mode).upper())
        if topo:
            self.ui.label_2.setText(str(topo).upper())

        # 批量更新绘图（所有缓冲数据）  DT=25µs @40kHz
        n = len(self._display_buffer)
        base_t = (self._frame_count - n) * 25e-6
        times = [base_t + i * 25e-6 for i in range(n)]

        vouts  = [d.get("vout", 0) for d in self._display_buffer]
        iouts  = [d.get("iout", 0) for d in self._display_buffer]
        pouts  = [d.get("pout", 0) for d in self._display_buffer]

        self.plot_widget.batch_update(times, vouts, iouts, pouts)
        self._display_buffer.clear()

    def _on_connection_changed(self, connected: bool):
        self._serial_connected = connected
        if not connected:
            self._display_timer.stop()
            self.ui.pushButton.setText("▶  开启串口")
            self.ui.pushButton.setEnabled(True)

    def _on_serial_error(self, error: str):
        self.statusBar().showMessage(f"⚠️ 串口错误: {error}")

    # ================================================================
    #  3. 参数设置子界面
    # ================================================================

    def _init_settings(self):
        """绑定「打开设置界面」按钮"""
        self.ui.open_set_btn.clicked.connect(self._open_settings)
        self._settings_window = None

    def _open_settings(self):
        """弹出参数设置子窗口（单例，防止重复打开）"""
        if self._settings_window is not None:
            self._settings_window.raise_()
            self._settings_window.activateWindow()
            return
        self._settings_window = ParamSetWidget()
        self._settings_window.destroyed.connect(
            lambda: setattr(self, "_settings_window", None)
        )
        self._settings_window.show()

    # ================================================================
    #  4. 菜单栏（文件导入 / 导出 / 清除）
    # ================================================================

    def _init_menu(self):
        file_menu = self.menuBar().addMenu("文件")

        act_import = QAction("导入数据文件...", self)
        act_import.triggered.connect(self._import_data)
        file_menu.addAction(act_import)

        act_export = QAction("导出数据...", self)
        act_export.triggered.connect(self._export_data)
        file_menu.addAction(act_export)

        file_menu.addSeparator()

        act_clear = QAction("清除数据", self)
        act_clear.triggered.connect(self.plot_widget.clear_plot)
        file_menu.addAction(act_clear)

    def _import_data(self):
        """从 CSV 导入数据并绘制"""
        path, _ = QFileDialog.getOpenFileName(
            self, "导入数据", "", "CSV 文件 (*.csv);;所有文件 (*)"
        )
        if not path:
            return
        try:
            times, volts, currents, powers = [], [], [], []
            with open(path, "r", encoding="utf-8-sig") as f:
                reader = csv.DictReader(f)
                for row in reader:
                    times.append(float(row.get("time", row.get("t", len(times)))))
                    volts.append(float(row.get("voltage", row.get("v", 0))))
                    currents.append(float(row.get("current", row.get("i", 0))))
                    powers.append(float(row.get("power", row.get("p", 0))))
            if times:
                self.plot_widget.load_data(times, volts, currents, powers)
                self.statusBar().showMessage(
                    f"✅ 已导入 {len(times)} 条数据 — {os.path.basename(path)}"
                )
            else:
                QMessageBox.warning(self, "导入失败", "文件中未找到有效数据")
        except Exception as e:
            QMessageBox.critical(self, "导入错误", str(e))

    def _export_data(self):
        """将当前数据导出为 CSV"""
        if not self.plot_widget.times:
            QMessageBox.information(self, "提示", "没有可导出的数据")
            return
        path, _ = QFileDialog.getSaveFileName(
            self, "导出数据", "power_data.csv", "CSV 文件 (*.csv)"
        )
        if not path:
            return
        try:
            with open(path, "w", newline="", encoding="utf-8-sig") as f:
                writer = csv.writer(f)
                writer.writerow(["time", "voltage", "current", "power"])
                for t, v, c, p in zip(
                    self.plot_widget.times,
                    self.plot_widget.voltages,
                    self.plot_widget.currents,
                    self.plot_widget.powers,
                ):
                    writer.writerow([t, v, c, p])
            self.statusBar().showMessage(
                f"✅ 已导出到 {os.path.basename(path)}"
            )
        except Exception as e:
            QMessageBox.critical(self, "导出错误", str(e))

    # ================================================================
    #  5. 暗色主题 QSS
    # ================================================================

    def _apply_style(self):
        self.setStyleSheet("""
            /* 全局 */
            QMainWindow { background-color: #1e1e1e; }
            QWidget     { color: #cccccc; font-size: 13px; }

            /* 标签 */
            QLabel {
                color: #cccccc;
                background: transparent;
            }

            /* 按钮 */
            QPushButton {
                background-color: #2d2d2d;
                color: #e0e0e0;
                border: 1px solid #555;
                border-radius: 4px;
                padding: 6px 18px;
                min-height: 26px;
            }
            QPushButton:hover {
                background-color: #3a3a3a;
                border-color: #00ff88;
            }
            QPushButton:pressed {
                background-color: #4a4a4a;
            }
            QPushButton#open_set_btn {
                background-color: #005f3f;
                border-color: #00aa66;
            }
            QPushButton#open_set_btn:hover {
                background-color: #007a55;
            }

            /* 下拉框 */
            QComboBox {
                background-color: #2d2d2d;
                color: #e0e0e0;
                border: 1px solid #555;
                border-radius: 4px;
                padding: 4px 10px;
                min-height: 26px;
            }
            QComboBox:hover { border-color: #00ff88; }
            QComboBox::drop-down {
                subcontrol-origin: padding;
                width: 24px;
                border-left: 1px solid #555;
            }
            QComboBox QAbstractItemView {
                background-color: #2d2d2d;
                color: #e0e0e0;
                selection-background-color: #00ff88;
                selection-color: #000;
                border: 1px solid #555;
                outline: none;
            }

            /* 菜单 */
            QMenuBar {
                background-color: #252525;
                color: #cccccc;
                border-bottom: 1px solid #333;
            }
            QMenuBar::item:selected {
                background-color: #00ff88;
                color: #000;
            }
            QMenu {
                background-color: #2b2b2b;
                color: #cccccc;
                border: 1px solid #555;
            }
            QMenu::item:selected {
                background-color: #00ff88;
                color: #000;
            }

            /* 状态栏 */
            QStatusBar {
                background-color: #252525;
                color: #aaaaaa;
                border-top: 1px solid #333;
            }
        """)


# ================================================================
#  入口
# ================================================================

def main():
    app = QApplication(sys.argv)
    app.setStyle("Fusion")          # 跨平台统一风格
    window = MainWindow()
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()

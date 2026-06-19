"""
参数设置子窗口
================
点击主窗口「打开设置界面」按钮时弹出的独立子窗口，
用于配置输出参数和保护参数。
"""
from PyQt6.QtCore import Qt

from PyQt6.QtWidgets import (
    QApplication,

    QWidget,
)

class ParamSetWidget(QWidget):
    """参数设置子窗口 (基于 Ui_Form 生成的布局)"""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("参数设置")
        self.setMinimumSize(540, 420)
        self.setAttribute(Qt.WidgetAttribute.WA_DeleteOnClose)

        # 动态导入 Ui_Form（从本地同目录的 .ui 编译产物）
        from Ui_param_set_widget import Ui_Form
        self.ui = Ui_Form()
        self.ui.setupUi(self)

        self._apply_style()

    def _apply_style(self):
        """暗色主题，与主窗口风格统一"""
        self.setStyleSheet("""
            QWidget {
                background-color: #1e1e1e;
                color: #cccccc;
                font-size: 13px;
            }
            QTabWidget::pane {
                border: 1px solid #444;
                background-color: #1a1a1a;
            }
            QTabBar::tab {
                background-color: #2d2d2d;
                color: #aaaaaa;
                padding: 8px 20px;
                border: 1px solid #444;
                border-bottom: none;
                border-top-left-radius: 4px;
                border-top-right-radius: 4px;
            }
            QTabBar::tab:selected {
                background-color: #1a1a1a;
                color: #00ff88;
                border-bottom: 2px solid #00ff88;
            }
            QTabBar::tab:hover:!selected {
                background-color: #3a3a3a;
            }
            QLabel {
                background: transparent;
                color: #cccccc;
            }
            QPushButton {
                background-color: #2d2d2d;
                color: #e0e0e0;
                border: 1px solid #555;
                border-radius: 3px;
                padding: 2px 12px;
                min-height: 24px;
            }
            QPushButton:hover {
                background-color: #3a3a3a;
                border-color: #00ff88;
            }
            QPushButton:pressed {
                background-color: #4a4a4a;
            }
            QDoubleSpinBox {
                background-color: #2d2d2d;
                color: #00ff88;
                border: 1px solid #555;
                border-radius: 3px;
                padding: 2px 6px;
                min-height: 22px;
            }
            QDoubleSpinBox:focus {
                border-color: #00ff88;
            }
        """)


# ================================================================
#  入口
# ================================================================

def main():
    import sys

    app = QApplication(sys.argv)
    app.setStyle("Fusion")          # 跨平台统一风格
    window = ParamSetWidget()
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()

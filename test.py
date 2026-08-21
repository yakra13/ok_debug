#!/usr/bin/python
# python3 -m pip install PySide6
from pathlib import Path
from typing import Iterable

import sys

from PySide6.QtWidgets import(
    QApplication,
    QMainWindow,
    QTreeWidget,
    QTreeWidgetItem,
)


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("Data Viewer")
        self.resize(1000, 700)

        tree = QTreeWidget()
        tree.setHeaderLabels(["Name", "Value"])

        processes = QTreeWidgetItem(["Processes", ""])
        processes.addChild(QTreeWidgetItem(["explorer.exe", "PID 1234"]))
        processes.addChild(QTreeWidgetItem(["svchost.exe", "PID 800"]))

        drives = QTreeWidgetItem(["Drives", ""])
        drives.addChild(QTreeWidgetItem(["C:\\", "Fixed drive"]))
        drives.addChild(QTreeWidgetItem(["D:\\", "CD-ROM drive"]))

        tree.addTopLevelItem(processes)
        tree.addTopLevelItem(drives)

        self.setCentralWidget(tree)


app = QApplication(sys.argv)

window = MainWindow()
window.show()

sys.exit(app.exec())
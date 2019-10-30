from socket import *

from PyQt5 import QtWidgets, QtCore, QtGui
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QListView, QMessageBox
from PyQt5.QtCore import QStringListModel
from ftp_login import Ui_login
from ftp_main import Ui_main
from PyQt5.QtWidgets import QFileDialog, QInputDialog
import sys

import time
import os
import re
import operator

HOST = '127.0.0.1'
PORT = 6789 # 标准客户端21.自己的server是6789
BUFFSIZE = 8192
ADDR = (HOST, PORT)

# list的索引长度
index = 0

# 创建control socket
server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
# 连接
server_socket.connect(ADDR)
# 创建data socket
client_socket = None
# 创建port的监视socket
port_socket = None


connect_info = server_socket.recv(BUFFSIZE)


class login_window(QtWidgets.QWidget, Ui_login):
    def __init__(self):
        super(login_window, self).__init__()
        self.setupUi_login(self)
        self.connect_line.setText(connect_info.decode('utf-8'))
        # 把确定按钮与confirm函数连接起来
        self.login_btn.clicked.connect(self.confirm)

    def confirm(self):
        try:
            # 获取用户名,得到命令:USER username并传入server
            user_name = self.user_line.text()
            command = 'USER ' + user_name + '\r\n'
            server_socket.send(command.encode())
            info_back = server_socket.recv(BUFFSIZE).decode('utf-8')
            self.connect_line.setText(info_back)
            # 正则表达式提取数字部分
            match = re.search(r'\d+', info_back)
            if operator.eq(match[0], '331'):
                #隐藏用户名输入,显示密码输入,改变按钮
                self.user_label.setVisible(False)
                self.user_line.setVisible(False)
                self.pass_label.setVisible(True)
                self.pass_line.setVisible(True)
                self.login_btn.setVisible(False)
                self.login_btn_2.setVisible(True)
                #把完成按钮与login函数连接起来
                self.login_btn_2.clicked.connect(self.finish)
        except Exception as e:
            print(e)

    def finish(self):
        try:
            # 获取用密码,得到命令:PASS password并传入server
            pass_word = self.pass_line.text()
            command = 'PASS ' + pass_word + '\r\n'
            server_socket.send(command.encode())
            info_back = server_socket.recv(BUFFSIZE).decode('utf-8')
            self.connect_line.setText(info_back)
            # 正则表达式提取数字部分
            match = re.search(r'\d+', info_back)
            if operator.eq(match[0], '230'):
                # 显示用户名输入,显示密码输入,改变按钮
                self.user_label.setGeometry(QtCore.QRect(50, 137, 58, 18))
                self.user_line.setGeometry(QtCore.QRect(140, 130, 180, 32))
                self.pass_label.setGeometry(QtCore.QRect(50, 197, 58, 18))
                self.pass_line.setGeometry(QtCore.QRect(140, 190, 180, 32))
                self.user_label.setVisible(True)
                self.user_line.setVisible(True)
                self.login_btn_2.setVisible(False)
                self.login_btn_3.setVisible(True)
        except Exception as e:
            print(e)


class main_window(QtWidgets.QMainWindow, Ui_main):
    def __init__(self):
        super(main_window, self).__init__()
        self.setupUi_main(self)
        # 将ContextMenuPolicy设置为Qt.CustomContextMenu,否则无法使用customContextMenuRequested信号
        self.list.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        # 显示欢迎信息
        info_back = 'Welcome to FTP Client by tuyc17 !'
        self.info_line.setText(info_back)
        # 获取当前程序文件位置
        self.cwd = os.getcwd()

        # 连接按钮和对应函数
        self.source_btn.clicked.connect(self.update_list)
        self.scan_btn.clicked.connect(self.select_file)
        self.scan_btn_2.clicked.connect(self.select_dir)
        self.upload_btn.clicked.connect(self.upload_file)
        self.new_btn.clicked.connect(self.make_dir)
        self.previous_btn.clicked.connect(self.go_previous)
        self.restart_small_btn.clicked.connect(self.restart_small)
        # 创建QMenu信号事件
        self.list.customContextMenuRequested.connect(self.show_menu)
        self.list.contextMenu = QtWidgets.QMenu(self)
        self.list.ENTER = self.list.contextMenu.addAction('打开文件夹')
        self.list.DELETE = self.list.contextMenu.addAction('删除空文件夹')
        self.list.DOWNLOAD = self.list.contextMenu.addAction('下载')
        self.list.RENAME = self.list.contextMenu.addAction('重命名')
        # 右键菜单事件绑定
        self.list.ENTER.triggered.connect(self.enter)
        self.list.DELETE.triggered.connect(self.delete_file)
        self.list.DOWNLOAD.triggered.connect(self.download_file)
        self.list.RENAME.triggered.connect(self.rename_file)
        # 菜单栏事件绑定
        self.actionAbout.triggered.connect(self.show_info)

    # 菜单信息
    def show_info(self):
        QMessageBox.information(self, 'About...', 'By Tuyc17.\n    2019.10.30')

    # pasc/port
    def set_port(self):
        global client_socket
        global port_socket
        try:
            mode = self.combo_box.currentText()
            if client_socket is not None:
                client_socket.close()
                client_socket = None
            # 疑似有问题
            if operator.eq(mode, 'Passive'):
                client_socket = socket(AF_INET, SOCK_STREAM, 0)
                command = 'PASV\r\n'
                server_socket.send(command.encode())
                info_back = server_socket.recv(BUFFSIZE).decode('utf-8')
                # 正则表达式
                match = re.search(r'\d+(?:,\d+){5}', info_back)
                nums = match[0].split(',')
                client_host = nums[0] + '.' + nums[1] + '.' + nums[2] + '.' + nums[3]
                client_port = int(nums[4]) * 256 + int(nums[5])
                client_addr = (client_host, client_port)
                client_socket.connect(client_addr)
                # self.info_line.setText(info_back)
            elif operator.eq(mode, 'Port'):
                port_socket = socket(AF_INET, SOCK_STREAM, 0)
                client_host = server_socket.getsockname()[0]
                client_port = 0
                client_addr = (client_host, client_port)
                port_socket.bind(client_addr)
                port_socket.listen(1)
                client_sockname = port_socket.getsockname()
                tmp_str_1 = str(client_sockname[1] // 256)
                tmp_str_2 = str(client_sockname[1] % 256)
                tmp_host = client_sockname[0]
                tmp_host = tmp_host.replace('.', ',')
                command = 'PORT ' + tmp_host + ',' + tmp_str_1 + ',' + tmp_str_2 + '\r\n'
                server_socket.send(command.encode())
                # 不急着把portsocket做accept来取clientsocket和关掉portsocket
                info_back = server_socket.recv(BUFFSIZE).decode('utf-8')
                info_back = info_back + tmp_host + ',' + tmp_str_1 + ',' + tmp_str_2
                # self.info_line.setText(info_back)
        except Exception as e:
            print(e)

    # 刷新list按钮,同时执行SYST和TYPE I
    def update_list(self):
        global index
        global client_socket
        global port_socket
        # global server_socket
        try:
            # 发送SYST和TYPE I
            command = 'SYST\r\n'
            server_socket.send(command.encode())
            info_back = server_socket.recv(BUFFSIZE).decode('utf-8')
            command = 'TYPE I\r\n'
            server_socket.send(command.encode())
            info_back = server_socket.recv(BUFFSIZE).decode('utf-8')
            # 发送PWD指令,获取服务端的位置
            command = 'PWD\r\n'
            server_socket.send(command.encode())
            info_back = server_socket.recv(BUFFSIZE).decode('utf-8')
            # 正则表达式搜索两双引号之间的内容
            match = re.search(r'".+"', info_back)
            info_back = match[0].replace('"', '')
            self.source_line.setText(info_back)
            # 根据info_back执行LIST并清空control socket的消息.去data socket接收文件名
            self.set_port()
            command = 'LIST\r\n'
            server_socket.send(command.encode())
            # 发送LIST command后,检测port_socket,做accept并赋值None
            if port_socket is not None:
                client_socket = port_socket.accept()[0]
                port_socket.close()
                port_socket = None
            info_back = server_socket.recv(BUFFSIZE).decode('utf-8')
            time.sleep(0.1)
            # self.info_line.setText(info_back)
            # 收到文件列表
            info_back = client_socket.recv(BUFFSIZE).decode('utf-8')
            # 清空self.list中的内容
            self.list.clear()
            # 分割得到文件名
            lists = info_back.split('\n')
            for cheese in lists:
                infos = cheese.split(' ')
                item = QtWidgets.QListWidgetItem()
                self.list.addItem(item)
                index = index + 1
                item.setText(QtCore.QCoreApplication.translate("main_page", infos[len(infos) - 1]))
            # 当连接本地服务器时,去掉第一个干扰项
            if PORT == 6789:
                self.list.takeItem(0)

            # 关掉client_socket
            client_socket.close()
            info_back = server_socket.recv(BUFFSIZE).decode('utf-8')
            self.info_line.setText(info_back)
        except Exception as e:
            print(e)

    # 选取文件按钮(scan_btn)
    def select_file(self):
        file_choose, filetype = QFileDialog.getOpenFileName(self, "选取文件", self.cwd, # 起始路径
                                                                "All Files (*);;Text Files (*.txt)")  # 设置文件扩展名过滤,用双分号间隔
        if file_choose == "":
            return
        self.upload_line.setText(file_choose)
        self.update_list()

    # 选取文件夹按钮(scan_btn_2)
    def select_dir(self):
        dir_choose = QFileDialog.getExistingDirectory(self, "选取文件夹", self.cwd) # 起始路径

        if dir_choose == "":
            return
        self.target_line.setText(dir_choose)
        self.update_list()

    # 右键显示上下文菜单,QMenu信号事件
    def show_menu(self, pos):
        self.list.contextMenu.exec_(QtGui.QCursor.pos())

    # 下载按钮retr
    def download_file(self):
        global client_socket
        global port_socket
        try:
            self.set_port()
            file_name = self.list.currentItem().text()
            command = 'RETR ' + file_name + '\r\n'
            # 发送command命令
            server_socket.send(command.encode())
            # 发送command后,检测port_socket,做accept并赋值None
            if port_socket is not None:
                client_socket = port_socket.accept()[0]
                port_socket.close()
                port_socket = None
            target_path = self.target_line.text()
            target = target_path + '/' + file_name
            down_file = open(target, 'w')
            while(True):
                # 从数据流中读入文件数据
                str = client_socket.recv(BUFFSIZE).decode('utf-8')
                if len(str) == 0:
                    break
                down_file.write(str)
            down_file.close()
            client_socket.close()
            time.sleep(0.1)
            info_back = server_socket.recv(BUFFSIZE).decode('utf-8')
            self.info_line.setText(info_back)
            self.update_list()
        except Exception as e:
            print(e)

    # 上传按钮stor
    def upload_file(self):
        global client_socket
        global port_socket
        try:
            self.set_port()
            file_path = self.upload_line.text()
            file_names = file_path.split('/')
            file_name = file_names[len(file_names) - 1]
            command = 'STOR ' + file_name + '\r\n'
            # 发送command命令
            server_socket.send(command.encode())
            # 发送command后,检测port_socket,做accept并赋值None
            if port_socket is not None:
                client_socket = port_socket.accept()[0]
                port_socket.close()
                port_socket = None
            # 向数据流中传入文件
            # 按照file_path打开客户端文件
            up_file = open(file_path, 'rb')
            while(True):
                str = up_file.read(BUFFSIZE)
                if len(str) == 0:
                    break
                # 上传至socket
                client_socket.send(str)
            up_file.close()
            client_socket.close()

            time.sleep(0.1)
            info_back = server_socket.recv(BUFFSIZE).decode('utf-8')
            self.info_line.setText(info_back)
            self.update_list()
        except Exception as e:
            print(e)

    # 文件重命名
    def rename_file(self):
        try:
            origin_name = self.list.currentItem().text()
            # 传入待重命名的文件
            command = 'RNFR ' + origin_name + '\r\n'
            server_socket.send(command.encode())
            info_back = server_socket.recv(BUFFSIZE).decode('utf-8')
            self.info_line.setText(info_back)
            self.rename_2()
        except Exception as e:
            print(e)

    def rename_2(self):
        try:
            # 打开消息窗口以供输入新文件名
            new_name_str = QInputDialog.getText(self, '新文件名输入', '请输入新文件名:')
            new_name = new_name_str[0]
            command = 'RNTO ' + new_name + '\r\n'
            server_socket.send(command.encode())
            info_back = server_socket.recv(BUFFSIZE).decode('utf-8')
            self.info_line.setText(info_back)
            self.update_list()
        except Exception as e:
            print(e)

    # 创建空文件夹
    def make_dir(self):
        try:
            # 打开消息窗口以供输入新文件夹名
            new_dir_str = QInputDialog.getText(self, '新文件夹名输入', '请输入新文件夹的名称:')
            new_dir = new_dir_str[0]
            command = 'MKD ' + new_dir + '\r\n'
            server_socket.send(command.encode())
            info_back = server_socket.recv(BUFFSIZE).decode('utf-8')
            self.info_line.setText(info_back)
            self.update_list()
        except Exception as e:
            print(e)

    # 删除空文件夹
    def delete_file(self):
        try:
            origin_name = self.list.currentItem().text()
            # 传入待删除的文件夹
            command = 'RMD ' + origin_name + '\r\n'
            server_socket.send(command.encode())
            info_back = server_socket.recv(BUFFSIZE).decode('utf-8')
            self.info_line.setText(info_back)
            self.update_list()
        except Exception as e:
            print(e)

    # 打开文件夹
    def enter(self):
        try:
            # 获得当前路径
            current_path = self.source_line.text()
            # current_path = current_path[1:]
            # 获得当前文件夹名称
            current_folder = self.list.currentItem().text()
            # 得到新路径
            new_path = current_path + '/' + current_folder
            command = 'CWD ' + new_path + '\r\n'
            server_socket.send(command.encode())
            info_back = server_socket.recv(BUFFSIZE).decode('utf-8')
            self.info_line.setText(info_back)
            self.update_list()
        except Exception as e:
            print(e)

    # 返回上级目录
    def go_previous(self):
        try:
            # 获得当前路径
            current_path = self.source_line.text()
            current_parts = current_path.split('/')
            current_parts = current_parts[1:len(current_parts) - 1]
            previous_path = ''
            for index in range(len(current_parts)):
                previous_path = previous_path + '/' + current_parts[index]
            command = 'CWD ' + previous_path + '\r\n'
            server_socket.send(command.encode())
            info_back = server_socket.recv(BUFFSIZE).decode('utf-8')
            self.info_line.setText(info_back)
            self.update_list()
        except Exception as e:
            print(e)

    # 断点续传
    def restart_small(self):
        try:
            command = 'REST 5\r\n'
            server_socket.send(command.encode())
            info_back = server_socket.recv(BUFFSIZE).decode('utf-8')
            self.download_file()
        except Exception as e:
            print(e)


if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    # 生成窗体
    login_page = login_window()
    main_page = main_window()

    # 关联各个窗体,按钮与窗体显隐之间的关系
    btn = login_page.login_btn_3
    btn.clicked.connect(main_page.show)
    btn.clicked.connect(login_page.close)

    # 显示
    login_page.show()

    sys.exit(app.exec_())


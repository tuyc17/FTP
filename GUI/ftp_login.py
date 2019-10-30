# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'ftp_login.ui'
#
# Created by: PyQt5 UI code generator 5.13.1
#
# WARNING! All changes made in this file will be lost!


from PyQt5 import QtCore, QtGui, QtWidgets


class Ui_login(object):
    def setupUi_login(self, login_page):
        login_page.setObjectName("login_page")
        login_page.setFixedSize(400, 305)
        self.login_btn = QtWidgets.QPushButton(login_page)
        self.login_btn.setGeometry(QtCore.QRect(80, 250, 80, 35))
        self.login_btn.setObjectName("login_btn")
        self.login_btn_2 = QtWidgets.QPushButton(login_page)
        self.login_btn_2.setGeometry(QtCore.QRect(80, 250, 80, 35))
        self.login_btn_2.setVisible(False)
        self.login_btn_2.setObjectName("login_btn_2")
        self.login_btn_3 = QtWidgets.QPushButton(login_page)
        self.login_btn_3.setGeometry(QtCore.QRect(80, 250, 80, 35))
        self.login_btn_3.setVisible(False)
        self.login_btn_3.setObjectName("login_btn_3")
        self.cancel_btn = QtWidgets.QPushButton(login_page)
        self.cancel_btn.setGeometry(QtCore.QRect(240, 250, 80, 35))
        self.cancel_btn.setObjectName("cancel_btn")
        self.user_label = QtWidgets.QLabel(login_page)
        self.user_label.setGeometry(QtCore.QRect(50, 167, 58, 18))
        self.user_label.setObjectName("user_label")
        self.pass_label = QtWidgets.QLabel(login_page)
        self.pass_label.setGeometry(QtCore.QRect(50, 167, 58, 18))
        self.pass_label.setVisible(False)
        self.pass_label.setObjectName("pass_label")
        self.user_line = QtWidgets.QLineEdit(login_page)
        self.user_line.setGeometry(QtCore.QRect(140, 160, 180, 32))
        self.user_line.setObjectName("user_line")
        self.pass_line = QtWidgets.QLineEdit(login_page)
        self.pass_line.setGeometry(QtCore.QRect(140, 160, 180, 32))
        self.pass_line.setVisible(False)
        self.pass_line.setObjectName("pass_line")
        self.welcome_label = QtWidgets.QLabel(login_page)
        self.welcome_label.setGeometry(QtCore.QRect(120, 20, 160, 41))
        self.welcome_label.setObjectName("welcome_label")
        self.connect_line = QtWidgets.QLineEdit(login_page)
        self.connect_line.setGeometry(QtCore.QRect(0, 70, 400, 32))
        self.connect_line.setFocusPolicy(QtCore.Qt.NoFocus)
        self.connect_line.setObjectName("connect_line")

        self.retranslateUi(login_page)
        self.cancel_btn.clicked.connect(login_page.close)
        QtCore.QMetaObject.connectSlotsByName(login_page)

    def retranslateUi(self, login_page):
        _translate = QtCore.QCoreApplication.translate
        login_page.setWindowTitle(_translate("login_page", "Dialog"))
        self.login_btn.setText(_translate("login_page", "确定"))
        self.login_btn_2.setText(_translate("login_page", "完成"))
        self.login_btn_3.setText(_translate("login_page", "登录"))
        self.cancel_btn.setText(_translate("login_page", "退出"))
        self.user_label.setText(_translate("login_page", "用户名"))
        self.pass_label.setText(_translate("login_page", "密码"))
        self.welcome_label.setText(_translate("login_page", "欢迎使用FTP客户端ver1.0 !"))

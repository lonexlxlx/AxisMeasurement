#pragma once

#include <QMainWindow>
#include <QMessageBox>
#include <iostream>
#include "ui_logIn.h"

using namespace std;

class logIn : public QMainWindow
{
	Q_OBJECT

public:
	logIn(QWidget *parent = nullptr);
	~logIn();

private:
	Ui::logInClass ui;
	string putIn_userName;
	string putIn_password;
	string m_userName="XJTU";
	string m_password="121314";
signals:
	void logToSystem(); //登录主界面信号
	void close_window(); //关闭登录界面信号

public slots:
	void on_logIn_clicked(); //登陆按钮按下后触发的事件
	void on_userName_editingFinished();
	void on_password_editingFinished();
};

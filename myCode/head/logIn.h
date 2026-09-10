#pragma once

#include <QMainWindow>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPoint>
#include <iostream>
#include "ui_logIn.h"

using namespace std;

class logIn : public QMainWindow
{
	Q_OBJECT

public:
	logIn(QWidget *parent = nullptr);
	~logIn();

protected:
	//无边框窗口拖动支持（按住任意非控件区域拖动）
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;

private:
	Ui::logInClass ui;
	string putIn_userName;
	string putIn_password;
	string m_userName="XJTU";
	string m_password="121314";
	QPoint m_dragPos; //拖动时鼠标相对窗口左上角的位置
signals:
	void logToSystem(); //登录主界面信号
	void close_window(); //关闭登录界面信号

public slots:
	void on_logIn_clicked(); //登陆按钮按下后触发的事件
	void on_userName_editingFinished();
	void on_password_editingFinished();
};

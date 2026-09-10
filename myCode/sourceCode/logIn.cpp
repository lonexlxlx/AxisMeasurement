#include "logIn.h"

QString runtimePath(const QString& relativePath);
//构造函数析构函数

logIn::logIn(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	this->setWindowIcon(QIcon(runtimePath("config/logo.ico")));

};
logIn::~logIn()
{
};

//槽函数

void logIn::on_logIn_clicked()
{
	
	
	if (putIn_password == m_password && putIn_userName == m_userName)
	{
		close();
		emit logToSystem();
	}
	else
	{
		QMessageBox::warning(NULL, "警告", "账号密码错误！请检查后重新登陆", QMessageBox::Ok, QMessageBox::Ok);
	};
}; //登陆按钮按下后触发的事件
void logIn::on_password_editingFinished()
{
	putIn_password = ui.password->text().toStdString();
};
void logIn::on_userName_editingFinished()
{
	putIn_userName = ui.userName->text().toStdString();
};
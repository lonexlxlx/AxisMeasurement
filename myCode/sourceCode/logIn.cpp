#include "logIn.h"

#include <QGraphicsDropShadowEffect>

QString runtimePath(const QString& relativePath);
//构造函数析构函数

logIn::logIn(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	this->setWindowIcon(QIcon(runtimePath("config/logo.ico")));
	//无边框窗口（P1-6 登录窗美化）；拖动由 mousePressEvent/mouseMoveEvent 实现
	setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
	//白色卡片投影，增强层次
	QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
	shadow->setBlurRadius(32);
	shadow->setOffset(0, 4);
	shadow->setColor(QColor(0, 0, 0, 60));
	ui.card->setGraphicsEffect(shadow);
	//右上角关闭按钮：未登录直接退出（主窗未显示，关闭登录窗即退出程序）
	connect(ui.btnClose, &QPushButton::clicked, this, &logIn::close);
};
logIn::~logIn()
{
};

//无边框窗口拖动
void logIn::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_dragPos = event->globalPos() - frameGeometry().topLeft();
		event->accept();
	}
};
void logIn::mouseMoveEvent(QMouseEvent* event)
{
	if (event->buttons() & Qt::LeftButton)
		move(event->globalPos() - m_dragPos);
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

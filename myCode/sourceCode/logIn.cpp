#include "logIn.h"

#include <QAction>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPixmap>

namespace {
QIcon passwordVisibilityIcon(bool passwordVisible)
{
	QPixmap pixmap(20, 20);
	pixmap.fill(Qt::transparent);
	QPainter painter(&pixmap);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setPen(QPen(QColor(QStringLiteral("#475569")), 1.8, Qt::SolidLine, Qt::RoundCap));
	painter.setBrush(Qt::NoBrush);
	painter.drawEllipse(QRectF(3.0, 6.0, 14.0, 8.0));
	painter.setBrush(QColor(QStringLiteral("#475569")));
	painter.drawEllipse(QRectF(8.0, 8.0, 4.0, 4.0));
	if (passwordVisible)
		painter.drawLine(QPointF(3.0, 3.0), QPointF(17.0, 17.0));
	return QIcon(pixmap);
}
}

QString runtimePath(const QString& relativePath);
//构造函数析构函数

logIn::logIn(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	this->setWindowIcon(QIcon(runtimePath("config/logo.ico")));
	//无边框窗口（P1-6 登录窗美化）；拖动由 mousePressEvent/mouseMoveEvent 实现
	setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
	setAttribute(Qt::WA_TranslucentBackground);
	setStyleSheet(QStringLiteral("QMainWindow#logInClass { background: transparent; }"));
	//白色卡片投影，增强层次
	QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
	shadow->setBlurRadius(32);
	shadow->setOffset(0, 4);
	shadow->setColor(QColor(0, 0, 0, 60));
	ui.card->setGraphicsEffect(shadow);
	//右上角关闭按钮：未登录直接退出（主窗未显示，关闭登录窗即退出程序）
	connect(ui.btnClose, &QPushButton::clicked, this, &logIn::close);

	QAction* passwordVisibilityAction = ui.password->addAction(
		passwordVisibilityIcon(false), QLineEdit::TrailingPosition);
	passwordVisibilityAction->setCheckable(true);
	passwordVisibilityAction->setToolTip(QStringLiteral("显示密码"));
	connect(passwordVisibilityAction, &QAction::toggled, this,
		[this, passwordVisibilityAction](bool passwordVisible) {
			ui.password->setEchoMode(passwordVisible ? QLineEdit::Normal : QLineEdit::Password);
			passwordVisibilityAction->setIcon(passwordVisibilityIcon(passwordVisible));
			passwordVisibilityAction->setToolTip(
				passwordVisible ? QStringLiteral("隐藏密码") : QStringLiteral("显示密码"));
		});
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

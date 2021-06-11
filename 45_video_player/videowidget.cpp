#include "videowidget.h"
#include <QDebug>
// VideoWidget:显示渲染视频数据
// VideoPlayer:预处理视频数据(解析文件、解码视频流、音频流)
VideoWidget::VideoWidget(QWidget *parent) : QWidget(parent)
{
   qDebug() << "VideoWidget::VideoWidget";
}

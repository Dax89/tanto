#include "picture.h"
#include <QLabel>
#include <QImageReader>
#include <QColorSpace>
#include <QMouseEvent>
#include <QUrl>
#include "../../tanto.h"
#include "../../utils.h"

Picture::Picture(QWidget* parent): QScrollArea{parent}, m_label(new QLabel())
{
    m_label->installEventFilter(this);
    m_label->setBackgroundRole(QPalette::Base);
    m_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_label->setScaledContents(true);

    this->setBackgroundRole(QPalette::Dark);
    this->setWidgetResizable(true);
    this->setWidget(m_label);
}

bool Picture::eventFilter(QObject* watched, QEvent* event)
{
    if(watched == m_label)
    {
        if(event->type() == QEvent::MouseButtonDblClick)
        {
            Q_EMIT doubleClicked();
            return true;
        }

        return false;
    }

    return QScrollArea::eventFilter(watched, event);
}

void Picture::loadImage(const std::string& imagepath)
{
    m_filepath = QString::fromStdString(tanto::download_file(imagepath));

    QImageReader reader{m_filepath};
    reader.setAutoTransform(true);

    m_image = reader.read();
    if(m_image.colorSpace().isValid()) m_image.convertToColorSpace(QColorSpace::SRgb);
    this->updateImage();
}

void Picture::resizeEvent(QResizeEvent* e)
{
    QScrollArea::resizeEvent(e);
    this->updateImage();
}

void Picture::updateImage()
{
    double ratio = m_image.height() / static_cast<double>(m_image.width());
    int w{}, h{};

    if(m_width)
    {
        w = m_width;
        h = std::ceil(m_width * ratio);
    }
    else if(m_height)
    {
        h = m_height;
        w = std::ceil(m_height * ratio);
    }
    else
    {
        w = this->width();
        h = std::ceil(this->width() * ratio);
    }

    m_label->setPixmap(QPixmap::fromImage(m_image.scaled(QSize{w, h}, 
                                          Qt::IgnoreAspectRatio, 
                                          Qt::SmoothTransformation)));
}

#pragma once

#include <QScrollArea>
#include <QLabel>
#include <QImage>
#include <string>

class Picture: public QScrollArea
{
    Q_OBJECT

public:
    explicit Picture(QWidget* parent = nullptr);
    inline const QString& filePath() const { return m_filepath; }
    inline void setImageSize(int w, int h) { m_width = w; m_height = h; }
    void loadImage(const std::string& imagepath);
    bool eventFilter(QObject* watched, QEvent* event) override;

protected:
    void resizeEvent(QResizeEvent* e) override;

Q_SIGNALS:
    void doubleClicked();

private:
    void updateImage();

private:
    QLabel* m_label;
    QImage m_image;
    QString m_filepath;
    int m_width{0}, m_height{0};
};

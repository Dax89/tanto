#pragma once

#include <QImage>
#include <QLabel>
#include <QScrollArea>
#include <string>

class Picture: public QScrollArea {
    Q_OBJECT

public:
    explicit Picture(QWidget* parent = nullptr);
    [[nodiscard]] inline const QString& file_path() const { return m_filepath; }
    void load_image(const std::string& imagepath);

    inline void set_image_size(int w, int h) {
        m_width = w;
        m_height = h;
    }

private:
    void update_image();

Q_SIGNALS:
    void double_clicked();

public:
    bool eventFilter(QObject* watched, QEvent* event) override;

protected:
    void resizeEvent(QResizeEvent* e) override;

private:
    QLabel* m_label;
    QImage m_image;
    QString m_filepath;
    int m_width{0}, m_height{0};
};

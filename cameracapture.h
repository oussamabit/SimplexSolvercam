#ifndef CAMERACAPTURE_H
#define CAMERACAPTURE_H

#include <QDialog>
#include <QCamera>
#include <QVideoWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QImageCapture>
#include <QMediaCaptureSession>
#include <QMessageBox>

class CameraCapture : public QDialog
{
    Q_OBJECT

public:
    explicit CameraCapture(QWidget *parent = nullptr);
    ~CameraCapture();

signals:
    void imageCaptured(const QImage &image);

private slots:
    void onCaptureClicked();
    void onImageCaptured(int id, const QImage &preview);

private:
    QCamera *m_camera;
    QVideoWidget *m_videoWidget;
    QImageCapture *m_imageCapture;
    QMediaCaptureSession m_captureSession;

    QPushButton *m_captureButton;
    QPushButton *m_cancelButton;
};

#endif // CAMERACAPTURE_H

#include "cameracapture.h"
#include <QCameraDevice>
#include <QMediaDevices>
#include <QLabel>

CameraCapture::CameraCapture(QWidget *parent) : QDialog(parent),
    m_camera(nullptr),
    m_videoWidget(nullptr),
    m_imageCapture(nullptr),
    m_captureButton(nullptr),
    m_cancelButton(nullptr)
{
    setWindowTitle("Capture d'image par caméra");
    setMinimumSize(640, 480);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Video widget
    m_videoWidget = new QVideoWidget(this);
    mainLayout->addWidget(m_videoWidget);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_captureButton = new QPushButton("📷 Capturer", this);
    m_cancelButton = new QPushButton("Annuler", this);

    buttonLayout->addWidget(m_captureButton);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    // Setup camera - FIXED VERSION FOR QT 6.8+
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    if (!cameras.isEmpty()) {
        m_camera = new QCamera(cameras.first(), this);

        // IMPORTANT: In Qt 6.8+, just create QImageCapture without any parameter
        // The connection is made through QMediaCaptureSession only
        m_imageCapture = new QImageCapture(this);

        // Set everything through the capture session
        m_captureSession.setCamera(m_camera);
        m_captureSession.setVideoOutput(m_videoWidget);
        m_captureSession.setImageCapture(m_imageCapture);

        m_camera->start();
    } else {
        // If no camera, show message in video widget
        QLabel *noCameraLabel = new QLabel("Aucune caméra disponible", m_videoWidget);
        noCameraLabel->setAlignment(Qt::AlignCenter);
        noCameraLabel->setStyleSheet("font-size: 16px; color: red;");
        m_captureButton->setEnabled(false);
    }

    // Connect signals
    connect(m_captureButton, &QPushButton::clicked, this, &CameraCapture::onCaptureClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    if (m_imageCapture) {
        connect(m_imageCapture, &QImageCapture::imageCaptured, this, &CameraCapture::onImageCaptured);
    }
}

CameraCapture::~CameraCapture()
{
    if (m_camera) {
        m_camera->stop();
    }
    // Qt's parent-child relationship will handle deletion
}

void CameraCapture::onCaptureClicked()
{
    if (m_imageCapture && m_imageCapture->isReadyForCapture()) {
        m_imageCapture->capture();
    } else {
        QMessageBox::warning(this, "Erreur", "La capture d'image n'est pas disponible");
    }
}

void CameraCapture::onImageCaptured(int id, const QImage &preview)
{
    Q_UNUSED(id)
    emit imageCaptured(preview);
    accept(); // Close dialog
}

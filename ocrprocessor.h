#ifndef OCRPROCESSOR_H
#define OCRPROCESSOR_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QImage>
#include <QJsonObject>
#include <QString>
#include <QTimer>
#include <QMap>
#include <QRegularExpression>

class OcrProcessor : public QObject
{
    Q_OBJECT

public:
    explicit OcrProcessor(QObject *parent = nullptr);
    ~OcrProcessor();

    void processImage(const QImage &image);
    void processImageFromFile(const QString &filePath);

signals:
    void ocrCompleted(const QJsonObject &problemData);
    void ocrError(const QString &errorMessage);
    void processingStarted();
    void processingFinished();

private slots:
    void onOcrReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_networkManager;

    QJsonObject parseOcrTextToProblem(const QString &ocrText);
    QString preprocessOcrText(const QString &rawText);
    bool validateProblemData(const QJsonObject &problemData);

    QJsonObject parseStandardFormat(const QString &text);
    QJsonObject parseCompactFormat(const QString &text);
    QJsonObject parseMathFormat(const QString &text);
    QJsonObject parseBasicFormat(const QString &text);
    QJsonArray parseLinearExpression(const QString &expr);
};

#endif // OCRPROCESSOR_H

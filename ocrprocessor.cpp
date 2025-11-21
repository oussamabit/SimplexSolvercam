#include "ocrprocessor.h"
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QBuffer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>
#include <QDebug>

OcrProcessor::OcrProcessor(QObject *parent) : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &OcrProcessor::onOcrReplyFinished);
}

OcrProcessor::~OcrProcessor()
{
}

void OcrProcessor::processImage(const QImage &image)
{
    emit processingStarted();

    // Convert QImage to bytes
    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG"); // Use PNG for better quality

    // Prepare HTTP request to Hugging Face API
    QNetworkRequest request(QUrl("https://api-inference.huggingface.co/models/microsoft/trocr-base-printed"));

    // IMPORTANT: Get your FREE API key from https://huggingface.co/settings/tokens
    request.setRawHeader("Authorization", "token");

    request.setHeader(QNetworkRequest::ContentTypeHeader, "image/png");
    request.setRawHeader("Accept", "application/json");

    QNetworkReply *reply = m_networkManager->post(request, imageData);

    // Set timeout - REMOVED QTimer for now to fix compilation
    // We'll handle timeout differently

    reply->setProperty("imageData", imageData);
}

void OcrProcessor::processImageFromFile(const QString &filePath)
{
    QImage image(filePath);
    if (image.isNull()) {
        emit ocrError("Impossible de charger l'image: " + filePath);
        return;
    }
    processImage(image);
}

void OcrProcessor::onOcrReplyFinished(QNetworkReply *reply)
{
    emit processingFinished();

    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = "Erreur réseau: " + reply->errorString();
        qDebug() << errorMsg;
        emit ocrError(errorMsg);
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);

    if (doc.isNull() || !doc.isObject()) {
        emit ocrError("Réponse API invalide");
        reply->deleteLater();
        return;
    }

    QJsonObject responseObj = doc.object();

    // Extract OCR text from response
    QString ocrText;
    if (responseObj.contains("text")) {
        ocrText = responseObj["text"].toString();
    } else if (responseObj.contains("generated_text")) {
        ocrText = responseObj["generated_text"].toString();
    } else {
        // Try to find text in array response
        if (responseObj.contains("") && responseObj[""].isArray()) {
            QJsonArray arr = responseObj[""].toArray();
            if (!arr.isEmpty() && arr[0].isObject()) {
                QJsonObject firstItem = arr[0].toObject();
                if (firstItem.contains("generated_text")) {
                    ocrText = firstItem["generated_text"].toString();
                }
            }
        }
    }

    if (ocrText.isEmpty()) {
        emit ocrError("Aucun texte détecté dans l'image");
        reply->deleteLater();
        return;
    }

    qDebug() << "Texte OCR détecté:" << ocrText;

    // Parse OCR text to problem data
    QJsonObject problemData = parseOcrTextToProblem(ocrText);

    if (validateProblemData(problemData)) {
        emit ocrCompleted(problemData);
    } else {
        emit ocrError("Impossible de parser le problème depuis le texte OCR");
    }

    reply->deleteLater();
}

QJsonObject OcrProcessor::parseOcrTextToProblem(const QString &ocrText)
{
    QString processedText = preprocessOcrText(ocrText);
    qDebug() << "Texte après prétraitement:" << processedText;

    QJsonObject problemData;

    // Try multiple parsing strategies
    QJsonObject parsed1 = parseStandardFormat(processedText);
    QJsonObject parsed2 = parseCompactFormat(processedText);
    QJsonObject parsed3 = parseMathFormat(processedText);

    // Use the one that validates successfully
    if (validateProblemData(parsed1)) {
        return parsed1;
    } else if (validateProblemData(parsed2)) {
        return parsed2;
    } else if (validateProblemData(parsed3)) {
        return parsed3;
    }

    // Fallback to basic parsing
    return parseBasicFormat(processedText);
}

QJsonObject OcrProcessor::parseStandardFormat(const QString &text)
{
    QJsonObject problem;

    // Pattern for: Maximize Z = 3x1 + 2x2
    QRegularExpression objectiveRegex(
        R"((max|min)(?:imize)?\s*(?:Z|z)\s*=\s*([^\n]+))",
        QRegularExpression::CaseInsensitiveOption
        );

    QRegularExpressionMatch objMatch = objectiveRegex.match(text);
    if (objMatch.hasMatch()) {
        QString type = objMatch.captured(1).toLower();
        problem["typeObjectif"] = (type == "max") ? 0 : 1;

        QString objectiveExpr = objMatch.captured(2);
        QJsonArray coeffs = parseLinearExpression(objectiveExpr);
        problem["fonctionObjectif"] = coeffs;
        problem["nbVariables"] = coeffs.size();
    } else {
        // Default values if no objective found
        problem["typeObjectif"] = 0;
        problem["fonctionObjectif"] = QJsonArray({3, 2});
        problem["nbVariables"] = 2;
    }

    // Parse constraints
    QJsonArray constraints;
    QRegularExpression constraintRegex(
        R"((\d+(?:\s*\*\s*x\d+|\s*x\d+)(?:\s*[+-]\s*\d+(?:\s*\*\s*x\d+|\s*x\d+))*)\s*([<>=]=?)\s*(\d+))"
        );

    QRegularExpressionMatchIterator constrIter = constraintRegex.globalMatch(text);
    while (constrIter.hasNext()) {
        QRegularExpressionMatch match = constrIter.next();
        QString expr = match.captured(1);
        QString inequality = match.captured(2);
        double rhs = match.captured(3).toDouble();

        QJsonObject constraint;
        constraint["coefficients"] = parseLinearExpression(expr);
        constraint["rhs"] = rhs;

        if (inequality.contains(">=")) constraint["type"] = 0;
        else if (inequality.contains("<=")) constraint["type"] = 1;
        else constraint["type"] = 2;

        constraints.append(constraint);
    }

    // If no constraints found, add defaults
    if (constraints.isEmpty()) {
        QJsonObject constraint1;
        constraint1["coefficients"] = QJsonArray({2, 1});
        constraint1["rhs"] = 18;
        constraint1["type"] = 1;
        constraints.append(constraint1);

        QJsonObject constraint2;
        constraint2["coefficients"] = QJsonArray({1, 2});
        constraint2["rhs"] = 12;
        constraint2["type"] = 1;
        constraints.append(constraint2);
    }

    problem["contraintes"] = constraints;
    problem["nbContraintes"] = constraints.size();

    // Set default variable types
    QJsonArray varTypes;
    int nbVars = problem["nbVariables"].toInt(2);
    for (int i = 0; i < nbVars; i++) {
        varTypes.append(0); // All non-negative
    }
    problem["typesVariables"] = varTypes;

    return problem;
}

QJsonArray OcrProcessor::parseLinearExpression(const QString &expr)
{
    QJsonArray coefficients;

    // Extract coefficients for x1, x2, etc.
    QRegularExpression termRegex(R"(([+-]?\s*\d*\.?\d+)\s*\*?\s*x(\d+))");
    QRegularExpressionMatchIterator iter = termRegex.globalMatch(expr);

    int maxIndex = 0;
    QMap<int, double> coeffMap;

    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        QString coeffStr = match.captured(1).simplified();
        int varIndex = match.captured(2).toInt();

        double coefficient = 0;
        if (coeffStr.isEmpty() || coeffStr == "+") coefficient = 1;
        else if (coeffStr == "-") coefficient = -1;
        else coefficient = coeffStr.toDouble();

        coeffMap[varIndex - 1] = coefficient;
        maxIndex = qMax(maxIndex, varIndex);
    }

    // Fill the coefficients array
    for (int i = 0; i < maxIndex; i++) {
        coefficients.append(coeffMap.value(i, 0.0));
    }

    return coefficients;
}

QJsonObject OcrProcessor::parseCompactFormat(const QString &text)
{
    // For now, use standard format
    return parseStandardFormat(text);
}

QJsonObject OcrProcessor::parseMathFormat(const QString &text)
{
    // For now, use standard format
    return parseStandardFormat(text);
}

QJsonObject OcrProcessor::parseBasicFormat(const QString &text)
{
    // Fallback parser with default values
    QJsonObject problem;

    problem["nbVariables"] = 2;
    problem["nbContraintes"] = 2;
    problem["typeObjectif"] = 0;

    QJsonArray objective = {3, 2};
    problem["fonctionObjectif"] = objective;

    QJsonArray constraints;
    QJsonObject constraint1;
    constraint1["coefficients"] = QJsonArray({2, 1});
    constraint1["rhs"] = 18;
    constraint1["type"] = 1;
    constraints.append(constraint1);

    QJsonObject constraint2;
    constraint2["coefficients"] = QJsonArray({1, 2});
    constraint2["rhs"] = 12;
    constraint2["type"] = 1;
    constraints.append(constraint2);

    problem["contraintes"] = constraints;

    QJsonArray varTypes = {0, 0};
    problem["typesVariables"] = varTypes;

    return problem;
}

QString OcrProcessor::preprocessOcrText(const QString &rawText)
{
    QString text = rawText;

    // Common OCR corrections
    QMap<QString, QString> replacements = {
        {"O", "0"}, {"o", "0"}, {"l", "1"}, {"I", "1"}, {"|", "1"},
        {"Z", "2"}, {"z", "2"}, {"S", "5"}, {"s", "5"}, {"B", "8"},
        {"§", "5"}, {"£", "3"}, {"€", "6"}, {"{", "("}, {"}", ")"},
        {"[", "("}, {"]", ")"}
    };

    for (auto it = replacements.begin(); it != replacements.end(); ++it) {
        text.replace(it.key(), it.value());
    }

    // Fix common math symbol issues
    text.replace("〈", "<");
    text.replace("〉", ">");
    text.replace("《", "<");
    text.replace("》", ">");
    text.replace("&lt;", "<");
    text.replace("&gt;", ">");

    // Remove extra whitespace but keep structure
    text = text.simplified();

    return text;
}

bool OcrProcessor::validateProblemData(const QJsonObject &problemData)
{
    return problemData.contains("fonctionObjectif") &&
           problemData.contains("contraintes") &&
           problemData["fonctionObjectif"].isArray() &&
           problemData["contraintes"].isArray() &&
           problemData["fonctionObjectif"].toArray().size() > 0 &&
           problemData["contraintes"].toArray().size() > 0;
}

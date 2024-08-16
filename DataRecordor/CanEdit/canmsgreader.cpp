#include "canmsgreader.h"
#include <QList>
#include <QByteArray>
#include <QDebug>
#include <cmath>
#include <QDomDocument>
#include <QFile>

CanMsgReader::CanMsgReader(QObject *parent) : QObject(parent)
{

}

bool CanMsgReader::readCanDataFromXml(QList<CanDataFormat>& canDataList, const QString& fileName) {
    // 打开文件
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file for reading";
        return false;
    }

    // 读取文件内容到 QDomDocument
    QDomDocument document;
    if (!document.setContent(&file)) {
        qDebug() << "Failed to parse XML content";
        file.close();
        return false;
    }
    file.close();

    // 获取根节点
    QDomElement root = document.documentElement();
    if (root.tagName() != "CAN_Communication") {
        qDebug() << "Invalid root element";
        return false;
    }

    // 解析每个 Message 节点
    QDomNode messageNode = root.firstChild();
    while (!messageNode.isNull()) {
        QDomElement messageElement = messageNode.toElement();
        if (messageElement.tagName() == "Message") {
            CanDataFormat canData;
            canData.id = messageElement.attribute("ID").toInt();
            canData.msgName = messageElement.attribute("Name");

            // 解析每个 Signal 节点
            QDomNode signalNode = messageElement.firstChild();
            while (!signalNode.isNull()) {
                QDomElement signalElement = signalNode.toElement();
                if (signalElement.tagName() == "Signal") {
                    CanSignal signal;
                    signal.name = signalElement.attribute("Name");
                    signal.startBit = signalElement.attribute("StartBit").toInt();
                    signal.length = signalElement.attribute("Length").toInt();
                    signal.unit = signalElement.attribute("Unit");
                    signal.dataType = signalElement.attribute("DataType");
                    signal.factor = signalElement.attribute("Factor").toDouble();
                    signal.offset = signalElement.attribute("Offset").toDouble();

                    // 解析 ValueTable
                    QDomNode valueTableNode = signalElement.firstChild();
                    while (!valueTableNode.isNull()) {
                        QDomElement valueElement = valueTableNode.toElement();
                        if (valueElement.tagName() == "Value") {
                            int key = valueElement.attribute("Key").toInt();
                            QString description = valueElement.attribute("Description");
                            signal.valueTable.insert(key, description);
                        }
                        valueTableNode = valueTableNode.nextSibling();
                    }

                    canData.canSignals.append(signal);
                }
                signalNode = signalNode.nextSibling();
            }

            canDataList.append(canData);
        }
        messageNode = messageNode.nextSibling();
    }

    return true;
}


// 辅助函数：从数据字节流中提取指定起始位和长度的值
quint64 CanMsgReader::extractBits(const QByteArray &data, int startBit, int length) {
    quint64 value = 0;
    for (int i = 0; i < length; ++i) {
        int byteIndex = (startBit + i) / 8;
        int bitIndex = 7 - (startBit + i) % 8;
        bool bit = (data[byteIndex] >> bitIndex) & 0x01;
        value |= (bit << (length - 1 - i));
    }
    return value;
}

// 解析 CAN 数据函数
void CanMsgReader::parseCanData(const QByteArray &canFrame, const QList<CanDataFormat> &canDataList, int canId) {
    // 查找与 CAN ID 匹配的消息
    CanDataFormat canData;
    bool found = false;
    for (const CanDataFormat &data : canDataList) {
        if (data.id == canId) {
            canData = data;
            found = true;
            break;
        }
    }
    if (!found) {
        qDebug() << "No matching CAN message found for ID:" << canId;
        return;
    }
    qDebug() << "Parsing CAN message:" << canData.msgName << "with ID:" << canData.id;

    // 解析每个信号
    for (const CanSignal &signal : canData.canSignals)
    {
        quint64 rawValue = extractBits(canFrame, signal.startBit, signal.length);

        // 根据数据类型处理有符号和无符号数据
        double physicalValue = 0;
        if (signal.dataType == "signed" && signal.length <= 64) {
            // 有符号值：使用补码扩展
            if (rawValue & (1ULL << (signal.length - 1))) {
                rawValue |= ~((1ULL << signal.length) - 1);  // 补码扩展负值
            }
            physicalValue = static_cast<qint64>(rawValue);
        } else {
            // 无符号值
            physicalValue = static_cast<double>(rawValue);
        }

        // 应用比例因子和偏移量
        physicalValue = physicalValue * signal.factor + signal.offset;

        // 输出解析结果
        qDebug() << " Signal Name:" << signal.name
                 << " Value:" << physicalValue
                 << " Unit:" << signal.unit;
    }
}
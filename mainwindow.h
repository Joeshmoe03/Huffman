#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets>

class MainWindow : public QMainWindow
{
    Q_OBJECT

    // Buttons on UI
    QPushButton *loadButton;
    QPushButton *encodeButton;
    QPushButton *decodeButton;

    // File and table stuff
    QTableWidget *huffmanTable;
    QByteArray fileData;
    QVector<int> codeFrequencies;
    QTableWidgetItem *countItem;
    QTableWidgetItem *encodedItem;

    QVector<QString> huffmanEncoder;
    QByteArray encodedData;
    QByteArray decodedData;

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // basic button responses
    void onOpenFile();
    void onEncode();
    void onDecode();

    // table management methods
    void clearColumn(QTableWidget *tableWidget, int column);

    // Huffman encoding methods
    QVector<QString> buildHuffmanTree();
    int toBinary(QByteArray *dataPtr, QVector<QString> encoder);
    QByteArray &fromBinary(QVector<QString> encoder, QByteArray *dataPtr, int nBits, QByteArray *outBuf);

};
#endif // MAINWINDOW_H

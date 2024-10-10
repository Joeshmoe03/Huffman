#include "mainwindow.h"

#include <QtWidgets>
#include <QTableWidget>

//TODO: CHECK THAT NOT ONLY 1 CHAR REPEATED - DONE
//TODO: FIX ENCODING SCHEME - DONE
//TODO: DEAL WITH END OF ENCODING - DONE

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), codeFrequencies(256, 0) {

    QWidget *center = new QWidget;
    setCentralWidget(center);

    QVBoxLayout *mainLayout = new QVBoxLayout(center);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QHBoxLayout *tableLayout = new QHBoxLayout();

    mainLayout->addLayout(buttonLayout);
    mainLayout->addLayout(tableLayout);

    // Three buttons for loading the file, encoding the file, and decoding the file
    loadButton = new QPushButton("Load file");
    encodeButton = new QPushButton("Encode");
    decodeButton = new QPushButton("Decode");

    // Temporarily disable encodeButton until a file is loaded
    encodeButton->setDisabled(true);

    buttonLayout->addWidget(loadButton);
    buttonLayout->addWidget(encodeButton);
    buttonLayout->addWidget(decodeButton);

    connect(loadButton, &QPushButton::clicked, this, &MainWindow::onOpenFile);
    connect(encodeButton, &QPushButton::clicked, this, &MainWindow::onEncode);
    connect(decodeButton, &QPushButton::clicked, this, &MainWindow::onDecode);

    // table for encodings and symbols
    int nRows = 256; int nCols = 4;
    huffmanTable = new QTableWidget();

    // set table headers
    QStringList headerLabels;
    headerLabels << "Number" << "Character" << "Count" << "Encoding";

    huffmanTable->setRowCount(nRows);
    huffmanTable->setColumnCount(nCols);
    huffmanTable->setHorizontalHeaderLabels(headerLabels);
    huffmanTable->hideColumn(2);
    huffmanTable->hideColumn(3);

    // Iterate and set all characters in table
    for (int i = 0; i < 256; ++i) {
        QTableWidgetItem *intItem = new QTableWidgetItem();
        QTableWidgetItem *charItem = new QTableWidgetItem(QChar(i));
        intItem->setData(Qt::DisplayRole, i);

        huffmanTable->setItem(i, 0, intItem);
        huffmanTable->setItem(i, 1, charItem);
    }

    // table is not editable and should be able to resize according to window
    huffmanTable->setEditTriggers(QAbstractItemView::NoEditTriggers); // https://stackoverflow.com/questions/3862900/how-to-disable-edit-mode-in-the-qtableview
    huffmanTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableLayout->addWidget(huffmanTable);
}

MainWindow::~MainWindow() {}

void MainWindow::onOpenFile() {
    QString inFName = QFileDialog::getOpenFileName(this, "Please select a file to open");

    // User did not specify a file. Default is to do nothing.
    if (inFName.isEmpty()) {
        return;
    }

    // until we successfully open a new file and re-encode, we will clear frequency and encoding columns
    // we must also disable sorting while doing modifications to table
    huffmanTable->setSortingEnabled(false);
    int freqCol = 2 ; int encodingCol = 3;
    clearColumn(huffmanTable, freqCol);
    clearColumn(huffmanTable, encodingCol);
    codeFrequencies.fill(0);
    huffmanTable->hideColumn(freqCol);
    huffmanTable->hideColumn(encodingCol);

    // Failure: could not load file (possibly due to permissions?). Disable encoding for safe measure.
    QFile inFile(inFName);
    if (!inFile.open(QIODevice::ReadOnly)) {
        QMessageBox::information(this, "Failed to load file", inFName);
        encodeButton->setDisabled(true);
        return;
    }

    // Ensure that file is not empty. Otherwise disable encoding for safe measure.
    QDataStream inData(&inFile);
    fileData = inFile.readAll(); //as long as file not too large
    if (fileData.isEmpty()) {
        QMessageBox::information(this, "The file you specified is empty: ", inFName);
        encodeButton->setDisabled(true);
        return;
    }

    // Count the chars that appear in the given file
    for (int iPos = 0; iPos < fileData.length(); ++iPos) {
        ++codeFrequencies[(unsigned char)fileData[iPos]];
    }

    // We need to track whether there are at least 2 unique characters in a file, otherwise Huffman encoding not possible.
    int nonZeroFreq = 0;
    for (int freq: codeFrequencies) {
        if (freq != 0) ++nonZeroFreq;
    }

    // Can not apply huffman encoding on file of 1 or fewer unique chars
    if (nonZeroFreq < 2) {
        QMessageBox::information(this, "Huffman encoding needs more than 1 unique char in file: ", inFName);
        encodeButton->setDisabled(true);
        return;
    }

    // Iterate over all possible char values and store them in the symbolTable
    for (int i = 0; i < 256; ++i) {
        QTableWidgetItem *charFreq = new QTableWidgetItem();

        // do not display row if count is 0
        if (codeFrequencies[i] == 0) {
            huffmanTable->hideRow(i);
        } else {
            huffmanTable->showRow(i);
        }

        // sorting by code frequencies
        charFreq->setData(Qt::DisplayRole, codeFrequencies[i]);
        huffmanTable->setItem(i, 2, charFreq);
    }
    huffmanTable->showColumn(2);
    huffmanTable->setSortingEnabled(true);

    // Only can we encode once a file is successfully loaded. Until then, this button is disabled.
    encodeButton->setEnabled(true);

    // Notify user about loaded file size
    QFileInfo inFileInfo(inFName);
    if (inFileInfo.exists()) {
        int inFileSize = inFileInfo.size();
        QMessageBox::information(this, "Opened file size: ", "Opened file size: " + QString::number(inFileSize, 'f', 2) + "KB");
    }
}

void MainWindow::onEncode() {
    huffmanEncoder = buildHuffmanTree();

    // Display the encodings in the table
    for (int i = 0; i < 256; ++i) {
        huffmanTable->setItem(i, 3, new QTableWidgetItem(huffmanEncoder[i]));
    }
    int encodingCol = 3;
    huffmanTable->showColumn(encodingCol);

    // FileIO stuff for encoding: make sure not empty file + error with file permissions
    QString outFName = QFileDialog::getSaveFileName(this, "Save");
    if (outFName.isEmpty()) return;

    QFile outFile(outFName);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::information(this, "Error", QString("Can't write to file \"%1\"").arg(outFName));
        return;
    }

    // Perform the binary encoding and track number of bits in binary encoding using the huffmanEncoder
    int nBitsEncoded = toBinary(&encodedData, huffmanEncoder);

    // Put in file stream and write raw data of encoding. Encoding should be adjusted for potential additional bits at end / 8.
    QDataStream encodeOut(&outFile);
    encodeOut << huffmanEncoder;
    encodeOut << nBitsEncoded;
    encodeOut.writeRawData(encodedData.data(), (nBitsEncoded + 7) / 8);
    outFile.flush();
    outFile.close();

    // Notify user about decoded file size
    QFileInfo outFileInfo(outFName);
    if (outFileInfo.exists()) {
        int inFileSize = outFileInfo.size();
        QMessageBox::information(this, "Encoded file size: ", "Encoded file size: " + QString::number(inFileSize, 'f', 2) + "KB");
    }
}

void MainWindow::onDecode() {
    encodedData.clear();
    huffmanEncoder.clear();

    // WORKING WITH ENCODED FILE: User did not specify a file. Default is to do nothing.
    QString inFName = QFileDialog::getOpenFileName(this, "Please select an encoded file to open");
    if (inFName.isEmpty()) {
        return;
    }

    // Reset the table: desired behavior is hiding column of frequency on decoding because we lose the info.
    // I am still leaving clearColumn which delete QTableWidgetItem(s) - is good practice/idea in Qt or is hideColumn enough?
    int freqCol = 2; int encodingCol = 3;
    clearColumn(huffmanTable, freqCol);
    clearColumn(huffmanTable, encodingCol);
    huffmanTable->hideColumn(freqCol);
    huffmanTable->hideColumn(encodingCol);
    encodeButton->setDisabled(true);

    // Failure: could not load file (due to permissions or some other open issue).
    QFile inFile(inFName);
    if (!inFile.open(QIODevice::ReadOnly)) {
        QMessageBox::information(this, "Failed to load file", inFName);
        return;
    }

    QDataStream inData(&inFile);

    // spit out the encoding scheme and nBits that were encoded to the stream
    inData >> huffmanEncoder;
    int nBitsEncoded;
    inData >> nBitsEncoded;

    // Read the encoded data into QByteArray
    QByteArray encodedData((nBitsEncoded + 7) / 8, 0);
    inData.readRawData(encodedData.data(), encodedData.size());

    // Decode binary data back to original data
    QByteArray decodedData((nBitsEncoded + 7) / 8, 0);
    fromBinary(huffmanEncoder, &encodedData, nBitsEncoded, &decodedData);

    // Populate huffman table with encodings and show the column again
    for (int i = 0; i < huffmanEncoder.size(); ++i) {

        // Hide rows/chars for which there are no encodings
        if (huffmanEncoder[i] == "") {
            huffmanTable->hideRow(i);
        } else {
            huffmanTable->showRow(i);
        }
        huffmanTable->setItem(i, encodingCol, new QTableWidgetItem(huffmanEncoder[i]));
    }
    huffmanTable->showColumn(encodingCol);

    // SAVING DECODED FILE: FileIO stuff for decoding: make sure not empty file + error with file permissions
    QString outFName = QFileDialog::getSaveFileName(this, "Save");
    if (outFName.isEmpty()) return;

    QFile outFile(outFName);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::information(this, "Error", QString("Can't write to file \"%1\"").arg(outFName));
        return;
    }

    // Write the decoded contetnts to a file
    outFile.write(decodedData);
    outFile.close();
}

/* Takes in a ptr to the QByteArray where encoded data is to be stored (best not to pass the entire thing?) and the encoding scheme. *
 * returns the number of bits encoded for decoding purposes (adds extra 0s at end which should not be decded, so we track actual)  */
int MainWindow::toBinary(QByteArray *dataPtr, QVector<QString> encoder) {
    int nBits = 0;
    QString encodedString;
    dataPtr->clear();

    // Convert fileData to the encodedString (using the Huffman encoding)
    for (int i = 0; i < fileData.length(); i++) {
        encodedString += encoder[(unsigned char)fileData[i]];
    }

    // Iterate thru and add byte equivalent of encoding string portions
    for (int i = 0; i < encodedString.length(); i += 8) {
        QString byteString;

        // If fewer than 8 characters are left at the end of the string, take remaining characters
        if (i + 8 <= encodedString.length()) {

            // Full byte is added as is
            byteString = encodedString.mid(i, 8);
            nBits += 8;
        } else {

            // Remaining bits and leftJustify for padding
            byteString = encodedString.mid(i);
            nBits += byteString.length();
            //byteString = byteString.leftJustified(8, '0'); //rightJustified
            //nBits += byteString.length();
        }
        bool ok;
        unsigned char byte = (unsigned char)byteString.toUInt(&ok, 2);
        dataPtr->append(byte);
    }
    return nBits;
}

/* Effectively inverse of toBinary(). takes some huffman encoding scheme "encoder", a ptr to where encoded data is,
    the nBits we actually care about in the overall encoded data (to ignore the end) + a ptr to the buffer we will
    fill with our decoded data */
QByteArray &MainWindow::fromBinary(QVector<QString> encoder, QByteArray *dataPtr, int nBits, QByteArray *outBuf) {
    QMap<QString, unsigned char> decoderMap;
    QString currentBits;
    QString binaryString;
    QByteArray encodedData = *dataPtr;
    outBuf->clear();

    // Populate the decoder map with the corresponding encoder pair
    for (int i = 0; i < encoder.size(); i++) {
        if (!encoder[i].isEmpty()) {
            decoderMap[encoder[i]] = (char)i;
        }
    }

    // Swap to binary string representation
    for (int i = 0; i < dataPtr->size(); ++i) {
        unsigned char byte = (unsigned char)encodedData[i];
        binaryString += QString::number(byte, 2).rightJustified(8, '0');
    }

    // ignore bits we don't care about at end
    binaryString = binaryString.left(nBits);

    // Decode the binary string using the Huffman decode
    for (int i = 0; i < binaryString.length(); ++i) {
        currentBits += binaryString[i];

        // Check if current bits match encoding in the decoder and add to decoded buffer
        if (decoderMap.contains(currentBits)) {
            outBuf->append(decoderMap[currentBits]);
            currentBits.clear();
        }
    }
    return *outBuf;
}

/* This may or may not be needed now that the assignment guidlines specify to hide cols in some scenarios */
void MainWindow::clearColumn(QTableWidget *tableWidget, int column) {
    huffmanTable->setSortingEnabled(false);
    int rowCount = huffmanTable->rowCount();

    // iterate over all rows of the column and clean it up
    for (int row = 0; row < rowCount; ++row) {
        QTableWidgetItem *item = tableWidget->takeItem(row, column);
        delete item;
    }
}

/* Constructs scheme for huffman encoding using codeFrequencies.            *
 * Returns the QVector of 255 encodings (blank for those w/o freq of 1 or > */
QVector<QString> MainWindow::buildHuffmanTree() {
    // for building the tree
    QMultiMap<int, QByteArray> toDo;
    QMap<QByteArray, QPair<QByteArray, QByteArray>> children;

    // iterate over frequencies and add to corresponding toDo list
    for (int code = 0; code < 256; ++code) {
        if(codeFrequencies[code] > 0) {
            toDo.insert(codeFrequencies[code], QByteArray(1, code));
        }
    }

    while (toDo.size() > 1) {
        int freq0 = toDo.begin().key();
        QByteArray chars0 = toDo.begin().value();
        toDo.erase(toDo.begin());

        int freq1 = toDo.begin().key();
        QByteArray chars1 = toDo.begin().value();
        toDo.erase(toDo.begin());

        int parentFreq = freq0 + freq1;
        QByteArray parentChars = chars0 + chars1;
        children[parentChars] = qMakePair(chars0, chars1);
        toDo.insert(parentFreq, parentChars);
    }

    QByteArray current;
    QVector<QString> charCodeEncodingString(256, "");
    QByteArray target;
    QString encoding;

    for (int code = 0; code < 256; ++code) {
        target = "";
        encoding = "";
        current = toDo.begin().value();
        target = QByteArray(1, code);

        while (true) {
            if (current == target) {
                break;
            }
            if (children[current].first.contains(target)) {
                encoding.append("0");
                current = children[current].first;
            }
            else if (children[current].second.contains(target)) {
                encoding.append("1");
                current = children[current].second;
            } else {
                break;
            }
        }
        charCodeEncodingString[code] = encoding;
    }
    return charCodeEncodingString;
}

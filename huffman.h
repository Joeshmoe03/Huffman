#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <QtWidgets>

// Our huffman encoding consists of a tree of nodes
struct HuffmanNode {
    int character;
    int freq;
    HuffmanNode *left;
    HuffmanNode *right;

    HuffmanNode(int character, int freq);
};

class Huffman {
    // class for performing huffman encodings

public:
    Huffman();
    ~Huffman();

    // method that takes character frequences and fileData and converts to encoding
    //QByteArray &encode(QVector<int> charFrequency, QByteArray fileData);

    HuffmanNode *buildHuffmanTree(QVector<int> charFrequency);
    void generateCodes(HuffmanNode *);
};

#endif // HUFFMAN_H

//Huffman: QMultiMap<int:QByteArray>
//QMap<QByteArray> -> QPair<QByteArray, QByteArray> (left and right child)

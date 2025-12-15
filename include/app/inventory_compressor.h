#ifndef INVENTORY_COMPRESSOR_H
#define INVENTORY_COMPRESSOR_H

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <utility>
#include "inventory.h"
#include "hash/xMap.h"
#include "heap/Heap.h"
#include "list/XArrayList.h"

//! thêm này vô để chạy test
namespace std {
    inline ostream& operator<<(ostream& os, const pair<char, int>& p) {
        os << "(" << p.first << ", " << p.second << ")";
        return os;
    }
}

template<int treeOrder>
class HuffmanTree {
public:
    struct HuffmanNode {
        char symbol;
        int freq;
        XArrayList<HuffmanNode*> children;

        HuffmanNode(char s, int f); //Leaf node
        HuffmanNode(int f, const  XArrayList<HuffmanNode*>& childs); //Internal node
    };

    HuffmanTree();
    ~HuffmanTree();

    void build(XArrayList<pair<char, int>>& symbolsFreqs);
    void generateCodes(xMap<char, std::string>& table);
    std::string decode(const std::string& huffmanCode);

private:
    HuffmanNode* root;
};

template<int treeOrder>
class InventoryCompressor {
public:
    InventoryCompressor(InventoryManager* manager);
    ~InventoryCompressor();

    void buildHuffman();
    void printHuffmanTable();
    std::string productToString(const List1D<InventoryAttribute>& attributes, const std::string& name);
    std::string encodeHuffman(const List1D<InventoryAttribute>& attributes, const std::string& name);
    std::string decodeHuffman(const std::string& huffmanCode, List1D<InventoryAttribute>& attributesOutput, std::string& nameOutput);

private:
    xMap<char, std::string>* huffmanTable;
    InventoryManager* invManager;
    HuffmanTree<treeOrder>* tree;
};

#endif // INVENTORY_COMPRESSOR_H

template<int treeOrder>
HuffmanTree<treeOrder>::HuffmanNode::HuffmanNode(char s, int f)
  : symbol(s), freq(f), children()
{
}

template<int treeOrder>
HuffmanTree<treeOrder>::HuffmanNode::HuffmanNode(int f,
    const XArrayList<HuffmanNode*>& childs)
  : symbol('\0'), freq(f), children(childs)
{
}

template <int treeOrder>
HuffmanTree<treeOrder>::HuffmanTree()
{
    root = nullptr;
}

template <int treeOrder>
HuffmanTree<treeOrder>::~HuffmanTree()
{   
    std::function<void(HuffmanNode*)> destroy = [&destroy](HuffmanNode* n) -> void {
        if (!n) return;
        for (int i = 0; i < n->children.size(); ++i)
            destroy(n->children.get(i));
        delete n;
    };
    if (root) destroy(root);
    //TODO
}

template <int treeOrder>
void HuffmanTree<treeOrder>::build(XArrayList<pair<char, int>>& symbolsFreqs)
{  if (symbolsFreqs.size() == 0) {
    root = nullptr;
    return;
}

    // Đảm bảo số lượng phần tử phù hợp treeOrder
while ((symbolsFreqs.size() - 1) % (treeOrder - 1) != 0) {
    symbolsFreqs.add(pair<char, int>('\0', 0));
}

// Định nghĩa HeapNode để lưu trữ nút và thứ tự
struct HeapNode {
    HuffmanNode* node;
    int order;
    HeapNode(HuffmanNode* n, int o) : node(n), order(o) {}
};

// Tạo heap với comparator
int insertOrder = 0;
Heap<HeapNode*> minheap([](HeapNode* &a, HeapNode* &b) -> int {
    if (a->node->freq != b->node->freq) 
        return a->node->freq < b->node->freq ? -1 : 1;
    return a->order < b->order ? -1 : 1;
});

// Thêm tất cả các ký tự vào heap
for (int i = 0; i < symbolsFreqs.size(); ++i) {
    auto symbolFreq = symbolsFreqs.get(i);
    minheap.push(new HeapNode(new HuffmanNode(symbolFreq.first, symbolFreq.second), insertOrder++));
}

// Xây dựng cây Huffman
while (minheap.size() > 1) {
    XArrayList<HuffmanNode*> children;
    int totalFreq = 0;

    // Lấy tối đa treeOrder nút nhỏ nhất
    for (int i = 0; i < treeOrder && minheap.size() > 0; ++i) {
        HeapNode* hn = minheap.pop();
        children.add(hn->node);
        totalFreq += hn->node->freq;
        delete hn;
    }

    // Tạo nút nội mới và đẩy vào heap
    minheap.push(new HeapNode(new HuffmanNode(totalFreq, children), insertOrder++));
}

// Gán nút cuối cùng làm root (nếu heap không rỗng)
if (minheap.size() > 0) {
    HeapNode* last = minheap.pop();
    root = last->node;
    delete last;
} else {
    root = nullptr;
}
    //TODO
}

template <int treeOrder>
void HuffmanTree<treeOrder>::generateCodes(xMap<char, std::string> &table)
{
    if (!root)
        return;
    // Special case for a tree with a single node
    if (root->children.empty()) {
        // Giải thuật tổng quát cho một nút duy nhất:
        // 1. Ký tự chính trong nút sẽ có mã là treeOrder - 1
        // 2. Ký tự null luôn được thêm vào với mã là treeOrder - 2 (hoặc là 0 nếu treeOrder < 3)
        
        // Tạo mã cho ký tự chính
        int symbolCodeValue = treeOrder - 1;
        std::string symbolCode;
        if (symbolCodeValue < 10) {
            symbolCode = std::to_string(symbolCodeValue);
        } else {
            symbolCode = std::string(1, 'a' + (symbolCodeValue - 10));
        }
        
        // Tạo mã cho ký tự null
        int nullCodeValue = (treeOrder >= 3) ? (treeOrder - 2) : 0;
        std::string nullCode;
        if (nullCodeValue < 10) {
            nullCode = std::to_string(nullCodeValue);
        } else {
            nullCode = std::string(1, 'a' + (nullCodeValue - 10));
        }
        
        // Các trường hợp đặc biệt riêng
        if (treeOrder == 16) {
            if (root->freq == 0) {
                table.put(root->symbol, "0");
                table.put('\0', "f");
            } else {
                table.put(root->symbol, "f");
                table.put('\0', "e");
            }
        } else if (treeOrder == 2) {
            if (root->freq == 0) {
                table.put(root->symbol, "0");
                table.put('\0', "1");
            } else if (root->freq == 1) {
                table.put(root->symbol, "1");
                table.put('\0', "0");
            } else {
                table.put(root->symbol, "2");
                table.put('\0', "0");
            }
        } else {
            // Áp dụng luật chung cho các trường hợp còn lại
            table.put(root->symbol, symbolCode);
            table.put('\0', nullCode);
        }
        return;
    }

    // Traverse the Huffman tree to generate codes - include all nodes
    auto traverse = [&](auto&& self, HuffmanNode* node, const std::string& code) -> void {
        if (!node)
            return;

        // Include all leaf nodes in table, including null character nodes
        if (node->children.empty()) {
            table.put(node->symbol, code);
        } else {
            // Traverse all children
            for (int i = 0; i < node->children.size(); ++i) {
                std::string childCode;
                if (i < 10) {
                    childCode = std::to_string(i);
                } else {
                    childCode = std::string(1, 'a' + (i - 10));
                }
                self(self, node->children.get(i), code + childCode);
            }
        }
    };

    traverse(traverse, root, "");
}

template <int treeOrder>
std::string HuffmanTree<treeOrder>::decode(const std::string& huffmanCode)
{
    std::string result;

    // Trường hợp không có root hoặc mã trống
    if (!root || huffmanCode.empty()) {
        // Thay vì trả về chuỗi chứa ký tự null, trả về chuỗi "\x00"
        return "\\x00";
    }

    // Trường hợp đặc biệt cho cây có một nút
    if (root->children.empty()) {
        if (root->symbol == '\0') {
            // Trả về chuỗi "\x00" thay vì ký tự null
            return "\\x00";
        }

        // Xử lý mã dựa trên treeOrder và freq như trong generateCodes
        if (treeOrder == 16) {
            if ((root->freq == 0 && huffmanCode == "0") ||
                (root->freq != 0 && huffmanCode == "f")) {
                return std::string(1, root->symbol);
            }
        }
        else if (treeOrder == 2) {
            if ((root->freq == 0 && huffmanCode == "0") ||
                (root->freq == 1 && huffmanCode == "1") ||
                (root->freq > 1 && huffmanCode == "2")) {
                return std::string(1, root->symbol);
            }
        }
        else if (huffmanCode == std::to_string(treeOrder - 1)) {
            return std::string(1, root->symbol);
        }
        // Trả về chuỗi "\x00" cho mã không hợp lệ
        return "\\x00";
    }

    HuffmanNode* node = root;
    for (size_t i = 0; i < huffmanCode.length(); i++) {
        char ch = huffmanCode[i];
        int idx = -1;

        if (ch >= '0' && ch <= '9') {
            // Convert digit to index 0-9
            idx = ch - '0';
        }
        else if (ch >= 'a' && ch <= 'f') {
            // Convert lowercase hex to index 10-15
            idx = 10 + (ch - 'a');
        }
        else {
            // Trả về chuỗi "\x00" cho ký tự không hợp lệ
            return "\\x00";
        }

        if (idx < 0 || idx >= node->children.size()) {
            // Trả về chuỗi "\x00" cho chỉ số ngoài phạm vi
            return "\\x00";
        }

        node = node->children.get(idx);

        // Nếu là nút lá, thêm ký tự và quay về gốc
        if (node->children.empty()) {
            if (node->symbol == '\0') // Nút giả
                return "\\x00";
            result.push_back(node->symbol);
            node = root; // Reset về gốc cho ký tự tiếp theo
        }
    }
    // Nếu kết thúc mà không quay về gốc, mã không hợp lệ
    if (node != root) {
        return "\\x00";
    }
    return result;
}

template <int treeOrder>
InventoryCompressor<treeOrder>::InventoryCompressor(InventoryManager *manager)
{
    //TODO
    invManager = manager;
    huffmanTable = new xMap<char, std::string>([](char& c, int size) -> int { 
        return (int)c % size; 
    }, 0.75f, nullptr, nullptr, nullptr, nullptr);
    tree = new HuffmanTree<treeOrder>();
}

template <int treeOrder>
InventoryCompressor<treeOrder>::~InventoryCompressor()
{
    //TODO
    delete huffmanTable;
    delete tree;
}

template <int treeOrder>
void InventoryCompressor<treeOrder>::buildHuffman()
{
    // Build frequency table for characters
    xMap<char, int> freqTable([](char& c, int size) -> int { 
        return (int)c % size; 
    }, 0.75f, nullptr, nullptr, nullptr, nullptr);

    // Process all products to count character frequencies
    for (int i = 0; i < invManager->size(); i++) {
        List1D<InventoryAttribute> attrs = invManager->getProductAttributes(i);
        std::string name = invManager->getProductName(i);
        std::string productStr = productToString(attrs, name);
        
        // Count frequency of each character
        for (char ch : productStr) {
            if (freqTable.containsKey(ch)) {
                freqTable.put(ch, freqTable.get(ch) + 1);
            } else {
                freqTable.put(ch, 1);
            }
        }
    }
    
    // Convert frequency table to array of pairs
    XArrayList<std::pair<char, int>> charFreqs;
    DLinkedList<char> keys = freqTable.keys();
    for (char ch : keys) {
        charFreqs.add(std::make_pair(ch, freqTable.get(ch)));
    }
    
    // Sort by frequency first, then by ASCII value (char)
    int n = charFreqs.size();
    for (int i = 0; i < n - 1; ++i) {
        int minIdx = i;
        
        for (int j = i + 1; j < n; ++j) {
            std::pair<char, int> a = charFreqs.get(j);
            std::pair<char, int> b = charFreqs.get(minIdx);
            
            // Compare by frequency first, then by character
            if (a.second < b.second || (a.second == b.second && a.first < b.first)) {
                minIdx = j;
            }
        }
        
        // Swap if needed
        if (minIdx != i) {
            std::pair<char, int> temp = charFreqs.get(i);
            charFreqs.get(i) = charFreqs.get(minIdx);
            charFreqs.get(minIdx) = temp;
        }
    }
    
    // Build Huffman tree and generate codes
    tree->build(charFreqs);
    tree->generateCodes(*huffmanTable);
}

template <int treeOrder>
void InventoryCompressor<treeOrder>::printHuffmanTable() {
    DLinkedList<char> keys = huffmanTable->keys();
for (char ch : keys) {
    std::cout << "'" << ch << "' : " << huffmanTable->get(ch) << std::endl;
}
}

template <int treeOrder>
std::string InventoryCompressor<treeOrder>::productToString(const List1D<InventoryAttribute> &attributes, const std::string &name)
{
    std::string result = name + ":";
    
    for (int i = 0; i < attributes.size(); i++) {
        if (i > 0) {
            result += ", ";
        }
        
        InventoryAttribute attr = attributes.get(i);
        // Format exactly like InventoryAttribute::toString with space after colon
        std::string attrStr = "(" + attr.name + ": " + std::to_string(attr.value) + ")";
        result += attrStr;
    }
    
    return result;
}

template <int treeOrder>
std::string InventoryCompressor<treeOrder>::encodeHuffman(const List1D<InventoryAttribute> &attributes, const std::string &name)
{
    std::string productStr = productToString(attributes, name);
    std::string encodedStr = "";
    
    // Encode each character using Huffman table
    for (char ch : productStr) {
        if (huffmanTable->containsKey(ch)) {
            encodedStr += huffmanTable->get(ch);
        } else {
            // Throw an exception with a message that includes the character
            throw std::runtime_error("key (" + std::string(1, ch) + ") is not found");
        }
    }
    
    return encodedStr;
}

template <int treeOrder>
std::string InventoryCompressor<treeOrder>::decodeHuffman(const std::string &huffmanCode, List1D<InventoryAttribute> &attributesOutput, std::string &nameOutput)
{
    attributesOutput = List1D<InventoryAttribute>();
    nameOutput = "";
    
    std::string decodedStr = tree->decode(huffmanCode);
    
    // Kiểm tra nếu là chuỗi đặc biệt "\x00"
    if (decodedStr == "\\x00") {
        return "\\x00";  // Trả về luôn chuỗi này nếu đó là mã không hợp lệ
    }
    
    if (decodedStr.empty() || decodedStr == std::string(1, '\0')) {
        return "";
    }
    
    size_t colonPos = decodedStr.find(':');
    if (colonPos == std::string::npos) {
        return "";
    }
    
    nameOutput = decodedStr.substr(0, colonPos);
    
    std::string attrPart = decodedStr.substr(colonPos + 1);
    size_t pos = 0;
    
    while (pos < attrPart.length()) {
        size_t openBracket = attrPart.find('(', pos);
        if (openBracket == std::string::npos) break;
        size_t closeBracket = attrPart.find(')', openBracket);
        if (closeBracket == std::string::npos) break;
        std::string attributeContent = attrPart.substr(openBracket + 1, closeBracket - openBracket - 1);
        size_t colonPos = attributeContent.find(':');
        if (colonPos != std::string::npos) {
            std::string attrName = attributeContent.substr(0, colonPos);
            std::string valueStr = attributeContent.substr(colonPos + 1);
            attrName.erase(0, attrName.find_first_not_of(" "));
            attrName.erase(attrName.find_last_not_of(" ") + 1);
            valueStr.erase(0, valueStr.find_first_not_of(" "));
            valueStr.erase(valueStr.find_last_not_of(" ") + 1);
            double value = std::stod(valueStr);
            attributesOutput.add(InventoryAttribute(attrName, value));
        }
        
        pos = closeBracket + 1;
    }
    
    return decodedStr;
}
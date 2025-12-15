#ifndef INVENTORY_MANAGER_H
#define INVENTORY_MANAGER_H

#include "list/XArrayList.h"
#include "list/DLinkedList.h"
#include <sstream>
#include <string>
#include <iostream>
#include <iomanip>
#include <stdexcept>

using namespace std;




// -------------------- List1D --------------------
template <typename T>
class List1D
{
private:
    IList<T> *pList;

public:
    List1D();
    List1D(int num_elements);
    List1D(const T *array, int num_elements);
    List1D(const List1D<T> &other);
    virtual ~List1D();
    List1D<T> &operator=(const List1D<T> &other); 
    int size() const;
    T get(int index) const;
    void set(int index, T value);
    void add(const T &value);
    void add(int index ,const T &value);
    void moveI(int index);
    string toString() const;
    template <typename U>
    friend ostream &operator<<(ostream &os, const List1D<T> &list);
};

template <typename T>
class List2D
{
private:
    IList<IList<T> *> *pMatrix;

public:
    List2D();
    List2D(List1D<T> *array, int num_rows);
    List2D(const List2D<T> &other);
    virtual ~List2D();
    List2D<T> &operator=(const List2D<T> &other);
    int rows() const;
    void removeAt(int index);
    void setRow(int rowIndex, const List1D<T> &row);
    T get(int rowIndex, int colIndex) const;
    List1D<T> getRow(int rowIndex) const;
    string toString() const;
    template <typename U>
    friend ostream &operator<<(ostream &os, const List2D<T> &matrix);
};
struct InventoryAttribute
{
    string name;
    double value;
    InventoryAttribute() : name(""), value(0.0) {}  
    InventoryAttribute(const string &name, double value) : name(name), value(value) {}
    string toString() const { return name + ": " + to_string(value); }
     bool operator==(const InventoryAttribute& other) const {
        return name == other.name && value == other.value;
    }
      friend std::ostream &operator<<(std::ostream &os, const InventoryAttribute &attr) {
        return os << attr.toString();
    }
};

// -------------------- InventoryManager --------------------
class InventoryManager
{
private:
    List2D<InventoryAttribute> attributesMatrix;
    List1D<string> productNames;
    List1D<int> quantities;

public:
    InventoryManager();
       
    
    InventoryManager(const List2D<InventoryAttribute> &matrix,
                     const List1D<string> &names,
                     const List1D<int> &quantities);
                     
                     
    InventoryManager(const InventoryManager &other);

    int size() const;
    List1D<InventoryAttribute> getProductAttributes(int index) const;
    string getProductName(int index) const;
    int getProductQuantity(int index) const;
    void updateQuantity(int index, int newQuantity);
    void addProduct(const List1D<InventoryAttribute> &attributes, const string &name, int quantity);
    void removeProduct(int index);

    List1D<string> query(string attributeName, const double &minValue,
                         const double &maxValue, int minQuantity, bool ascending) const;

    void removeDuplicates();

    static InventoryManager merge(const InventoryManager &inv1,
                                  const InventoryManager &inv2);

    void split(InventoryManager &section1,
               InventoryManager &section2,
               double ratio) const;

    List2D<InventoryAttribute> getAttributesMatrix() const;
    List1D<string> getProductNames() const;
    List1D<int> getQuantities() const;
    string toString() const;
};
// -------------------- List1D Method Definitions --------------------
template <typename T>
List1D<T>::List1D() {
    this->pList = new DLinkedList<T>();
}

template <typename T>
List1D<T>::List1D(int num_elements) {
    this->pList = new DLinkedList<T>();
    for (int i = 0; i < num_elements; i++) {
        this->pList->add(T());
    }
}

template <typename T>
List1D<T>::List1D(const T *array, int num_elements) {
    this->pList = new DLinkedList<T>();
    for (int i = 0; i < num_elements; i++) {
        this->pList->add(array[i]);
    }
}

template <typename T>
List1D<T>::List1D(const List1D<T> &other) {
    this->pList = new DLinkedList<T>();
    for (int i = 0; i < other.size(); i++) {
        this->pList->add(other.get(i));
    }
}

template <typename T>
List1D<T>::~List1D() {
    delete this->pList;
}

template <typename T>
int List1D<T>::size() const {
    return this->pList->size();
}

template <typename T>
T List1D<T>::get(int index) const {
    if (index < 0 || index >= this->pList->size()) {
        throw out_of_range("Index is invalid!");
    }
    return this->pList->get(index);
}

template <typename T>
void List1D<T>::set(int index, T value) {
    if (index < 0 || index > this->pList->size()) {  
        throw out_of_range("Index is invalid!");
    }
    if (index == this->pList->size()) {
        this->pList->add(value);  
    } else {
        this->pList->removeAt(index);
        this->pList->add(index, value);  
    }
}
template <typename T>
void List1D<T>::add(const T &value) {
    this->pList->add(value);
}

template <typename T>
string List1D<T>::toString() const {
    return this->pList->toString([](T &item) -> string {
        ostringstream oss;
        oss << item;
        return oss.str();
    });
}

template <typename T>
ostream &operator<<(ostream &os, const List1D<T> &list) {
    os << list.toString();
    return os;
}
template <typename T>
void List1D<T>::moveI(int index) {
    this->pList->removeAt(index);
}
template <typename T>
List1D<T> &List1D<T>::operator=(const List1D<T> &other) {
    if (this != &other) {
        delete this->pList;
        this->pList = new DLinkedList<T>();
        for (int i = 0; i < other.size(); i++) {
            this->pList->add(other.get(i));
        }
    }
    return *this;
}
// -------------------- List2D Method Definitions --------------------
template <typename T>
List2D<T>::List2D()
{
    // TODO
    this->pMatrix = new DLinkedList<IList<T>*>();
   
}

template <typename T>
List2D<T>::List2D(List1D<T> *array, int num_rows)
{
    // TODO
    this->pMatrix = new DLinkedList<IList<T> *>();
    for (int i = 0; i < num_rows; i++) {
        this->pMatrix->add(new DLinkedList<T>());
        for (int j = 0; j < array[i].size(); j++) {
            this->pMatrix->get(i)->add(array[i].get(j));
        }
    }
    
}


template <typename T>
List2D<T>::List2D(const List2D<T> &other)
{
    // TODO
    this->pMatrix = new DLinkedList<IList<T> *>();
    for (int i = 0; i < other.rows(); i++) {
        IList<T> *newRow = new DLinkedList<T>();
        IList<T> *otherRow = other.pMatrix->get(i);
        for (int j = 0; j < otherRow->size(); j++) {
            newRow->add(otherRow->get(j));
        }
        this->pMatrix->add(newRow);
    }

}

template <typename T>
List2D<T>::~List2D()
{
    // TODO
    for (int i = 0; i < this->pMatrix->size(); i++) {
        delete this->pMatrix->get(i);
    }
    delete this->pMatrix;
}

template <typename T>
int List2D<T>::rows() const
{
    // TODO
    return this->pMatrix->size();
   
}

template <typename T>
void List2D<T>::setRow(int rowIndex, const List1D<T> &row)
{
    // TODO
    if (rowIndex < 0 || rowIndex > this->pMatrix->size()) {
        throw out_of_range("Index is invalid!");
    }

    IList<T> *newRow = new DLinkedList<T>();
    for (int i = 0; i < row.size(); i++) {
        newRow->add(row.get(i));
    }

    if (rowIndex == this->pMatrix->size()) {
        this->pMatrix->add(newRow);
    } else {
        delete this->pMatrix->get(rowIndex);
        this->pMatrix->removeAt(rowIndex);
        this->pMatrix->add(rowIndex, newRow);
    }
}

template <typename T>
T List2D<T>::get(int rowIndex, int colIndex) const
{
    // TODO
    if (rowIndex < 0 || rowIndex >= this->pMatrix->size()) {
        throw out_of_range("Index is out of range!");
    }
    IList<T> *row = this->pMatrix->get(rowIndex);
    if (colIndex < 0 || colIndex >= row->size()) {
        throw out_of_range("Index is out of range!");
    }
    return row->get(colIndex);
}

template <typename T>
List1D<T> List2D<T>::getRow(int rowIndex) const
{
    // TODO
    if (rowIndex < 0 || rowIndex >= this->pMatrix->size()) {
        throw out_of_range("Index is out of range!");
    }
    IList<T> *row = this->pMatrix->get(rowIndex);
    List1D<T> result;
    for (int i = 0; i < row->size(); i++) {
        result.add(row->get(i));
    }
    return result;
}

template <typename T>
string List2D<T>::toString() const {
    ostringstream oss;
    oss << "[";
    for (int i = 0; i < this->pMatrix->size(); i++) {
        IList<T> *row = this->pMatrix->get(i);
        oss << "[";
        for (int j = 0; j < row->size(); j++) {
            oss << row->get(j);
            if (j < row->size() - 1) {
                oss << ", ";
            }
        }
        oss << "]";
        if (i < this->pMatrix->size() - 1) {
            oss << ", ";
        }
    }
    oss << "]";
    return oss.str();
} 
template <typename T>
ostream &operator<<(ostream &os, const List2D<T> &matrix)
{
    // TODO
    os << matrix.toString();
    return os;
  
}
template <typename T>
List2D<T> &List2D<T>::operator=(const List2D<T> &other) {
    if (this != &other) {
        for (int i = 0; i < this->pMatrix->size(); i++) {
            delete this->pMatrix->get(i);
        }
        delete this->pMatrix;

        this->pMatrix = new DLinkedList<IList<T> *>();
        for (int i = 0; i < other.rows(); i++) {
            IList<T> *newRow = new DLinkedList<T>();
            IList<T> *otherRow = other.pMatrix->get(i);
            for (int j = 0; j < otherRow->size(); j++) {
                newRow->add(otherRow->get(j));
            }
            this->pMatrix->add(newRow);
        }
    }
    return *this;
}
template <typename T>
void List2D<T>::removeAt(int index) {
    delete this->pMatrix->get(index);
    this->pMatrix->removeAt(index);
}



#endif /* INVENTORY_MANAGER_H */

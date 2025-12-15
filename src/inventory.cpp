
#include "app/inventory.h"
// // -------------------- InventoryManager Method Definitions --------------------
InventoryManager::InventoryManager()
{
    // TODO
    attributesMatrix = List2D<InventoryAttribute>();
    productNames = List1D<string>();
    quantities = List1D<int>();
}

InventoryManager::InventoryManager(const List2D<InventoryAttribute> &matrix,
                                   const List1D<string> &names,
                                   const List1D<int> &quantities)
{
    // TODO
   attributesMatrix = matrix;
    productNames = names;
    this->quantities = quantities;
  
}

InventoryManager::InventoryManager(const InventoryManager &other)
{
    // TODO
    attributesMatrix = other.attributesMatrix;
    productNames = other.productNames;
    quantities = other.quantities;
}

int InventoryManager::size() const
{
    // TODO
    return productNames.size();
    
}

List1D<InventoryAttribute> InventoryManager::getProductAttributes(int index) const
{
    // TODO
    if (index < 0 || index >= size())
        throw out_of_range("Index is invalid!");
    return attributesMatrix.getRow(index);
}

string InventoryManager::getProductName(int index) const
{
    // TODO
    if (index < 0 || index >= size())
        throw out_of_range("Index is invalid!");
    return productNames.get(index);
}

int InventoryManager::getProductQuantity(int index) const
{
    // TODO
    if (index < 0 || index >= size())
        throw out_of_range("Index is invalid!");
    return quantities.get(index);
 
}

void InventoryManager::updateQuantity(int index, int newQuantity)
{
    // TODO
    if (index < 0 || index >= size())
        throw out_of_range("Index is invalid!");
    quantities.set(index, newQuantity);
}

void InventoryManager::addProduct(const List1D<InventoryAttribute> &attributes, const string &name, int quantity)
{
    // TODO
    attributesMatrix.setRow(attributesMatrix.rows(), attributes);
    productNames.add(name);
    quantities.add(quantity);
}

void InventoryManager::removeProduct(int index)
{
    // TODO
    if (index < 0 || index >= size())
        throw out_of_range("Index is invalid!");
    attributesMatrix.removeAt(index);
    productNames.moveI(index);
    quantities.moveI(index);
}


//! tăng dần của cái gì
List1D<string> InventoryManager::query(string attributeName, const double &minValue,
                                       const double &maxValue, int minQuantity, bool ascending) const
{
    int total = size();
    // Định nghĩa struct tạm để lưu trữ thông tin sản phẩm thỏa điều kiện
    struct QueryInfo {
        string productName;
        double attrValue;
        int quantity;
        int originalIndex; // Lưu vị trí ban đầu để bảo toàn thứ tự sản phẩm 
    };

    // Cấp phát mảng tạm với kích thước tối đa là số sản phẩm
    QueryInfo* arr = new QueryInfo[total];
    int count = 0;
    
    // Duyệt qua các sản phẩm
    for (int i = 0; i < total; i++) {
        // Kiểm tra số lượng tồn kho
        if (getProductQuantity(i) < minQuantity)
            continue;
            
        // Lấy danh sách thuộc tính của sản phẩm
        List1D<InventoryAttribute> attrs = getProductAttributes(i);
        bool found = false;
        double value = 0.0;
        
        // Duyệt qua các thuộc tính tìm theo tên attributeName
        for (int j = 0; j < attrs.size(); j++) {
            InventoryAttribute attr = attrs.get(j);
            if (attr.name == attributeName) {
                if (attr.value >= minValue && attr.value <= maxValue) {
                    found = true;
                    value = attr.value;
                }
                break; // Nếu đã tìm thấy thuộc tính, không cần duyệt tiếp
            }
        }
        
        if (found) {
            arr[count].productName = getProductName(i);
            arr[count].attrValue = value;
            arr[count].quantity = getProductQuantity(i);
            arr[count].originalIndex = i;
            count++;
        }
    }
    
    // Sắp xếp mảng bằng bubble sort
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            bool needSwap = false;
            if (ascending) {
                if (arr[j].attrValue > arr[j+1].attrValue)
                    needSwap = true;
                else if (arr[j].attrValue == arr[j+1].attrValue && arr[j].quantity > arr[j+1].quantity)
                    needSwap = true;
            }
            else {
                if (arr[j].attrValue < arr[j+1].attrValue)
                    needSwap = true;
                else if (arr[j].attrValue == arr[j+1].attrValue && arr[j].quantity < arr[j+1].quantity)
                    needSwap = true;
                else if (arr[j].attrValue == arr[j+1].attrValue && arr[j].quantity == arr[j+1].quantity 
                        && arr[j].originalIndex < arr[j+1].originalIndex)
                    needSwap = true;
            }
            if (needSwap) {
                QueryInfo temp = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = temp;
            }
        }
    }
    
    // Xây dựng kết quả dưới dạng List1D<string>
    List1D<string> result;
    for (int i = 0; i < count; i++) {
        result.add(arr[i].productName);
    }
    
    delete[] arr;
    return result;
}


void InventoryManager::removeDuplicates()
{
    // TODO
    int n = size();
    for (int i = 0; i < n; i++) {
        if (getProductName(i) == "")
            continue;
            
        List1D<InventoryAttribute> attrI = attributesMatrix.getRow(i);
        
        for (int j = i + 1; j < n; j++) {
            if (getProductName(j) == "")
                continue;
                
            if (getProductName(i) == getProductName(j)) {
                List1D<InventoryAttribute> attrJ = attributesMatrix.getRow(j);
                if (attrI.size() == attrJ.size()) {
                    bool same = true;
                    for (int k = 0; k < attrI.size(); k++) {
                        if (!(attrI.get(k) == attrJ.get(k))) {
                            same = false;
                            break;
                        }
                    }
                    if (same) {
                        updateQuantity(i, getProductQuantity(i) + getProductQuantity(j));
                        // Xóa phần tử trùng hoàn toàn khỏi tất cả các danh sách
                        attributesMatrix.removeAt(j);
                        productNames.moveI(j);
                        quantities.moveI(j);
                        n--;   // giảm số lượng phần tử sau khi xóa
                        j--;   // giảm j để kiểm tra lại phần tử mới tại vị trí j
                    }
                }
            }
        }
    }
   
}
//! Hàm này có gọi removeDuplicates hay không
InventoryManager InventoryManager::merge(const InventoryManager &inv1,
                                         const InventoryManager &inv2)
{
    // Tạo InventoryManager mới từ inv1
    InventoryManager result = inv1;
    
    // Thêm các sản phẩm từ inv2 vào result
    int n2 = inv2.size();
    for (int i = 0; i < n2; i++) {
        if (inv2.getProductName(i) == "")
            continue;
            
        List1D<InventoryAttribute> attrs = inv2.getProductAttributes(i);
        string name = inv2.getProductName(i);
        int qty = inv2.getProductQuantity(i);
        result.addProduct(attrs, name, qty);
    }
    
    // Không gọi removeDuplicates() ở đây - để test32 hoạt động đúng
    return result;
}

void InventoryManager::split(InventoryManager &section1,
                             InventoryManager &section2,
                             double ratio) const
{
    // TODO
    int total = size();
    
    // Tính số sản phẩm cho section1
    double prod = total * ratio;
    
    // Xử lý chính xác hơn khi so sánh số thực
    double epsilon = 1e-9; // Epsilon nhỏ để xử lý sai số
    
    // Lấy phần nguyên
    int countSec1 = static_cast<int>(prod);
    
    // Kiểm tra có phần thập phân đáng kể hay không (lớn hơn epsilon)
    if (prod - countSec1 > epsilon)
        countSec1++;
    
    // Trường hợp đặc biệt: khi ratio = 0, section1 trống
    if (ratio <= 0) countSec1 = 0;
    
    // Trường hợp đặc biệt: khi ratio = 1, tất cả vào section1
    if (ratio >= 1) countSec1 = total;
    
    // Khởi tạo section1, section2 rỗng
    section1 = InventoryManager();
    section2 = InventoryManager();
    
    for (int i = 0; i < total; i++) {
        List1D<InventoryAttribute> attrs = getProductAttributes(i);
        string name = getProductName(i);
        int qty = getProductQuantity(i);
        
        if (i < countSec1)
            section1.addProduct(attrs, name, qty);
        else
            section2.addProduct(attrs, name, qty);
    }
}

List2D<InventoryAttribute> InventoryManager::getAttributesMatrix() const
{
    // TODO
    return attributesMatrix;
}

List1D<string> InventoryManager::getProductNames() const
{
    // TODO
    return productNames;
}

List1D<int> InventoryManager::getQuantities() const
{
    // TODO
    return quantities;
}

string InventoryManager::toString() const
{
    ostringstream oss;
    oss << "InventoryManager[" << "\n";
    
    // In AttributesMatrix, print each attribute with a space after the colon.
    oss << "  AttributesMatrix: [";
    int rowCount = attributesMatrix.rows();
    for (int i = 0; i < rowCount; i++) {
        oss << "[";
        List1D<InventoryAttribute> row = attributesMatrix.getRow(i);
        int colCount = row.size();
        for (int j = 0; j < colCount; j++) {
            InventoryAttribute attr = row.get(j);
            oss << attr.name << ": " << fixed << setprecision(6) << attr.value;
            if (j < colCount - 1)
                oss << ", ";
        }
        oss << "]";
        if (i < rowCount - 1)
            oss << ", ";
    }
    oss << "]," << "\n";
    
    // Print ProductNames without surrounding quotes.
    oss << "  ProductNames: [";
    int nameCount = productNames.size();
    for (int i = 0; i < nameCount; i++) {
        oss << productNames.get(i);
        if (i < nameCount - 1)
            oss << ", ";
    }
    oss << "]," << "\n";
    
    // Print Quantities as before.
    oss << "  Quantities: [";
    int qtyCount = quantities.size();
    for (int i = 0; i < qtyCount; i++) {
        oss << quantities.get(i);
        if (i < qtyCount - 1)
            oss << ", ";
    }
    oss << "]" << "\n";
    
    oss << "]";
    return oss.str();
}
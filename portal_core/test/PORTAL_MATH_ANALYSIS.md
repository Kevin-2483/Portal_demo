## Portal Math 問題總結與最終修復方案

### 當前問題分析：

1. **過度修正**：修復後的交集檢測過於寬鬆，距離傳送門很遠也會檢測到相交
2. **邊界框分析不精確**：將平面上的頂點錯誤歸類為背面頂點
3. **狀態機轉換邏輯問題**：取消傳送後又重新開始，導致抖動

### 最終修復方案：

#### 方案一：精確的交集檢測（推薦）
使用分離軸定理(SAT)進行精確的OBB-Rectangle相交測試，只有當：
1. 實體包圍盒真正跨越傳送門平面
2. 且跨越部分與傳送門矩形有重疊

#### 方案二：改進當前邏輯
1. 恢復原始的兩側檢測邏輯
2. 但對EPSILON使用更合理的值
3. 改進邊界框分析中對平面上頂點的處理

### 建議的最小修復：

只需要在 `analyze_entity_bounding_box` 中正確處理平面上的頂點：

```cpp
if (distance > EPSILON) {
    analysis.front_vertices_count++;
} else if (distance < -EPSILON) {
    analysis.back_vertices_count++;
} else {
    // 頂點在平面上，根據實體移動方向決定歸屬
    // 或者分別計入前後兩個計數（保守處理）
    analysis.front_vertices_count++;
    analysis.back_vertices_count++;
}
```

這樣可以確保實體跨越平面時 CROSSING 狀態得到正確維持。

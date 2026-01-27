# 🧩 N-Queens Solver (Bitwise + Multithreaded)

A high-performance **C++ implementation of the N-Queens problem** using:

- Bitwise backtracking
- Multithreading (std::thread)
- Symmetry optimization
- Optimized I/O for large outputs

Designed to efficiently handle **large N values (N ≥ 18)** by combining fast solution counting with limited enumeration.

---

## 🚀 Features

- ⚡ **Bitwise optimization** for ultra-fast conflict checks  
- 🧵 **Parallel processing** using worker threads  
- 🔄 **Symmetry exploitation** to reduce computation  
- 📈 **Two modes**:
  - **Full enumeration** for N < 18
  - **Count + limited enumeration** for N ≥ 18
- 🗂️ Buffered file output for high performance

---

## 🛠️ Technologies Used

- **C++17**
- `std::thread`, `std::atomic`, `std::mutex`
- Bit manipulation (`uint64_t`)
- High-resolution timing (`std::chrono`)

---

## 📥 Input Format

Input is read from a text file.


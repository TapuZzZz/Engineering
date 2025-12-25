# 🛠 Engineering Fundamentals

> A curated collection of core Data Structures and Algorithms, optimized for cross-platform development between **macOS** and **Windows**.

---

## 🏗 Repository Structure

This repository serves as a personal engineering handbook. Each directory contains implementations, complexity analysis, and use cases.

* **📂 DataStructures**: Implementations of Trees, Graphs, Linked Lists, and Stacks.
* **📂 SearchAlgorithms**: Binary search, BFS, DFS, and optimized lookup patterns.
* **📂 SortingAlgorithms**: QuickSort, MergeSort, and HeapSort implementations.

---

## 💻 Environment Syncing (PC & Mac)

Since this project is developed across multiple operating systems, the following configurations are in place to ensure consistency:

### ⚙️ Cross-Platform Configuration
To prevent issues with line endings (CRLF vs LF) between Windows and Mac, ensure your local git config is set:
```bash
# On Windows (PC)
git config --global core.autocrlf true

# On macOS (MacBook)
git config --global core.autocrlf input

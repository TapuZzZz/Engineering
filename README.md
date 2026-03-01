# ═══════════════════════════════════════════════════════════════════════════════
#                         🔧 ALGORITHM ENGINEERING GUIDE 🔧
# ═══════════════════════════════════════════════════════════════════════════════

> *A comprehensive reference for data structures, algorithms, and graph theory implementations*

---

## 📚 Data Structures

### Stack (LIFO - Last In, First Out)
```
    Push: 3      ┌─────┐
    Push: 2  →   │  3  │
    Push: 1      │  2  │
    Pop: 1       │  1  │
                 └─────┘
```
**Use Cases:** Function call stack, undo/redo, parentheses matching, DFS

---

### Queue (FIFO - First In, First Out)
```
    Enqueue: A   A → B → C → NULL
    Enqueue: B   
    Enqueue: C   (Head)
    Dequeue: A
```
**Use Cases:** BFS, task scheduling, printer queue, message handling

---

### Linked Lists
```
Linear Linked List (LLL)
    ┌─────┬───┐  ┌─────┬───┐  ┌─────┬───┐
    │ 1   │ •─┼─→│ 2   │ •─┼─→│ 3   │ ∅ │
    └─────┴───┘  └─────┴───┘  └─────┴───┘

Circular Linked List (CLLL)
    ┌─────┬───┐  ┌─────┬───┐  ┌─────┬───┐
    │ 1   │ •─┼─→│ 2   │ •─┼─→│ 3   │ •─┐
    └─────┴───┘  └─────┴───┘  └─────┴───┘
         ↑────────────────────────────────┘

Doubly Linked List (DLLL)
    ┌─────┬───┬───┐  ┌─────┬───┬───┐  ┌─────┬───┬───┐
    │ 1   │ ∅ │ •─┼─→│ 2   │ •─┼─→│ 3   │ •─┼─→│ ∅ │
    │     │ ←─┼─│   │     │ ←─┼─│   │     │ ←─┼─│
    └─────┴───┴───┘  └─────┴───┴───┘  └─────┴───┴───┘
```

---

### Trees

#### Binary Tree
```
           ┌─────┐
           │  1  │
         ┌─┴──┬──┴─┐
        ┌─┴─┐┌─┴─┐ │
        │ 2 ││3  │ │
       ┌┴─┐ ││    │ │
       │4 │ ││    │ │
       └──┘ └┘    └─┘
```
**Time Complexity:** O(log n) average, O(n) worst case

#### AVL Tree (Self-Balancing)
```
        ┌────┴────┐
        │    20   │ ↔ Height: 2
      ┌─┴─┐   ┌───┴─┐
      │10 │   │ 30  │ ↔ Height: 1
     ┌┴─┐ │    │  └──┴──┐
     │5 │ │   │15│    40│
```
**Time Complexity:** O(log n) guaranteed

#### K-ary Tree
```
         ┌──────┴──────┐
         │      1      │ K=3 (ternary)
    ┌────┼────┬──────┐
    │    │    │      │
   ┌2┐  ┌3┐  ┌4┐    ┌5┐
```

#### Heap (Binary Heap)
```
Min Heap              Max Heap
      1                    100
    ┌─┴─┐              ┌──┴──┐
    2   3              80    70
   ┌┴┐ ┌┴┐            ┌┴┐  ┌─┴┐
   4 5 6 7           50 40 30 20

Array: [1,2,3,4,5,6,7]    [100,80,70,50,40,30,20]
```
**Use Cases:** Priority queues, heapsort

---

## 🔄 Graph Theory

### Graph Representations
```
Adjacency Matrix          Adjacency List
  0 1 2 3                 0: [1, 2]
0[0 1 1 0]                1: [0, 3]
1[1 0 1 1]                2: [0, 3]
2[1 1 0 1]                3: [1, 2]
3[0 1 1 0]
```

### Graph Traversal

#### Breadth-First Search (BFS)
```
Start: 0
Level 0:  [0]
Level 1:  [1, 2]
Level 2:  [3, 4, 5]
Level 3:  [6, 7, 8]

Use: Shortest path, level order, connected components
```

#### Depth-First Search (DFS)
```
Start: 0
Path 1: 0 → 1 → 3 → (backtrack)
Path 2: 0 → 2 → 4 → 5 → (backtrack)

Use: Topological sort, cycle detection, connected components
```

### Shortest Path Algorithms

#### Dijkstra's Algorithm
```
Finds shortest path from source to all vertices
Time: O((V + E) log V)
Constraints: Non-negative weights only

     2      3
  A ──→ B ──→ C
  │           ↑
  │─────5─────│
```

#### Bellman-Ford Algorithm
```
Finds shortest path, handles negative weights
Time: O(V × E)
Use: Graphs with negative edge weights

     2      3
  A ──→ B ──→ C
  │          ↑
  │────-1────│
```

#### Floyd-Warshall Algorithm
```
All pairs shortest paths
Time: O(V³)
Space: O(V²)

Creates distance matrix for all vertex pairs
```

### Minimum Spanning Tree

#### Kruskal's Algorithm
```
1. Sort edges by weight
2. Add edge if no cycle formed
3. Continue until all vertices connected

Time: O(E log E)
```

#### Prim's Algorithm
```
1. Start from vertex
2. Add minimum weight edge connecting tree to new vertex
3. Repeat until all vertices in tree

Time: O(V²) or O(E log V)
```

### Topological Sort (DAG)
```
Prerequisites: 0 → 1 → 3
              0 → 2 → 3

Order: 0, 1/2, 2/1, 3

Use: Task scheduling, build systems, course prerequisites
```

---

## 📊 Sorting Algorithms

### Comparison-Based Sorts

| Algorithm | Best | Average | Worst | Space | Stable |
|-----------|------|---------|-------|-------|--------|
| **Bubble Sort** | O(n) | O(n²) | O(n²) | O(1) | ✓ |
| **Selection Sort** | O(n²) | O(n²) | O(n²) | O(1) | ✗ |
| **Insertion Sort** | O(n) | O(n²) | O(n²) | O(1) | ✓ |
| **Shell Sort** | O(n) | O(n log² n) | O(n²) | O(1) | ✗ |
| **Merge Sort** | O(n log n) | O(n log n) | O(n log n) | O(n) | ✓ |
| **Quick Sort** | O(n log n) | O(n log n) | O(n²) | O(log n) | ✗ |
| **Heap Sort** | O(n log n) | O(n log n) | O(n log n) | O(1) | ✗ |

### Non-Comparison Sorts

| Algorithm | Time | Space | Conditions |
|-----------|------|-------|-----------|
| **Counting Sort** | O(n + k) | O(k) | Non-negative integers |
| **Bucket Sort** | O(n + k) avg | O(n + k) | Uniformly distributed |
| **Radix Sort** | O(d × n) | O(n + k) | Fixed number of digits |

### Visualization Example: Merge Sort
```
[38, 27, 43, 3, 9, 82, 10]
        ↓ Divide
[38,27] [43,3] [9,82] [10]
        ↓ Merge
[27,38] [3,43] [9,82] [10]
        ↓ Merge Again
    [3,27,38,43]  [9,10,82]
        ↓ Final Merge
  [3,9,10,27,38,43,82]
```

---

## 🔍 Search Algorithms

### Binary Search
```
Sorted Array: [1, 3, 5, 7, 9, 11, 13, 15]
Search for 7:

  L: 0         M: 3         R: 7
  [1, 3, 5, 7, 9, 11, 13, 15]
           ↑
        Found!

Time Complexity: O(log n)
Space Complexity: O(1) or O(log n) recursive
```

---

## 📈 Complexity Analysis

### Big O Notation Hierarchy
```
O(1) ──────────────── Constant
      ↓
O(log n) ───────────── Logarithmic
      ↓
O(n) ───────────────── Linear
      ↓
O(n log n) ─────────── Linearithmic
      ↓
O(n²) ───────────────── Quadratic
      ↓
O(n³) ───────────────── Cubic
      ↓
O(2ⁿ) ───────────────── Exponential
      ↓
O(n!) ──────────────── Factorial
```

### Graph Complexity

| Operation | Adjacency Matrix | Adjacency List |
|-----------|-----------------|----------------|
| Add Edge | O(1) | O(1) |
| Remove Edge | O(1) | O(V) |
| Add Vertex | O(V²) | O(1) |
| Check Edge | O(1) | O(V) |
| Space | O(V²) | O(V + E) |

---

## 💡 Quick Reference

### When to Use Each Structure

```
🔹 Stack → Recursion, undo/redo, parsing
🔹 Queue → BFS, task scheduling
🔹 Linked List → Insertion/deletion heavy workloads
🔹 Tree → Hierarchical data, searching
🔹 Heap → Priority queue, k-th largest/smallest
🔹 Hash Table → Fast lookup, mapping
🔹 Graph → Networks, recommendations, shortest paths
```

### Algorithm Selection Guide

| Problem | Choose | Why |
|---------|--------|-----|
| **Shortest Path** | Dijkstra | Non-negative weights |
| **Shortest Path (negative)** | Bellman-Ford | Handles negatives |
| **All Pairs Shortest** | Floyd-Warshall | Complete info needed |
| **Minimum Span Tree** | Kruskal | Edge-based approach |
| **Task Order** | Topological Sort | DAG processing |
| **Quick Sorting** | Quick Sort | Average O(n log n) |
| **Stable Sort** | Merge Sort | Preserve order |
| **Small Arrays** | Insertion Sort | Low overhead |
| **Search Sorted** | Binary Search | O(log n) lookup |

---

## ✨ Key Takeaways

1. **Choose your tools wisely** - The right data structure determines efficiency
2. **Know your complexity** - Understand best/average/worst cases
3. **Trade-offs matter** - Time vs space, implementation vs performance
4. **Test edge cases** - Empty inputs, single elements, duplicates
5. **Profile first** - Optimize bottlenecks, not everything
6. **Document well** - Future you (and others) will thank you

---

```
╔═══════════════════════════════════════════════════════════════════════════════╗
║                     Happy Coding! 🚀 Build Beautiful Code                     ║
╚═══════════════════════════════════════════════════════════════════════════════╝
```

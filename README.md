# 🏥 Hospital Management System (High-Performance DSA in C11)

A robust, modular, and dependency-free hospital operations and clinical workflow management engine implemented from scratch in **pure C11**. Every clinical module is backed by a custom, purpose-built data structure and algorithm engineered for optimal time and space complexity.

[![Language](https://img.shields.io/badge/Language-C11-blue.svg)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Standard](https://img.shields.io/badge/Standard-POSIX-informational.svg)](https://pubs.opengroup.org/)
[![Sanitizers](https://img.shields.io/badge/Sanitizers-ASan%20%7C%20UBSan-brightgreen.svg)]()
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

---

## 🌟 Key Capabilities

- **⚡ Emergency Department Triage**: Max-heap prioritized by clinical severity score and sequence order with automated bed routing.
- **🫀 Multi-Organ Transplant Matching**: Urgency-weighted priority heaps with ABO blood compatibility filtering and ischemic viability evaluation.
- **🚑 Real-Time Ambulance Routing**: Weighted road graph evaluating least-time transit using Dijkstra's shortest path algorithm.
- **🛏️ Ward & Bed Allocation**: Constant-time ward indexing with dynamic FIFO waiting queues for overflowing admissions.
- **👩‍⚕️ Automated Nurse Staffing**: Round-robin circular queue enforcing labor regulations (maximum shifts, rest intervals, leave tracking).
- **🗂️ Zero-Collision Patient Directory**: Separate-chaining hash table with dynamic bucket lookups by ID and prefix name search.
- **↩️ Multi-Level Transaction Undo**: LIFO stack preserving system state transitions across admits, discharges, and reassignment cascades.
- **💾 Lightweight Disk Persistence**: Safe serialization and deserialization engine for state preservation across sessions.

---

## 🧠 Custom Data Structures & Algorithmic Complexities

| Module | Purpose | Underlying Data Structure | Key Operations | Time Complexity |
| :--- | :--- | :--- | :--- | :--- |
| [`src/patient.c`](src/patient.c) | Patient Registry | **Hash Table (Separate Chaining)** | Lookup, Insertion, Deletion | $\mathcal{O}(1)$ avg |
| [`src/triage.c`](src/triage.c) | ER Emergency Triage | **Binary Max-Heap** | Priority Push, Peek, Extract-Max | $\mathcal{O}(\log n)$ |
| [`src/transplant.c`](src/transplant.c) | Organ Allocation | **Composite Priority Heap** | Urgency enqueue, ABO Match & Pop | $\mathcal{O}(k \log n)$ |
| [`src/beds.c`](src/beds.c) | Bed Management | **Fixed Arrays + FIFO Linked Queue** | Bed lookup, Waitlist Enqueue | $\mathcal{O}(1)$ queue / $\mathcal{O}(\text{beds})$ |
| [`src/ambulance.c`](src/ambulance.c) | Route Dispatch | **Weighted Graph (Dijkstra)** | Shortest route calculation | $\mathcal{O}(V^2)$ |
| [`src/nurse.c`](src/nurse.c) | Shift Scheduling | **Circular Queue (Round-Robin)** | Fair cyclic shift assignment | $\mathcal{O}(D \cdot S \cdot N)$ |
| [`src/undo.c`](src/undo.c) | Operational Audit & Undo | **LIFO Stack (Linked List)** | Action push, reverse rollback | $\mathcal{O}(1)$ |

---

## 🔬 Deep Dive: System Architecture

### 1. Emergency Triage (Max-Heap)
Patients admitted to ER are scored with a clinical severity index ($1 \le \text{severity} \le 10$). The max-heap prioritizes urgent cases. When severity ties occur, arrival order timestamps preserve fair FIFO discipline. Popping the triage heap automatically triggers ward admission based on clinical thresholds:
- **Severity 8–10**: Admitted directly to Intensive Care Unit (ICU).
- **Severity 5–7**: Admitted to Emergency Ward.
- **Severity 1–4**: Admitted to General Ward.
- *If capacity is reached, the patient is seamlessly queued into the ward's waiting list.*

### 2. Organ Matching & Viability Filtering
Organ waitlists calculate composite urgency scores:
$$\text{Priority} = (\text{Urgency Score} \times 10{,}000) + \text{Wait Time (Days)}$$
When a donor organ becomes available:
1. Shortest transit time from donor location to Central Hospital is calculated via Dijkstra.
2. Viability limits are checked (Heart $\le 6\text{h}$, Liver $\le 12\text{h}$).
3. The heap is popped iteratively to locate the highest-urgency candidate matching donor ABO blood compatibility rules. Temporarily skipped candidates are pushed back without losing rank.

### 3. Fair Nurse Scheduling (Circular Queue)
Hospital shift rotation operates over 3 daily 8-hour windows (Morning `06:00-14:00`, Evening `14:00-22:00`, Night `22:00-06:00`) across 7 days. Eligible nurses are popped from the head of the circular queue, evaluated for eligibility (minimum 12 hours rest, max 40 hours/week, not on leave), assigned, and cycled to the tail.

---

## 🛠️ Build and Compilation

### Prerequisites
- Clang or GCC (`C11` standard compliant)
- Make (optional, direct invocation supported)

### Standard Compilation
```bash
make clean && make
```
Run the executable:
```bash
./hospital
```

### Strict Diagnostic & Sanitizer Build (ASan / UBSan)
For rigorous memory safety verification (detecting buffer overruns, leaks, and undefined operations):
```bash
make CFLAGS="-std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -g -Iinclude" clean all
./hospital
```

### Build Without Make
```bash
mkdir -p build
cc -std=c11 -Wall -Wextra -O2 -Iinclude src/*.c -o hospital
./hospital
```

---

## 💻 Interactive CLI Workflow & Demo

Launch `./hospital` and interact through the unified terminal menu:

```text
==============================================
   HOSPITAL MANAGEMENT SYSTEM  (DSA project)
==============================================
Tip: choose option 8 to load demo data.

=========== MAIN MENU ===========
  1. Patient records      (hash table)
  2. Emergency triage     (max-heap)
  3. Beds and wards       (queue)
  4. Organ transplant     (heap)
  5. Ambulance routing    (graph, Dijkstra)
  6. Nurse scheduling     (circular queue)
  7. Undo / audit log     (stack)
  8. Load demo data
  0. Save and exit
```

### Quick Verification Walkthrough:
1. **Load Demo Suite**: Press `8` to populate mock clinical patients, triage cases, liver/heart candidate queues, and nurse rosters.
2. **Execute Emergency Triage**: Choose `2` $\rightarrow$ Call next patient. Watch the highest severity patient get dequeued and placed into ICU or Emergency beds.
3. **Inspect Bed Occupancy**: Choose `3` $\rightarrow$ Show beds and waiting queue.
4. **Trigger Organ Allocation**: Choose `4` $\rightarrow$ Option `4` (Organ available) $\rightarrow$ Select Heart/Liver and enter donor blood group. Observe Dijkstra-backed viability validation and ABO matching.
5. **Simulate Ambulance Dispatch**: Choose `5` $\rightarrow$ Find shortest route between city nodes.
6. **Rollback Operations**: Choose `7` $\rightarrow$ Option `1` (Undo last action) to reverse bed admissions or discharges atomically.
7. **Exit & Persist**: Select `0` to write application state to binary/text persistence storage (`patients.dat`, `nurses.dat`, `waitlist.dat`).

---

## 📂 Project Structure

```
hospital_management/
├── include/                # Modular C header definitions
│   ├── ambulance.h         # Graph & Dijkstra routing structures
│   ├── beds.h              # Ward allocation & FIFO queue structures
│   ├── hospital.h          # Core orchestration facade
│   ├── nurse.h             # Circular queue & nurse scheduling models
│   ├── patient.h           # Hash table & patient record definitions
│   ├── storage.h           # File I/O persistence protocols
│   ├── transplant.h        # Composite priority heap models
│   ├── triage.h            # Binary max-heap triage structures
│   ├── undo.h              # LIFO stack action log
│   └── util.h              # String, input, and numeric utilities
├── src/                    # Implementations
│   ├── ambulance.c
│   ├── beds.c
│   ├── hospital.c
│   ├── main.c              # Terminal user interface & control loop
│   ├── nurse.c
│   ├── patient.c
│   ├── storage.c
│   ├── transplant.c
│   ├── triage.c
│   ├── undo.c
│   └── util.c
├── Makefile                # Dependency-tracked build configuration
├── .gitignore              # Build artifact & persistence exclusion
└── README.md               # Technical project documentation
```

---

## 📄 License
This project is licensed under the MIT License.

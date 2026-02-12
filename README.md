# ChessBot
**Tactical Chess Engine with Search Optimization**

A performance-focused chess engine built using classical AI search techniques, optimized board evaluation, and modular move generation. Designed for competitive play and extensibility.

**🔹 Project Summary**

* Built a chess engine from scratch using C++ with efficient board representation.

* Implemented Minimax with Alpha-Beta pruning for optimized decision making.

* Designed a custom static evaluation function based on material balance, positional heuristics, and mobility.

* Integrated legal move validation with check detection and state management.

* Optimized performance using pruning, move ordering, and depth control.

**🧠 Problem Overview**

* Chess engines require:

* Efficient state representation

* Fast legal move generation

* Optimal search strategy

* Reliable position evaluation

* The challenge lies in balancing:

* Search depth vs computation time

* Tactical sharpness vs positional understanding

**Correctness vs performance**

* A naïve brute-force approach quickly becomes infeasible due to the exponential branching factor (~35 moves per position).

**🎯 Project Objective**

* Bot treats chess as a deterministic search problem with heuristic evaluation.

  *Input*

  - Current board state

  - Active player

  - Search depth

  *Output*

  - Best possible move

  - Evaluation score of resulting position

  - The engine ensures:

  - Legal move generation

  - Checkmate and stalemate detection

  - Valid state transitions

**💡 Core Design Philosophy**

* Search smart, prune aggressively, evaluate efficiently.

* Instead of exploring all possibilities:

* Prune irrelevant branches early using Alpha-Beta

* Evaluate only meaningful board states

* Use move ordering to maximize pruning efficiency

* The system is modular — evaluation, search, and board logic are separated for easy upgrades.

**🧩 System Approach**

  - VaultMate follows a Search → Evaluate → Prune pipeline.

**1️⃣ Board Representation**

  - 8×8 matrix representation

  - Piece encoding using enums / integers

  - Turn-based state tracking

  - Move history (for undo functionality)

**Supports:**

  - Castling

  - Pawn promotion

  - En passant

  - Check detection

**2️⃣ Move Generation Engine**

  - Generates all pseudo-legal moves

  - Filters illegal moves (leaving king in check)

**Categorizes:**

  - Captures

  - Quiet moves

  - Special moves

  - Optimized to reduce redundant computations.

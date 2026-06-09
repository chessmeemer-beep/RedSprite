# RedSprite Chess Engine

## Architecture Overview

RedSprite is a modern, high-performance chess engine featuring a unique multi-head evaluation system powered by NNUE (Efficiently Updatable Neural Network).

### Core Features

1. **Bitboard Representation**: 64-bit integers for efficient piece position tracking
2. **Move Generation**: Fast legal move generation using bitboard operations
3. **Search Algorithms**: 
   - Alpha-beta pruning
   - Iterative deepening
   - Quiescence search
   - Null move pruning
   - Late Move Reduction (LMR)
4. **Transposition Table**: Zobrist hashing with efficient lookup
5. **Multi-threading**: Parallel search with work stealing
6. **NNUE Evaluation**: Efficiently updatable neural network

### Unique Multi-Head Architecture

RedSprite features 8 specialized evaluation heads:

1. **Tactical Head**: Evaluates immediate tactical opportunities
2. **Strategic Head**: Long-term positional planning
3. **Positional Head**: Static positional advantages
4. **Opening Head**: Opening theory and development
5. **Endgame Head**: Endgame-specific knowledge
6. **King Safety Head**: King security assessment
7. **Prediction Head**: Opponent threat prediction
8. **Material Head**: Material balance and piece values

### Fusion Layer

A learned fusion layer combines all head outputs into a final evaluation score.

## Folder Structure

```
RedSprite/
├── include/
│   ├── bitboard.h
│   ├── board.h
│   ├── move.h
│   ├── search.h
│   ├── evaluation.h
│   ├── nnue.h
│   ├── transposition.h
│   └── utils.h
├── src/
│   ├── core/
│   │   ├── bitboard.cpp
│   │   ├── board.cpp
│   │   └── move.cpp
│   ├── search/
│   │   ├── alphabeta.cpp
│   │   ├── iterative_deepening.cpp
│   │   ├── quiescence.cpp
│   │   └── mtt.cpp
│   ├── evaluation/
│   │   ├── evaluator.cpp
│   │   ├── heads.cpp
│   │   └── fusion.cpp
│   ├── nnue/
│   │   ├── network.cpp
│   │   ├── layers.cpp
│   │   └── trainer.cpp
│   ├── utils/
│   │   ├── zobrist.cpp
│   │   ├── timer.cpp
│   │   └── thread_pool.cpp
│   └── heads/
│       ├── tactical.cpp
│       ├── strategic.cpp
│       ├── positional.cpp
│       ├── opening.cpp
│       ├── endgame.cpp
│       ├── king_safety.cpp
│       ├── prediction.cpp
│       └── material.cpp
├── tests/
│   ├── test_bitboard.cpp
│   ├── test_movegen.cpp
│   └── test_search.cpp
├── training/
│   ├── generate_data.py
│   ├── train_nnue.py
│   └── validate.py
├── data/
│   └── networks/
├── CMakeLists.txt
└── README.md
```

## Class Diagram

```
┌─────────────────┐     ┌──────────────────┐     ┌─────────────────┐
│   Bitboard      │────▶│      Board       │────▶│      Move       │
└─────────────────┘     └──────────────────┘     └─────────────────┘
                              │                        │
                              ▼                        ▼
                      ┌──────────────────┐     ┌─────────────────┐
                      │    Searcher      │────▶│  MoveGenerator  │
                      └──────────────────┘     └─────────────────┘
                              │
                              ▼
                      ┌──────────────────┐
                      │   Evaluator      │
                      └──────────────────┘
                              │
              ┌───────────────┼───────────────┐
              ▼               ▼               ▼
      ┌─────────────┐ ┌─────────────┐ ┌─────────────┐
      │  NNUE Net   │ │  Head[8]    │ │  Fusion     │
      └─────────────┘ └─────────────┘ └─────────────┘
```

## Data Flow

1. **Input Position** → Board representation with bitboards
2. **Move Generation** → Legal moves via bitboard operations
3. **Search** → Alpha-beta with iterative deepening
4. **Evaluation** → Multi-head NNUE evaluation
5. **Fusion** → Combine head outputs
6. **Output** → Best move and evaluation score

## Performance Optimizations

1. **SIMD Instructions**: AVX2/AVX-512 for NNUE computations
2. **Cache Optimization**: Data locality for transposition table
3. **Parallel Search**: Lock-free work stealing
4. **Incremental Updates**: NNUE feature updates without full recomputation
5. **Prefetching**: Anticipatory data loading

# RedSprite Chess Engine

## Architecture Overview

RedSprite is a modern UCI chess engine featuring a unique multi-head evaluation system combined with NNUE technology.

### Core Components

```
┌─────────────────────────────────────────────────────────────────┐
│                        UCI Interface                             │
├─────────────────────────────────────────────────────────────────┤
│                         Search Layer                             │
│  ┌─────────────┐  ┌──────────────┐  ┌──────────────────────┐   │
│  │Alpha-Beta   │  │Iterative     │  │Move Ordering         │   │
│  │Search       │  │Deepening     │  │(TT, Killers, History)│   │
│  └─────────────┘  └──────────────┘  └──────────────────────┘   │
├─────────────────────────────────────────────────────────────────┤
│                      Evaluation Layer                            │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │              Multi-Head Fusion Layer                     │    │
│  ├─────────┬─────────┬──────────┬─────────┬───────────────┤    │
│  │Tactical │Strategic│Positional│Opening  │Endgame        │    │
│  │Head     │Head     │Head      │Head     │Head           │    │
│  ├─────────┼─────────┼──────────┼─────────┼───────────────┤    │
│  │King     │Prediction│          NNUE       │               │    │
│  │Safety   │Head     │          Network    │               │    │
│  └─────────┴─────────┴──────────┴─────────┴───────────────┘    │
├─────────────────────────────────────────────────────────────────┤
│                       Move Generation                            │
│  ┌─────────────┐  ┌──────────────┐  ┌──────────────────────┐   │
│  │Bitboard     │  │Legal Move    │  │Attack Generation     │   │
│  │Operations   │  │Validation    │  │(Sliding, Stepping)   │   │
│  └─────────────┘  └──────────────┘  └──────────────────────┘   │
├─────────────────────────────────────────────────────────────────┤
│                        Position                                  │
│  ┌─────────────┐  ┌──────────────┐  ┌──────────────────────┐   │
│  │Zobrist      │  │FEN Parsing   │  │Make/Unmake Moves     │   │
│  │Hashing      │  │              │  │                      │   │
│  └─────────────┘  └──────────────┘  └──────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

## Unique Features: Multi-Head Evaluation

### 1. Tactical Head
- Material counting with piece values
- Threat detection
- Capture analysis
- High confidence (0.9) for concrete calculations

### 2. Strategic Head
- Pawn structure evaluation
- Space advantage
- Piece activity metrics
- Long-term planning

### 3. Positional Head
- Piece-square tables
- Outpost identification
- Weak square exploitation
- Optimal piece placement

### 4. Opening Head
- Development bonus
- Center control
- Early castling incentives
- Phase-dependent weighting

### 5. Endgame Head
- King centralization
- Passed pawn advancement
- Opposition and triangulation
- Promotion chances

### 6. King Safety Head
- Pawn shield analysis
- Attacking pieces near king
- Escape squares
- Mating net detection

### 7. Prediction Head
- Opponent response prediction
- Danger assessment
- Neural network-based outcome prediction

### Fusion Layer
All heads output scores with confidence values that are combined using weighted averaging:

```
final_score = Σ(head_score × confidence × weight) / Σ(confidence × weight)
```

## Folder Structure

```
RedSprite/
├── CMakeLists.txt
├── README.md
├── docs/
│   ├── architecture.md
│   ├── evaluation.md
│   └── training.md
├── src/
│   ├── core/
│   │   ├── types.h
│   │   ├── bitboard.h
│   │   ├── bitboard.cpp
│   │   ├── position.h
│   │   └── position.cpp
│   ├── movegen/
│   │   ├── move.h
│   │   └── move.cpp
│   ├── eval/
│   │   ├── nnue.h
│   │   └── nnue.cpp
│   ├── search/
│   │   ├── search.h
│   │   └── search.cpp
│   ├── ttable/
│   │   ├── tt.h
│   │   └── tt.cpp
│   └── uci/
│       ├── uci.h
│       └── uci.cpp
└── tests/
    └── perft_test.cpp
```

## Building

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

## Usage

```bash
./redsprite
```

UCI Commands:
- `uci` - Initialize UCI protocol
- `position startpos` - Set starting position
- `position fen <fen>` - Set custom position
- `go depth 10` - Search to depth 10
- `go movetime 1000` - Search for 1 second
- `stop` - Stop searching
- `eval` - Show current evaluation
- `quit` - Exit

## Performance Optimizations

1. **Bitboard Operations**: All piece locations stored as 64-bit integers
2. **Magic Bitboards**: Fast sliding piece attack generation
3. **Incremental Updates**: Zobrist hash and NNUE accumulators updated incrementally
4. **SIMD Instructions**: AVX2/BMI2 for parallel processing
5. **Cache-Friendly TT**: Power-of-2 sized transposition table
6. **Move Ordering**: TT moves, killers, history heuristic for better pruning

## Training System

The NNUE weights can be trained using:
1. Self-play games
2. Position datasets from master games
3. Gradient descent optimization
4. Head weight tuning via evolutionary algorithms

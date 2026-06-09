#include "core/position.h"
#include "movegen/move.h"
#include <iostream>
#include <cstring>

using namespace RedSprite;

// Perft (Performance Test) positions
const char* startpos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// Known perft results for verification
struct PerftResult {
    const char* fen;
    int depth;
    uint64_t expected_nodes;
};

// Standard perft positions with known results
PerftResult test_positions[] = {
    {startpos, 1, 20},
    {startpos, 2, 400},
    {startpos, 3, 8902},
    {startpos, 4, 197281},
    // Kiwipete position
    {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 1, 48},
    {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 2, 2039},
    // En passant test
    {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 1, 14},
    {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 2, 191},
    // Castling test
    {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 1, 6},
    {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 2, 264},
};

uint64_t perft(Position& pos, int depth) {
    if (depth == 0) {
        return 1;
    }
    
    MoveGenerator mg(&pos);
    ScoredMove moves[MAX_MOVES];
    int count = mg.generate_legal(moves);
    
    if (depth == 1) {
        return count;
    }
    
    uint64_t nodes = 0;
    
    for (int i = 0; i < count; ++i) {
        Move m = moves[i].move;
        
        Piece captured = pos.piece_at(m.to_sq());
        bool was_promotion = (m.type() == MOVE_PROMOTION);
        PieceType promo_type = m.promotion_type();
        
        pos.make_move(m);
        nodes += perft(pos, depth - 1);
        pos.unmake_move(m, captured, was_promotion, promo_type);
    }
    
    return nodes;
}

void perft_div(Position& pos, int depth) {
    MoveGenerator mg(&pos);
    ScoredMove moves[MAX_MOVES];
    int count = mg.generate_legal(moves);
    
    uint64_t total = 0;
    
    for (int i = 0; i < count; ++i) {
        Move m = moves[i].move;
        
        Piece captured = pos.piece_at(m.to_sq());
        bool was_promotion = (m.type() == MOVE_PROMOTION);
        PieceType promo_type = m.promotion_type();
        
        pos.make_move(m);
        uint64_t nodes = perft(pos, depth - 1);
        pos.unmake_move(m, captured, was_promotion, promo_type);
        
        total += nodes;
        
        // Print move and nodes
        std::cout << static_cast<char>('a' + file_of(m.from_sq()))
                  << static_cast<char>('1' + rank_of(m.from_sq()))
                  << static_cast<char>('a' + file_of(m.to_sq()))
                  << static_cast<char>('1' + rank_of(m.to_sq()));
        
        if (m.type() == MOVE_PROMOTION) {
            switch (m.promotion_type()) {
                case KNIGHT: std::cout << 'n'; break;
                case BISHOP: std::cout << 'b'; break;
                case ROOK: std::cout << 'r'; break;
                case QUEEN: std::cout << 'q'; break;
                default: break;
            }
        }
        
        std::cout << ": " << nodes << std::endl;
    }
    
    std::cout << "\nTotal: " << total << std::endl;
}

bool run_perft_test(const PerftResult& test) {
    Position pos;
    pos.set_from_fen(test.fen);
    
    std::cout << "Testing: " << test.fen << " at depth " << test.depth << std::endl;
    
    auto start = std::chrono::steady_clock::now();
    uint64_t result = perft(pos, test.depth);
    auto end = std::chrono::steady_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    std::cout << "Result: " << result << " (expected: " << test.expected_nodes << ") ";
    
    if (result == test.expected_nodes) {
        std::cout << "PASS";
        if (duration > 0) {
            std::cout << " (" << (result * 1000 / duration) << " nps)";
        }
        std::cout << std::endl;
        return true;
    } else {
        std::cout << "FAIL" << std::endl;
        std::cout << "Running perft_div for debugging:" << std::endl;
        perft_div(pos, test.depth);
        return false;
    }
}

int main(int argc, char* argv[]) {
    // Initialize tables
    AttackGenerator::init();
    ZobristHash::init();
    
    std::cout << "RedSprite Perft Test" << std::endl;
    std::cout << "====================" << std::endl << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    // Run specific test if depth provided
    if (argc >= 3) {
        std::string fen = argv[1];
        int depth = std::stoi(argv[2]);
        
        Position pos;
        pos.set_from_fen(fen.c_str());
        
        std::cout << "Running perft_div on:" << std::endl;
        std::cout << fen << std::endl;
        std::cout << "Depth: " << depth << std::endl << std::endl;
        
        perft_div(pos, depth);
        return 0;
    }
    
    // Run all standard tests
    for (const auto& test : test_positions) {
        if (run_perft_test(test)) {
            passed++;
        } else {
            failed++;
        }
        std::cout << std::endl;
    }
    
    std::cout << "====================" << std::endl;
    std::cout << "Results: " << passed << " passed, " << failed << " failed" << std::endl;
    
    return failed > 0 ? 1 : 0;
}

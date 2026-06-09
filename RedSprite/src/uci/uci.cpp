#include "uci.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace RedSprite {

UCIEngine::UCIEngine() : search(nullptr) {
    tt.resize(64); // Default 64MB TT
    position.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

UCIEngine::~UCIEngine() {
    if (searching.load()) {
        if (search) search->stop();
        if (search_thread.joinable()) search_thread.join();
    }
    delete search;
}

bool UCIEngine::parse_move(const std::string& move_str, Move& m) {
    if (move_str.length() < 4 || move_str.length() > 5) return false;
    
    int from_file = move_str[0] - 'a';
    int from_rank = move_str[1] - '1';
    int to_file = move_str[2] - 'a';
    int to_rank = move_str[3] - '1';
    
    if (from_file < 0 || from_file > 7 || from_rank < 0 || from_rank > 7 ||
        to_file < 0 || to_file > 7 || to_rank < 0 || to_rank > 7) {
        return false;
    }
    
    Square from = make_square(from_file, from_rank);
    Square to = make_square(to_file, to_rank);
    
    PieceType promo = NO_PIECE_TYPE;
    MoveType type = MOVE_NORMAL;
    
    if (move_str.length() == 5) {
        char p = move_str[4];
        switch (p) {
            case 'n': promo = KNIGHT; break;
            case 'b': promo = BISHOP; break;
            case 'r': promo = ROOK; break;
            case 'q': promo = QUEEN; break;
            default: return false;
        }
        type = MOVE_PROMOTION;
    }
    
    // Check for castling notation
    if (position.piece_at(from) == W_KING || position.piece_at(from) == B_KING) {
        if (from == SQ_E1 && to == SQ_G1) type = MOVE_CASTLING;
        else if (from == SQ_E1 && to == SQ_C1) type = MOVE_CASTLING;
        else if (from == SQ_E8 && to == SQ_G8) type = MOVE_CASTLING;
        else if (from == SQ_E8 && to == SQ_C8) type = MOVE_CASTLING;
    }
    
    m = Move(from, to, type, promo);
    return true;
}

void UCIEngine::position_cmd(const std::string& cmd) {
    std::istringstream iss(cmd);
    std::string token;
    iss >> token; // "position"
    
    std::string fen;
    bool has_moves = false;
    
    iss >> token;
    if (token == "startpos") {
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        position.set_from_fen(fen.c_str());
    } else if (token == "fen") {
        // Read FEN tokens
        std::string fen_part;
        while (iss >> fen_part && fen_part != "moves") {
            fen += fen_part + " ";
        }
        position.set_from_fen(fen.c_str());
        
        if (fen_part == "moves") {
            has_moves = true;
        }
    }
    
    // Process moves
    if (has_moves || token == "moves") {
        if (!has_moves) {
            iss >> token; // Skip "moves"
        }
        
        std::string move_str;
        while (iss >> move_str) {
            Move m;
            if (parse_move(move_str, m)) {
                Piece captured = position.piece_at(m.to_sq());
                bool was_promotion = (m.type() == MOVE_PROMOTION);
                PieceType promo_type = m.promotion_type();
                
                position.make_move(m);
            }
        }
    }
}

void UCIEngine::go(const std::string& cmd) {
    if (searching.load()) {
        if (search) search->stop();
        if (search_thread.joinable()) search_thread.join();
    }
    
    delete search;
    search = new Search(tt);
    
    SearchLimits limits;
    limits.infinite = false;
    limits.depth = -1;
    limits.time_ms = -1;
    limits.nodes = 0;
    
    std::istringstream iss(cmd);
    std::string token;
    iss >> token; // "go"
    
    while (iss >> token) {
        int value;
        if (token == "depth" && (iss >> value)) {
            limits.depth = value;
        } else if (token == "wtime" && position.get_side_to_move() == WHITE && (iss >> value)) {
            limits.time_ms = value;
            limits.move_time = value / 20; // Simple time management
        } else if (token == "btime" && position.get_side_to_move() == BLACK && (iss >> value)) {
            limits.time_ms = value;
            limits.move_time = value / 20;
        } else if (token == "movetime" && (iss >> value)) {
            limits.time_ms = value;
            limits.move_time = value;
        } else if (token == "nodes" && (iss >> value)) {
            limits.nodes = value;
        } else if (token == "infinite" || token == "ponder") {
            limits.infinite = true;
        }
    }
    
    // Default depth if not specified
    if (limits.depth < 0 && limits.time_ms < 0 && !limits.infinite && limits.nodes == 0) {
        limits.depth = 10;
    }
    
    search->set_limits(limits);
    
    searching.store(true);
    search_thread = std::thread([this]() {
        Move best = search->search(position, search->limits.depth, search->limits.time_ms);
        searching.store(false);
        send_bestmove(best);
    });
}

void UCIEngine::send_info(const std::string& info) {
    std::cout << info << std::endl;
}

void UCIEngine::send_bestmove(Move m) {
    if (!m.is_valid()) {
        std::cout << "bestmove 0000" << std::endl;
        return;
    }
    
    std::ostringstream oss;
    oss << "bestmove ";
    oss << static_cast<char>('a' + file_of(m.from_sq()));
    oss << static_cast<char>('1' + rank_of(m.from_sq()));
    oss << static_cast<char>('a' + file_of(m.to_sq()));
    oss << static_cast<char>('1' + rank_of(m.to_sq()));
    
    if (m.type() == MOVE_PROMOTION) {
        switch (m.promotion_type()) {
            case KNIGHT: oss << 'n'; break;
            case BISHOP: oss << 'b'; break;
            case ROOK: oss << 'r'; break;
            case QUEEN: oss << 'q'; break;
            default: break;
        }
    }
    
    std::cout << oss.str() << std::endl;
}

void UCIEngine::handle_command(const std::string& cmd) {
    std::istringstream iss(cmd);
    std::string token;
    iss >> token;
    
    if (token == "uci") {
        std::cout << "id name RedSprite" << std::endl;
        std::cout << "id author RedSprite Team" << std::endl;
        std::cout << "option name Hash type spin default 64 min 1 max 2048" << std::endl;
        std::cout << "option name Threads type spin default 1 min 1 max 256" << std::endl;
        std::cout << "option name NNUE type check default true" << std::endl;
        std::cout << "uciok" << std::endl;
    }
    else if (token == "isready") {
        std::cout << "readyok" << std::endl;
    }
    else if (token == "ucinewgame") {
        tt.clear();
        position.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        if (search) {
            delete search;
            search = nullptr;
        }
    }
    else if (token == "position") {
        position_cmd(cmd);
    }
    else if (token == "go") {
        go(cmd);
    }
    else if (token == "stop") {
        if (search) search->stop();
    }
    else if (token == "quit") {
        if (search) search->stop();
        if (search_thread.joinable()) search_thread.join();
        exit(0);
    }
    else if (token == "eval") {
        MultiHeadEvaluator eval;
        int score = eval.evaluate(position);
        std::cout << "Evaluation: " << score << " centipawns" << std::endl;
    }
    else if (token == "d") {
        std::cout << position.get_fen() << std::endl;
    }
}

void UCIEngine::run() {
    std::string line;
    while (std::getline(std::cin, line)) {
        handle_command(line);
    }
}

} // namespace RedSprite

int main() {
    // Initialize static tables
    RedSprite::AttackGenerator::init();
    RedSprite::ZobristHash::init();
    
    RedSprite::UCIEngine engine;
    engine.run();
    
    return 0;
}

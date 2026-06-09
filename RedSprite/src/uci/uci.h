#pragma once

#include "core/position.h"
#include "search/search.h"
#include "ttable/tt.h"
#include <string>
#include <thread>
#include <atomic>

namespace RedSprite {

class UCIEngine {
private:
    Position position;
    TranspositionTable tt;
    Search* search;
    
    std::thread search_thread;
    std::atomic<bool> searching{false};
    
    bool parse_move(const std::string& move_str, Move& m);
    void go(const std::string& cmd);
    void position_cmd(const std::string& cmd);
    
public:
    UCIEngine();
    ~UCIEngine();
    
    // Main UCI loop
    void run();
    
    // Handle UCI commands
    void handle_command(const std::string& cmd);
    
    // Send info to GUI
    void send_info(const std::string& info);
    
    // Send bestmove
    void send_bestmove(Move m);
    
    // Get current position
    const Position& get_position() const { return position; }
};

} // namespace RedSprite

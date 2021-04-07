#include <string>
#include <pthread.h>
#include <iostream>
#include <cassert>
#include "board.h"
#include "search.h"
#include "defs.h"
#include "tt.h"

// Commands we get:
// * uci
// * debug
// * isready
// * setoption name [value]
// * register
// * ucinewgame
// * position [fen | startpos] moves ...
// * go
// * stop
// * ponderhit
// * quit

struct Engine {
    Board board;
    // TranspositionTable tt;    
    bool stop_search, is_searching; // is_pondering;
    Move best_move;

    Engine() {
        board = Board();
        tt.allocate(128);
    }

    void set_position() {
        board = Board();
        tt.clear();
    }

    void set_position(const std::string& fen) {
        board = Board(fen);
        tt.clear();
    }

    void search() {
        stop_search = false;
        best_move = NULL_MOVE;
        best_move = think(board, &stop_search);
        assert(stop_search == true);
    }
};

// Engine engine; // engine will be a global object

enum {
   MOVETIME = 0,
   INFINITE = 1
};

struct GoStruct {
    Engine* engine;
    int param_name;
    int param_value;
};

std::vector<std::string> split(const std::string &str) {
    std::vector<std::string> tokens;
    std::string::size_type start = 0;
    std::string::size_type end = 0;
    while ((end = str.find(" ", start)) != std::string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + 1;
    }
    tokens.push_back(str.substr(start));
    return tokens;
}

void* process_go(void* _go_struct) {
    GoStruct* go_struct = (GoStruct*)_go_struct;
    assert(go_struct->param_name == MOVETIME);
    go_struct->engine->is_searching = true;
    go_struct->engine->search();
    if(go_struct->engine->is_searching) {
        assert(go_struct->engine->best_move != NULL_MOVE);
        // send statistics...
        std::cout << "bestmove " << move_to_str(go_struct->engine->best_move) << endl;
    }
    go_struct->engine->is_searching = false;
}

void parse_option(const std::vector<std::string>& args) {
    cout << "Parsing some option..." << endl;
}

void uci() {
    freopen("log.txt", "w", stderr);

	std::string read_uci;
	cin >> read_uci;
    cerr << read_uci << endl;
	assert(read_uci == "uci");

    std::cout << "id name Dratini" << endl;
    std::cout << "id author Oscar Balcells" << endl;
    std::cout << "uciok" << endl;
    Engine engine = Engine();
    pthread_t pthread_go;
    std::string line, command;
    std::vector<std::string> args;

    while(true) {
        // cout << "Waiting to read something..." << endl;
        getline(cin, line);

        if(line.size() <= 1) {
            continue;
        }

        cerr << line << endl;

        // cin >> line;
        if(line.size() == 0) {
            std::cout << "error" << endl;
            return;
        }
        args = split(line);
        command = args[0];

        if(command == "debug") {
            if(args.size() == 2 && args[1] == "on") {
                // engine.debug_mode = true;
            } else if(args.size() == 2 && args[1] == "off") {
                // engine.debug_mode = false;
            } else {
                // engine.debug_mode = !engine.debug_mode;
            }
        } else if(command == "isready") {
            // assert(engine.is_ready);
            std::cout << "readyok" << endl;
        } else if(command == "setoption") {
            parse_option(args);
        } else if(command == "register") {
            cout << "you don't have to register" << endl;
            assert(false);
        } else if(command == "ucinewgame") {
            engine.set_position();
        } else if(command == "position") {
            engine.is_searching = false; // stop the search and don't return bestmove
            if(args[1] == "startpos") {
                engine.set_position(); // default position
            } else {
                engine.set_position(args[1]); // args[1] is the fen
            }
            if(args[2] == "moves") {
                for(int i = 3; i < (int)args.size(); i++) {
                    if(!engine.board.make_move_from_str(args[i])) {
                        cout << "There was an error making move " << args[i] << endl;
                        return;
                    }
                }
            }
        } else if(command == "go") {
            int param_name = MOVETIME, param_value = 0;
            GoStruct* go_struct = (GoStruct*)malloc(sizeof(GoStruct));
            go_struct->engine = &engine;
            go_struct->param_name = MOVETIME;
            go_struct->param_value = 0;
            pthread_create(&pthread_go, NULL, &process_go, go_struct);
        } else if(command == "stop") {
            engine.stop_search = true;
        } else if(command == "ponderhit") {
            // engine.is_pondering = false;
        } else if(command == "quit") {
            return;
        }
    }
}
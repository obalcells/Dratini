#include <string>
#include <pthread.h>
#include <iostream>
#include <cassert>
#include "board.h"
#include "search.h"
#include "defs.h"
#include "tt.h"
#include "engine.h"

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

static std::vector<std::string> split(const std::string &str) {
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
    // GoStruct* go_struct = (GoStruct*)_go_struct;
    // assert(go_struct->param_name == MOVETIME);
    // go_struct->engine->is_searching = true;
    // think(*go_struct->engine);
    // if(go_struct->engine->is_searching) {
    //     assert(go_struct->engine->best_move != NULL_MOVE);
    //     // send statistics...
    //     cout << "bestmove " << move_to_str(go_struct->engine->best_move) << " ponder " << move_to_str(go_struct->engine->ponder_move) << endl;
    //     cerr << "out: bestmove " << move_to_str(go_struct->engine->best_move) << " ponder " << move_to_str(go_struct->engine->ponder_move) << endl;
    // }
    // go_struct->engine->is_searching = false;
    engine.is_searching = true;
    engine.stop_search = false;
    think(engine);
    if(engine.is_searching) {
        assert(engine.best_move != NULL_MOVE);
        cout << "bestmove " << move_to_str(engine.best_move) << " ponder " << move_to_str(engine.ponder_move) << endl;
        cerr << "out: bestmove " << move_to_str(engine.best_move) << " ponder " << move_to_str(engine.ponder_move) << endl;
        engine.is_searching = false;
    }
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

    cout << "id name Dratini sin tt" << endl;
    cout << "id author Oscar Balcells" << endl;
    cout << "uciok" << endl;

    engine = Engine();
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
            cout << "readyok" << endl;
            cerr << "out: readyok" << endl;
        } else if(command == "setoption") {
            parse_option(args);
        } else if(command == "register") {
            cout << "you don't have to register" << endl;
            assert(false);
        } else if(command == "ucinewgame") {
            engine.set_position();
            tt.clear();
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
            // go_struct->engine = &engine;
            // go_struct->param_name = MOVETIME;
            // go_struct->param_value = 0;
            pthread_create(&pthread_go, NULL, &process_go, go_struct);
        } else if(command == "stop") {
            engine.stop_search = true;
        } else if(command == "ponderhit") {
            // engine.is_pondering = false;
        } else if(command == "print") {
            engine.board.print_board();
        } else if(command == "quit") {
            return;
        }
    }
}

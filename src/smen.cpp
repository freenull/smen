#include <smen/engine.hpp>

int main(int argc, char ** argv) {
    if (argc < 2) {
        std::cerr << "usage: smen <game_dir>\n";
        return 1;
    }

    auto game_dir = std::string(argv[1]);

    auto engine = smen::Engine("SMEN", game_dir);
    engine.execute();

    return 0;
}

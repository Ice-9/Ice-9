
# include       "tilengine.hpp"



int main(int argc, char *argv[]) {
    engine_init(argc > 1 && ! strcmp(argv[1], "-no-ok"));
    Cell return_code = engine();
    exit(return_code);
}


#include "bot.h"

int main(int argc, char* argv[]){
    if (argc < 2) return 0;
    std::string path = argv[1];
    Bot bot(path);
    return 0;
}
#include <string>
#include "manufactory.h"

int main(int argc, char* argv[]){
    if (argc < 2) return -1;
    std::string path = argv[1];
    //std::string path = "D:/institute/YADRO/task2/in.txt";
    Manufactory man(path);
    return 0;
    
}

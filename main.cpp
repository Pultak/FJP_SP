#include <iostream>
#include <string>
#include <fstream>

extern int parse_code(FILE* input, FILE* output);

int main(int argc, char* argv[]){
    int parseresult = 0;
    std::string out_name;
    if (argc >= 3) {
        std::cout << "Opening source file " << argv[2] << std::endl;
        FILE* in_file = fopen(argv[2], "r");
        out_name = argv[1];

        if (!in_file) {
            std::cerr << "Cannot open file: " << in_file << std::endl;
            return -1;
        }

        std::cout << "Parsing..." << std::endl;
        parseresult = parse_code(in_file, stdout);

    } else if(argc == 2) {
        parseresult = parse_code(stdin, stdout);
        out_name = argv[1];
    }
    else{
        std::cout << "Not enough program arguments" << std::endl;
    }
    std::cout << "Parsing finished" << std::endl;
    if (parseresult != 0) {
        std::cerr << "Parsing error" << std::endl;
        return -1;
    }
    return 0;
}

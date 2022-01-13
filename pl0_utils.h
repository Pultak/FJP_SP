#include <string>

namespace pl0_utils{
    // prikazy pl0
    enum class pl0code_fct : uint8_t {
        LIT,
        OPR,
        LOD,
        STO,
        CAL,
        INT,
        JMP,
        JPC,
        STA,
        LDA,
    };

    //pl0 INT operations
    enum class pl0code_opr : int {
        RETURN = 0,

        // arithmetic operations
        NEGATE = 1, // arithmetic
        ADD = 2,
        SUBTRACT = 3,
        MULTIPLY = 4,
        DIVIDE = 5,

        // compare operations
        ODD = 6,
        // 7 is undefined
        EQUAL = 8,
        NOTEQUAL = 9,
        LESS = 10,
        GREATER_OR_EQUAL = 11,
        GREATER = 12,
        LESS_OR_EQUAL = 13,
    };

    struct pl0code_arg {

        pl0code_arg(int val) : isref(false), value(val), symbolref("") {
        }

        pl0code_arg(pl0code_opr val) : isref(false), value(static_cast<int>(val)), symbolref("") {
        }

        pl0code_arg(const std::string &sym, bool function = false) : isref(true), isfunc(function), value(0),
                                                                   symbolref(sym) {
        }

        // is this argument a reference? (needs to be resolved?)
        bool isref;
        // is this argument a function reference?
        bool isfunc = false;
        // if resolved, this holds the actual value
        int value;
        // symbol reference (for what symbol to look when resolving)
        std::string symbolref;
        // offset to be used when resolving to an actual address
        int offset = 0;

        // shortcut for resolving address of a symbol
        void resolve(int addr_value) {
            value = addr_value + offset;
            isref = false;
            symbolref = "";
        }
    };



    struct pl0code_instruction {

        pl0code_instruction() : instruction(pl0code_fct::LIT), lvl(0), arg(0) {
        }

        pl0code_instruction(pl0code_fct fct, int level, pl0code_arg argument) : instruction(fct), lvl(level), arg(argument) {
        }

        pl0code_instruction(pl0code_fct fct, pl0code_arg argument) : instruction(fct), lvl(0), arg(argument) {
        }

        pl0code_fct instruction;
        int lvl;
        pl0code_arg arg;

        std::string to_string() const {
            std::string ret;

            switch (instruction) {
                case pl0code_fct::LIT:
                    ret = "LIT";
                    break;
                case pl0code_fct::OPR:
                    ret = "OPR";
                    break;
                case pl0code_fct::LOD:
                    ret = "LOD";
                    break;
                case pl0code_fct::STO:
                    ret = "STO";
                    break;
                case pl0code_fct::CAL:
                    ret = "CAL";
                    break;
                case pl0code_fct::INT:
                    ret = "INT";
                    break;
                case pl0code_fct::JMP:
                    ret = "JMP";
                    break;
                case pl0code_fct::JPC:
                    ret = "JPC";
                    break;
                case pl0code_fct::STA:
                    ret = "STA";
                    break;
                case pl0code_fct::LDA:
                    ret = "LDA";
                    break;
                default:
                    ret = "???";
                    break;
            }

            ret += " " + std::to_string(lvl) + " " +
                   (arg.isref ? "<" + arg.symbolref + ">" : std::to_string(arg.value));

            return ret;
        }
    };

}

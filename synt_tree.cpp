#include "synt_tree.h"


generation_result generate_result(evaluate_error result, std::string message){
    return generation_result{result, std::move(message)};
}
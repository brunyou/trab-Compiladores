#ifndef SEMANTICO_H
#define SEMANTICO_H

#include "Token.h"
#include "SemanticError.h"
#include "SymbolTable.h"
#include "Symbol.h"
#include <string>
#include <vector>
#include <optional>
#include <sstream>
#include <stack>
#include <functional>

// ... (structs e enums como antes) ...
struct PendingDeclaration {
    std::string name;
    int position;
    bool is_vector;
    int vector_size;
};

enum class ActiveIdContext {
    NONE,
    LHS_ASSIGNMENT,
    RHS_EXPRESSION,
    LEIA_TARGET
};


class Semantico
{
public:
    Semantico(SymbolTable& table);
    void executeAction(int action, const Token *token);
    std::string get_generated_code();

    // --- FUNÇÕES ADICIONADAS DE VOLTA ---
    std::string formatarTabelaSimbolos();
    std::vector<std::string> getCompilationWarnings();


private:
    SymbolTable& symbolTable;
    std::vector<std::string> data_section;
    std::vector<std::string> text_section;
    std::vector<std::string> compilationWarnings; 
    int label_counter;
    int temp_address_start;
    std::stack<std::string> labels_stack;
    std::stack<std::string> operand_stack;
    std::vector<PendingDeclaration> pending_declarations_list;
    std::string current_dec_id_name;
    int current_dec_id_pos;
    std::string assignment_target_id_name;
    bool assignment_target_is_vector_element;
    std::string currentLHS_idType;
    int currentLHS_idPosition;
    std::string currentRHS_expressionType;
    std::string pendingFunctionReturnType;
    std::string currentProcessingFunctionName;
    std::vector<ParameterInfo> currentFunctionParamsInfoList;
    std::string pendingParamType;
    int paramPositionCounter;
    std::string active_function_call_name;
    int arg_count;
    std::vector<std::string> arg_types;
    std::vector<std::string> arg_temps;
    std::string active_id_name;
    int active_id_pos;
    bool active_id_is_vector_access;
    ActiveIdContext active_id_context;
    std::string currentRelationalOperator;
    bool is_in_for_post_op_capture;
    std::vector<std::string> for_post_op_code_buffer;
    bool is_in_for_body_capture;
    std::vector<std::string> for_body_code_buffer;
    std::string main_code_label;
    bool main_label_placed;
    std::string new_label();
    std::string new_temp();
    void free_temp(const std::string& temp);
    void gera_cod(const std::string& instruction, const std::string& arg1 = "", const std::string& arg2 = "");
    void gera_data(const std::string& label, const std::string& value, int count = 1);
};

#endif
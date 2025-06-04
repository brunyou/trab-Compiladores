#ifndef SEMANTICO_H
#define SEMANTICO_H

#include "Token.h"
#include "SemanticError.h"
#include "SymbolTable.h"
#include "Symbol.h"      // Para ParameterInfo e Symbol
#include <string>
#include <vector>
#include <optional>
#include <sstream>
#include <stack>

class Semantico
{
public:
    Semantico(SymbolTable& table) :
        symbolTable(table),
        currentLHS_idPosition(-1),
        pendingParamIsArray(false),
        paramPositionCounter(-1),
        label_counter(0),
        temp_address_start(1000),
        active_id_pos(0),
        active_id_is_vector_access(false),
        active_id_context(ActiveIdContext::NONE),
        current_dec_id_pos(0),
        current_dec_is_vector(false),
        current_dec_vector_size(1)
        {}

    void executeAction(int action, const Token *token);
    std::string formatarTabelaSimbolos();
    std::vector<std::string> getCompilationWarnings() const { return compilationWarnings; }
    std::string get_generated_code();

private:
    SymbolTable& symbolTable;

    enum class ActiveIdContext { NONE, LHS_ASSIGNMENT, RHS_EXPRESSION, LEIA_TARGET };

    std::string currentLHS_idName;
    std::string currentLHS_idType;
    int currentLHS_idPosition;
    std::string currentRHS_expressionType;

    std::string pendingFunctionReturnType;
    std::string currentProcessingFunctionName;
    std::string currentFunctionDeclarationScope;
    std::vector<ParameterInfo> currentFunctionParamsInfoList;
    std::string pendingParamType;
    bool pendingParamIsArray;
    int paramPositionCounter;
    std::vector<std::string> compilationWarnings;

    std::string active_id_name;
    int active_id_pos;
    bool active_id_is_vector_access;
    ActiveIdContext active_id_context;

    struct PendingDeclarationItem {
        std::string name;
        int position;
        bool is_vector;
        int vector_size;
    };
    std::vector<PendingDeclarationItem> pending_declarations_list;

    std::string current_dec_id_name;
    int current_dec_id_pos;
    bool current_dec_is_vector;
    int current_dec_vector_size;

    std::vector<std::string> data_section;
    std::vector<std::string> text_section;
    int label_counter;
    int temp_address_start;

    std::string new_label();
    std::string new_temp();
    void free_temp(const std::string& temp);
    void gera_cod(const std::string& instruction,
                  const std::string& arg1 = "",
                  const std::string& arg2 = "");
    // Assinatura correta para aceitar 'count'
    void gera_data(const std::string& label, const std::string& value, int count = 1);
};

#endif // SEMANTICO_H
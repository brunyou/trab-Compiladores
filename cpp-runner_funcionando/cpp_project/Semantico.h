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
        assignment_target_is_vector_element(false), // Inicialização do novo membro
        current_dec_id_pos(0),
        current_dec_is_vector(false),
        current_dec_vector_size(1)
        // current_expression_accumulator_temp e last_operator_token são std::string,
        // inicializadas vazias por padrão, o que é ok.
        {}

    void executeAction(int action, const Token *token);
    std::string formatarTabelaSimbolos();
    std::vector<std::string> getCompilationWarnings() const { return compilationWarnings; }
    std::string get_generated_code();

private:
    SymbolTable& symbolTable;

    enum class ActiveIdContext { NONE, LHS_ASSIGNMENT, RHS_EXPRESSION, LEIA_TARGET };

    // --- Membros para Semântica de Atribuição e Tipos ---
    std::string currentLHS_idType;     // Tipo do ID no LHS (setado por #3, usado por #5)
    int currentLHS_idPosition;         // Posição do ID no LHS (setado por #3, usado por #5)
    std::string currentRHS_expressionType; // Tipo inferido para a expressão no RHS (setado por #4, usado por #5)

    // --- Novos Membros para o Alvo da Atribuição ---
    std::string assignment_target_id_name;         // Guarda o nome do ID que é o alvo da atribuição
    bool assignment_target_is_vector_element; // True se o alvo da atribuição for um elemento de vetor

    // --- Membros para Declaração de Funções e Parâmetros ---
    std::string pendingFunctionReturnType;
    std::string currentProcessingFunctionName;
    std::string currentFunctionDeclarationScope;
    std::vector<ParameterInfo> currentFunctionParamsInfoList;
    std::string pendingParamType;
    bool pendingParamIsArray;
    int paramPositionCounter;
    std::vector<std::string> compilationWarnings;

    // --- Membros para Geração de Código e Estado Ativo de ID/Vetor (ACESSO em expressões/leia) ---
    std::string active_id_name;        // Nome do ID sendo processado para acesso ou como OPERANDO
    int active_id_pos;                 // Posição do ID ativo para acesso/operando
    bool active_id_is_vector_access; // Flag: o ID ativo (acesso/operando) foi acessado como vetor?
    ActiveIdContext active_id_context; // Contexto do ID ativo

    // --- Membros para Processamento de Declarações de Variáveis/Vetores ---
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

    // --- Membros Geração de Código ---
    std::vector<std::string> data_section;
    std::vector<std::string> text_section;
    int label_counter;
    int temp_address_start;

    // --- Membros para Acumulador de Expressão Aritmética ---
    std::string current_expression_accumulator_temp;
    std::string last_operator_token;

    // --- Funções Auxiliares ---
    std::string new_label();
    std::string new_temp();
    void free_temp(const std::string& temp);
    void gera_cod(const std::string& instruction,
                  const std::string& arg1 = "",
                  const std::string& arg2 = "");
    void gera_data(const std::string& label, const std::string& value, int count = 1);

        // --- NOVOS MEMBROS PARA CONTROLE CONDICIONAL ---
    std::string currentRelationalOperator;
    std::stack<std::string> labels_stack;
};

#endif // SEMANTICO_H
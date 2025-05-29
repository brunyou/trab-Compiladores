#ifndef SEMANTICO_H
#define SEMANTICO_H

#include "Token.h"
#include "SemanticError.h"
#include "SymbolTable.h"
#include "Symbol.h"
#include <string>
#include <vector>
#include <optional>
#include <sstream> // Necessário para geração de código
#include <stack>   // Necessário para geração de código (futuro)

class Semantico
{
public:
    // Construtor inicializa os novos membros
    Semantico(SymbolTable& table) :
        symbolTable(table),
        currentLHS_idPosition(-1),
        pendingParamIsArray(false),
        paramPositionCounter(-1),
        label_counter(0),        // Inicializa contador de rótulos
        temp_address_start(1000) // Endereço inicial para temporários
        {}

    // Ação principal
    void executeAction(int action, const Token *token);

    // Função original de formatação da tabela (mantida)
    std::string formatarTabelaSimbolos();

    // Funções existentes
    std::vector<std::string> getCompilationWarnings() const { return compilationWarnings; }

    // Nova função para obter o código gerado
    std::string get_generated_code();

private:
    SymbolTable& symbolTable;

    // --- Membros Existentes (Semântica) ---
    std::vector<std::string> currentIdList;
    std::vector<int> currentIdPositions;
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

    // --- Novos Membros para Geração de Código ---
    std::vector<std::string> data_section;
    std::vector<std::string> text_section;
    int label_counter;
    int temp_address_start;
    // std::stack<std::string> code_gen_stack; // Exemplo para futuro

    // --- Novas Funções Auxiliares (Geração de Código) ---
    std::string new_label();
    std::string new_temp();
    void free_temp(const std::string& temp);
    void gera_cod(const std::string& instruction,
                  const std::string& arg1 = "",
                  const std::string& arg2 = "");
    void gera_data(const std::string& label, const std::string& value);
};

#endif // SEMANTICO_H
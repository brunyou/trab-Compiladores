#ifndef SEMANTICO_H
#define SEMANTICO_H

#include "Token.h"
#include "SemanticError.h" 
#include "SymbolTable.h"
#include "Symbol.h"      // Para ParameterInfo
#include <string>
#include <vector>
#include <optional>

class Semantico
{
public:
    Semantico(SymbolTable& table) : 
        symbolTable(table), 
        currentLHS_idPosition(-1),
        pendingParamIsArray(false), // Inicializa flag de parâmetro array
        paramPositionCounter(-1)    // Inicializa contador de posição de parâmetro para uma função
        {}

    void executeAction(int action, const Token *token);
    std::string formatarTabelaSimbolos();
    std::vector<std::string> getCompilationWarnings() const { return compilationWarnings; }

private:
    SymbolTable& symbolTable; 

    // Para declarações de variáveis simples
    std::vector<std::string> currentIdList;
    std::vector<int> currentIdPositions;

    // Para comandos de atribuição
    std::string currentLHS_idName;
    std::string currentLHS_idType;
    int currentLHS_idPosition;
    std::string currentRHS_expressionType; 

    // Para declaração de funções e parâmetros
    std::string pendingFunctionReturnType;          // Armazena o tipo de retorno da função sendo declarada
    std::string currentProcessingFunctionName;      // Nome da função/procedimento atual
    std::string currentFunctionDeclarationScope;    // Escopo ONDE a função foi declarada
    std::vector<ParameterInfo> currentFunctionParamsInfoList; // Coleta ParameterInfo para a função atual
    
    std::string pendingParamType;   // Tipo do parâmetro individual sendo processado
    bool pendingParamIsArray;       // Se o parâmetro individual atual é um array
    int paramPositionCounter;       // Para a 'posicaoParametro' no símbolo do parâmetro (0, 1, 2...)

    std::vector<std::string> compilationWarnings; 
};

#endif
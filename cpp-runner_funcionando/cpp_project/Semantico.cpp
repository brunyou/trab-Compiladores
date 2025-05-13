#include "Semantico.h"
#include "Constants.h" // Seu Constants.h gerado pelo GALS (contém enum TokenId)
#include "Symbol.h"
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <optional>
#include <functional>

void Semantico::executeAction(int action, const Token *token) {
    Symbol symbol_being_processed; // Usado para criar novos símbolos

    std::cerr << "DEBUG - Semantico::executeAction: action=" << action;
    if (token) {
        std::cerr << ", token_lexeme='" << token->getLexeme() << "'"
                  << ", token_id=" << token->getId() 
                  << ", token_pos=" << token->getPosition();
    } else {
        std::cerr << ", token=NULL";
    }
    std::cerr << std::endl;

    // Ajuste a condição de verificação de token nulo conforme necessário para cada ação
    if (!token && (action == 1 || action == 2 || action == 3 || action == 4 || action == 6 || 
                   action == 8 || action == 11 || action == 12 || action == 13 || action == 14)) { 
         throw SemanticError("Token nulo inesperado para Ação Semântica #" + std::to_string(action), (token ? token->getPosition() : -1) );
    }

    // ####################################################################################
    // SUBSTITUA OS NÚMEROS DOS CASE 8 A 15 ABAIXO PELOS NÚMEROS REAIS QUE O GALS ATRIBUIU
    // ÀS SUAS AÇÕES SEMÂNTICAS CORRESPONDENTES NA GRAMÁTICA, CONFORME SEU NOVO Constants.h
    // ####################################################################################
    switch (action) {
        // --- AÇÕES EXISTENTES #1 a #7 (Lógica como na última versão funcional) ---
        case 1: { // DECLARAÇÃO DE VAR: Após <type>
            std::string recognized_type = token->getLexeme();
            std::string escopoAtual = symbolTable.getEscopoAtual(); 
            if (recognized_type != "int" && recognized_type != "float" && recognized_type != "string" &&
                recognized_type != "bool" && recognized_type != "char" && recognized_type != "double" &&
                recognized_type != "void") {
                if (recognized_type == "void") throw SemanticError("Variáveis não podem ser do tipo 'void'.", token->getPosition());
                throw SemanticError("Tipo inválido na declaração: '" + recognized_type + "'", token->getPosition());
            }
            if (!currentIdList.empty()) {
                for (size_t i = 0; i < currentIdList.size(); ++i) {
                    const std::string& varName = currentIdList[i];
                    int varPos = currentIdPositions[i];
                    if (symbolTable.existeNoEscopoAtual(varName)) { 
                        throw SemanticError("Variável '" + varName + "' já declarada no escopo '" + escopoAtual + "'.", varPos);
                    }
                    symbol_being_processed = Symbol(varName, recognized_type, escopoAtual, varPos);
                    symbolTable.inserir(symbol_being_processed);
                }
            }
            currentIdList.clear();
            currentIdPositions.clear();
            break;
        }
        case 2: { // DECLARAÇÃO DE VAR: Após ID
            std::string varName = token->getLexeme();
            currentIdList.push_back(varName);
            currentIdPositions.push_back(token->getPosition());
            break;
        }
        case 3: { // ATRIBUIÇÃO: Após ID no LHS
            this->currentLHS_idName = token->getLexeme();
            this->currentLHS_idPosition = token->getPosition();
            std::optional<Symbol> symbol_opt = symbolTable.buscar(this->currentLHS_idName); 
            if (!symbol_opt) {
                throw SemanticError("Variável '" + this->currentLHS_idName + "' não declarada.", this->currentLHS_idPosition);
            }
             if (symbol_opt->funcao) {
                throw SemanticError("Identificador '" + this->currentLHS_idName + "' é uma função e não pode receber atribuição.", this->currentLHS_idPosition);
            }
            this->currentLHS_idType = symbol_opt->tipo;
            symbolTable.marcarUsado(this->currentLHS_idName); 
            break;
        }
        case 4: { // ATRIBUIÇÃO: Após <expression> no RHS
            // Lógica simplificada (substitua t_TOKEN_ID pelos nomes do seu Constants.h)
            if (token->getId() == t_INTEGER) { this->currentRHS_expressionType = "int"; }
            else if (token->getId() == t_FLOAT) { this->currentRHS_expressionType = "float"; }
            else if (token->getId() == t_STRING) { this->currentRHS_expressionType = "string"; }
            else if (token->getId() == t_ID) { 
                std::string rhs_id_name = token->getLexeme();
                std::optional<Symbol> rhs_sym_opt = symbolTable.buscar(rhs_id_name); 
                if (!rhs_sym_opt) {
                    throw SemanticError("Variável '" + rhs_id_name + "' não declarada (RHS).", token->getPosition());
                }
                if (rhs_sym_opt->funcao && rhs_sym_opt->tipo == "void") {
                     throw SemanticError("Procedimento '" + rhs_id_name + "' do tipo 'void' não pode ser usado em expressão.", token->getPosition());   
                }
                if (!rhs_sym_opt->inicializado && !rhs_sym_opt->parametro && !rhs_sym_opt->funcao) { 
                    std::string warning_msg = "AVISO SEMÂNTICO: Variável '" + rhs_id_name + "' (tipo " + rhs_sym_opt->tipo + ") usada no RHS sem ter sido inicializada (Pos: " + std::to_string(token->getPosition()) + ").";
                    std::cerr << warning_msg << std::endl;
                    compilationWarnings.push_back(warning_msg);
                }
                symbolTable.marcarUsado(rhs_id_name);
                this->currentRHS_expressionType = rhs_sym_opt->tipo; 
            } else { this->currentRHS_expressionType = "tipo_desconhecido_expr"; }
            break;
        }
        case 5: { // ATRIBUIÇÃO: Final da atribuição
            if (this->currentLHS_idName.empty()) { break; }
            if (this->currentRHS_expressionType.empty() || this->currentRHS_expressionType == "tipo_desconhecido_expr") {
                 throw SemanticError("Não foi possível determinar o tipo da expressão RHS para '" + this->currentLHS_idName + "'.", this->currentLHS_idPosition);
            }
            bool compatible = false;
            if (this->currentLHS_idType == this->currentRHS_expressionType) compatible = true;
            else if (this->currentLHS_idType == "float" && this->currentRHS_expressionType == "int") compatible = true;
            else if (this->currentLHS_idType == "double" && (this->currentRHS_expressionType == "int" || this->currentRHS_expressionType == "float")) compatible = true;
            if (!compatible) {
                throw SemanticError("Tipos incompatíveis na atribuição para '" + this->currentLHS_idName +
                                    "': " + this->currentLHS_idType + " = " + this->currentRHS_expressionType,
                                    this->currentLHS_idPosition);
            }
            symbolTable.marcarInicializado(this->currentLHS_idName);
            this->currentLHS_idName.clear(); this->currentLHS_idType.clear(); 
            this->currentLHS_idPosition = -1; this->currentRHS_expressionType.clear();
            break;
        }
        case 6: { // ESCOPO: Início de Bloco '{'
            symbolTable.entrarEscopoBloco(); 
            break;
        }
        case 7: { // ESCOPO: Fim de Bloco '}'
            std::string escopoQueTermina = symbolTable.getEscopoAtual();
            std::vector<Symbol> simbolosDoEscopo = symbolTable.getSimbolosDoEscopo(escopoQueTermina);
            for (const auto& sym : simbolosDoEscopo) {
                if (!sym.usado && !sym.funcao && !sym.parametro && !sym.vetor) { 
                    std::string warning_msg = "AVISO SEMÂNTICO: Identificador '" + sym.id + "' (tipo " + sym.tipo + 
                                              ") declarado no escopo '" + escopoQueTermina + 
                                              "' mas não foi usado (declarado na linha aprox. " + 
                                              std::to_string(sym.linhaDeclaracao) + ").";
                    std::cerr << warning_msg << std::endl;
                    compilationWarnings.push_back(warning_msg);
                }
            }
            symbolTable.sairEscopo(); 
            break;
        }

        // --- NOVAS AÇÕES PARA FUNÇÕES E PARÂMETROS ---
        // SUBSTITUA OS NÚMEROS 8, 9, 10, 11, 12, 13, 14, 15 PELOS REAIS DO SEU Constants.h
        case 8: { // #8: Após ID da função em <subroutine_declaration>
            this->currentProcessingFunctionName = token->getLexeme();
            int funcPos = token->getPosition();
            // O tipo de retorno já deve estar em pendingFunctionReturnType (definido pela Ação #11)
            if (this->pendingFunctionReturnType.empty()) {
                throw SemanticError("Erro interno: tipo de retorno da função não definido antes do nome da função.", funcPos);
            }

            this->currentFunctionDeclarationScope = symbolTable.getEscopoAtual(); // Escopo onde a função é declarada

            std::cerr << "DEBUG - Action #8 (FUNC_DEC - ID): Registrando Função '" << this->currentProcessingFunctionName 
                      << "' com tipo_retorno='" << this->pendingFunctionReturnType 
                      << "' no escopo '" << this->currentFunctionDeclarationScope << "'" << std::endl;

            if (symbolTable.existeNoEscopoAtual(this->currentProcessingFunctionName)) {
                throw SemanticError("Função ou variável '" + this->currentProcessingFunctionName + "' já declarada no escopo '" + this->currentFunctionDeclarationScope + "'.", funcPos);
            }

            symbol_being_processed = Symbol(this->currentProcessingFunctionName, this->pendingFunctionReturnType, this->currentFunctionDeclarationScope, funcPos);
            symbol_being_processed.funcao = true;
            // A assinatura de parâmetros (assinaturaParametros) será preenchida pela Ação #9
            symbolTable.inserir(symbol_being_processed);
            
            symbolTable.entrarEscopoFuncao(this->currentProcessingFunctionName); // Entra no escopo INTERNO da função
            currentFunctionParamsInfoList.clear(); // Prepara para coletar infos dos parâmetros
            paramPositionCounter = 0; // Reseta contador de posição de parâmetro para esta função
            break;
        }
        case 9: { // #9: Após ')' da lista de parâmetros em <subroutine_declaration>
            std::cerr << "DEBUG - Action #9 (FUNC_DEC - END PARAMS): Finalizando assinatura para func '" << this->currentProcessingFunctionName 
                      << "' (declarada em '" << this->currentFunctionDeclarationScope << "') com " 
                      << this->currentFunctionParamsInfoList.size() << " params." << std::endl;

            // Atualiza o símbolo da função (que está no escopo pai) com a lista de parâmetros
            Symbol* funcSym = symbolTable.buscarParaModificacaoNoEscopo(this->currentProcessingFunctionName, this->currentFunctionDeclarationScope);
            if (funcSym && funcSym->funcao) {
                funcSym->assinaturaParametros = this->currentFunctionParamsInfoList;
            } else {
                throw SemanticError("Erro interno: Função '" + this->currentProcessingFunctionName + "' não encontrada no escopo '" + this->currentFunctionDeclarationScope + "' para atualizar assinatura.", -1); // Posição do ')' seria ideal se token não for nulo
            }
            currentFunctionParamsInfoList.clear(); 
            break;
        }
        case 10: { // #10: Após <function_body_block> em <subroutine_declaration>
            std::string escopoDaFuncaoQueTermina = symbolTable.getEscopoAtual(); 
            std::cerr << "DEBUG - Action #10 (FUNC_DEC - END BODY): Verificando não usados e saindo do escopo da função '" << escopoDaFuncaoQueTermina << "'" << std::endl;

            std::vector<Symbol> simbolosDoEscopoFunc = symbolTable.getSimbolosDoEscopo(escopoDaFuncaoQueTermina);
            for (const auto& sym : simbolosDoEscopoFunc) {
                if (!sym.usado) { 
                    std::string modalidade = sym.parametro ? "Parâmetro" : "Variável local";
                    std::string warning_msg = "AVISO SEMÂNTICO: " + modalidade + " '" + sym.id + "' (tipo " + sym.tipo + 
                                              ") declarado na função '" + this->currentProcessingFunctionName + // Usa o nome da função que está sendo processada
                                              "' mas não foi usado (declarado na linha aprox. " + 
                                              std::to_string(sym.linhaDeclaracao) + ").";
                    std::cerr << warning_msg << std::endl;
                    compilationWarnings.push_back(warning_msg);
                }
            }
            symbolTable.sairEscopo(); 
            this->currentProcessingFunctionName.clear();
            this->pendingFunctionReturnType.clear();
            this->currentFunctionDeclarationScope.clear();
            break;
        }
        case 11: { // #11: Em <return_type_spec> (após <type> ou VOID_KEYWORD)
            this->pendingFunctionReturnType = token->getLexeme();
            std::cerr << "DEBUG - Action #11 (FUNC_DEC - RETURN TYPE): Tipo de retorno pendente: '" << this->pendingFunctionReturnType << "'" << std::endl;
            break;
        }
        case 13: { // #13: Em <parameter>, após <type> do parâmetro
            this->pendingParamType = token->getLexeme();
            this->pendingParamIsArray = false; 
            std::cerr << "DEBUG - Action #13 (PARAM_DEC - TYPE): Tipo de parâmetro pendente: '" << this->pendingParamType << "'" << std::endl;
            break;
        }
        case 12: { // #12: Em <parameter>, após ID do parâmetro
            std::string paramName = token->getLexeme();
            int paramTokenPos = token->getPosition();
            std::string escopoAtualFunc = symbolTable.getEscopoAtual(); 

            std::cerr << "DEBUG - Action #12 (PARAM_DEC - ID): ParamID='" << paramName 
                      << "', TipoPendente='" << this->pendingParamType 
                      << "', ArrayPendente=" << this->pendingParamIsArray // Será definido por #14/#15
                      << ", EscopoFunc='" << escopoAtualFunc << "'" << std::endl;

            if (this->pendingParamType.empty()) {
                 throw SemanticError("Erro interno: tipo do parâmetro '" + paramName + "' não definido.", paramTokenPos);
            }
            if (this->pendingParamType == "void") {
                throw SemanticError("Parâmetros não podem ser do tipo 'void'. Parâmetro: '" + paramName + "'.", paramTokenPos);
            }

            if (symbolTable.existeNoEscopoAtual(paramName)) {
                throw SemanticError("Parâmetro '" + paramName + "' já declarado nesta função.", paramTokenPos);
            }

            symbol_being_processed = Symbol(paramName, this->pendingParamType, escopoAtualFunc, paramTokenPos);
            symbol_being_processed.parametro = true;
            symbol_being_processed.inicializado = true; 
            symbol_being_processed.posicaoParametro = this->paramPositionCounter++; 
            // A flag 'vetor' será setada por Ação #14 se aplicável
            
            symbolTable.inserir(symbol_being_processed);

            ParameterInfo paramInfo;
            paramInfo.nome = paramName;
            paramInfo.tipo = this->pendingParamType;
            paramInfo.isArray = false; // Default, será atualizado por #14 se necessário
            paramInfo.linhaDeclaracaoParam = paramTokenPos;
            currentFunctionParamsInfoList.push_back(paramInfo); 
            
            // Resetar pendingParamIsArray para o próximo parâmetro ou para o estado padrão após este
            this->pendingParamIsArray = false;
            break;
        }
        case 14: { // #14: Em <optional_array_spec_param_suffix> para '[]'
            std::cerr << "DEBUG - Action #14 (PARAM_DEC - IS_ARRAY)" << std::endl;
            // Marca que o último parâmetro adicionado à lista é um array
            if (!currentFunctionParamsInfoList.empty()) {
                currentFunctionParamsInfoList.back().isArray = true;
                
                // Atualiza também o símbolo na tabela de símbolos principal
                Symbol* lastParamSymbol = symbolTable.buscarParaModificacaoNoEscopo(
                                                currentFunctionParamsInfoList.back().nome,
                                                symbolTable.getEscopoAtual());
                if (lastParamSymbol) {
                    lastParamSymbol->vetor = true;
                    // TODO: Tratar múltiplas dimensões se <optional_array_spec_param_suffix> for recursiva
                    //       e se Symbol precisar armazenar contagem de dimensões.
                    //       Por enquanto, apenas um bool 'vetor'.
                }
            } else {
                std::cerr << "WARNING - Action #14: Chamada sem um parâmetro pendente na lista." << std::endl;
            }
            break;
        }
        case 15: { // #15: Em <optional_array_spec_param_suffix> para 'î' (parametro escalar)
            std::cerr << "DEBUG - Action #15 (PARAM_DEC - IS_SCALAR)" << std::endl;
            // Não precisa fazer nada se o default para ParameterInfo.isArray é false
            // e para Symbol.vetor é false.
            break;
        }
        
        // TODO: Implementar ações para CHAMADA DE FUNÇÃO (#17, #18, #19, #20, #21 da sua BNF)
        // TODO: Implementar ações para EXPRESSÕES (#16 para ID em expressão, #30-#32 para literais, e para operadores)
        // TODO: Implementar ação para FIM DE PROGRAMA (#22 da sua BNF)

        default:
            std::cerr << "WARNING - Ação semântica #" << action 
                      << " (Número real do GALS para cmd[1]) desconhecida ou não implementada." << std::endl;
            break;
    }
}

std::string Semantico::formatarTabelaSimbolos() {
    std::string output = "";
    std::vector<Symbol> tabela = symbolTable.getTabela();
    if (tabela.empty()) {
        return ""; 
    }
    output += "ID,Tipo,Inicializado,Usado,Escopo,Parametro,PosParametro,Vetor,Funcao,LinhaDec\n"; // Removido Matriz, Referencia por enquanto
    for (const auto& s : tabela) {
        output += s.id + "," +
                  s.tipo + "," +
                  (s.inicializado ? "T" : "F") + "," +
                  (s.usado ? "T" : "F") + "," +
                  s.escopo + "," +
                  (s.parametro ? "T" : "F") + "," +
                  std::to_string(s.posicaoParametro) + "," +
                  (s.vetor ? "T" : "F") + "," +
                //   (s.matriz ? "T" : "F") + "," +    // Removido temporariamente
                //   (s.referencia ? "T" : "F") + "," + // Removido temporariamente
                  (s.funcao ? "T" : "F") + "," +
                  std::to_string(s.linhaDeclaracao) + "\n";
    }
    return output;
}
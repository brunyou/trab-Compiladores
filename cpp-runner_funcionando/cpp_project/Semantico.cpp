#include "Semantico.h"
#include "Constants.h" // Seu Constants.h gerado pelo GALS
#include "Symbol.h"
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <optional>
#include <functional>
#include <sstream>
#include <stack>
#include <algorithm>

// --- Implementação das Funções Auxiliares de Geração de Código ---

std::string Semantico::new_label() {
    std::stringstream ss;
    ss << "L" << label_counter++;
    return ss.str();
}

std::string Semantico::new_temp() {
    std::stringstream ss;
    ss << temp_address_start++;
    std::string temp_name = ss.str();
    gera_data(temp_name, "0");  // Declara o temp na seção .data
    return temp_name;
}

void Semantico::free_temp(const std::string& temp) {
    // Nenhuma ação nesta implementação simples.
}

void Semantico::gera_cod(const std::string& instruction, const std::string& arg1, const std::string& arg2) {
    std::string line = "    " + instruction;
    if (!arg1.empty()) line += "\t" + arg1;
    if (!arg2.empty()) line += "\t" + arg2;
    text_section.push_back(line);
    std::cerr << "DEBUG - GERA_COD: " << line << std::endl;
}

void Semantico::gera_data(const std::string& label, const std::string& value) {
    std::string data_line = label + ":\t" + value;
    bool found = false;
    for(const auto& line : data_section) {
        if(line.find(label + ":") == 0) {
            found = true;
            break;
        }
    }
    if(!found) {
       data_section.push_back(data_line);
       std::cerr << "DEBUG - GERA_DATA: " << data_line << std::endl;
    }
}

std::string Semantico::get_generated_code() {
    std::stringstream ss;
    ss << ".data\n";
    for (const auto& line : data_section) {
        ss << "    " << line << "\n";
    }
    ss << "\n.text\n_PRINCIPAL:\n"; // TODO: Ajustar ponto de entrada
    for (const auto& line : text_section) {
        ss << line << "\n";
    }
    return ss.str();
}

// --- Implementação de executeAction (Completa e com Geração de Código) ---

void Semantico::executeAction(int action, const Token *token) {
    Symbol symbol_being_processed;

    std::cerr << "DEBUG - Semantico::executeAction: action=" << action;
    if (token) {
        std::cerr << ", token_lexeme='" << token->getLexeme() << "'"
                  << ", token_id=" << token->getId()
                  << ", token_pos=" << token->getPosition();
    } else {
        std::cerr << ", token=NULL";
    }
    std::cerr << std::endl;

    // Adicione os novos números de ação aqui se eles precisarem de token.
    if (!token && (action == 1 || action == 2 || action == 3 || action == 4 || action == 6 ||
                   action == 8 || action == 11 || action == 12 || action == 13 || action == 14 ||
                   action == 16 || action == 18 || action == 19 )) {
         throw SemanticError("Token nulo inesperado para Ação Semântica #" + std::to_string(action), (token ? token->getPosition() : -1) );
    }

    switch (action) {
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
                    // GERAÇÃO DE CÓDIGO: Adiciona variável à seção .data
                    gera_data(varName, "0");
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
        case 4: { // ATRIBUIÇÃO: Após <expression> no RHS (AGORA SÓ SEMÂNTICA)
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
            // GERAÇÃO DE CÓDIGO: Gera o STO final
            gera_cod("STO", this->currentLHS_idName);
            // Limpeza
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
        case 8: { // #8: Após ID da função em <subroutine_declaration>
            this->currentProcessingFunctionName = token->getLexeme();
            int funcPos = token->getPosition();
            if (this->pendingFunctionReturnType.empty()) {
                throw SemanticError("Erro interno: tipo de retorno da função não definido antes do nome da função.", funcPos);
            }
            this->currentFunctionDeclarationScope = symbolTable.getEscopoAtual();
            std::cerr << "DEBUG - Action #8 (FUNC_DEC - ID): Registrando Função '" << this->currentProcessingFunctionName
                      << "' com tipo_retorno='" << this->pendingFunctionReturnType
                      << "' no escopo '" << this->currentFunctionDeclarationScope << "'" << std::endl;
            if (symbolTable.existeNoEscopoAtual(this->currentProcessingFunctionName)) {
                throw SemanticError("Função ou variável '" + this->currentProcessingFunctionName + "' já declarada no escopo '" + this->currentFunctionDeclarationScope + "'.", funcPos);
            }
            symbol_being_processed = Symbol(this->currentProcessingFunctionName, this->pendingFunctionReturnType, this->currentFunctionDeclarationScope, funcPos);
            symbol_being_processed.funcao = true;
            symbolTable.inserir(symbol_being_processed);
            symbolTable.entrarEscopoFuncao(this->currentProcessingFunctionName);
            currentFunctionParamsInfoList.clear();
            paramPositionCounter = 0;
            gera_cod(this->currentProcessingFunctionName + ":");
            break;
        }
        case 9: { // #9: Após ')' da lista de parâmetros em <subroutine_declaration>
            std::cerr << "DEBUG - Action #9 (FUNC_DEC - END PARAMS): Finalizando assinatura para func '" << this->currentProcessingFunctionName
                      << "' (declarada em '" << this->currentFunctionDeclarationScope << "') com "
                      << this->currentFunctionParamsInfoList.size() << " params." << std::endl;
            Symbol* funcSym = symbolTable.buscarParaModificacaoNoEscopo(this->currentProcessingFunctionName, this->currentFunctionDeclarationScope);
            if (funcSym && funcSym->funcao) {
                funcSym->assinaturaParametros = this->currentFunctionParamsInfoList;
            } else {
                throw SemanticError("Erro interno: Função '" + this->currentProcessingFunctionName + "' não encontrada no escopo '" + this->currentFunctionDeclarationScope + "' para atualizar assinatura.", -1);
            }
            currentFunctionParamsInfoList.clear();
            break;
        }
        case 10: { // #10: Após <function_body_block> em <subroutine_declaration>
            std::string escopoDaFuncaoQueTermina = symbolTable.getEscopoAtual();
            std::cerr << "DEBUG - Action #10 (FUNC_DEC - END BODY): Verificando não usados e saindo do escopo da função '" << escopoDaFuncaoQueTermina << "'" << std::endl;
            std::vector<Symbol> simbolosDoEscopoFunc = symbolTable.getSimbolosDoEscopo(escopoDaFuncaoQueTermina);
            for (const auto& sym : simbolosDoEscopoFunc) {
                if (!sym.usado && !sym.funcao) {
                    std::string modalidade = sym.parametro ? "Parâmetro" : "Variável local";
                    std::string warning_msg = "AVISO SEMÂNTICO: " + modalidade + " '" + sym.id + "' (tipo " + sym.tipo +
                                              ") declarado na função '" + this->currentProcessingFunctionName +
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
            gera_cod("RETURN");
            break;
        }
        case 11: { // #11: Em <return_type_spec>
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
                      << "', EscopoFunc='" << escopoAtualFunc << "'" << std::endl;
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
            symbolTable.inserir(symbol_being_processed);
            ParameterInfo paramInfo;
            paramInfo.nome = paramName;
            paramInfo.tipo = this->pendingParamType;
            paramInfo.isArray = false;
            paramInfo.linhaDeclaracaoParam = paramTokenPos;
            currentFunctionParamsInfoList.push_back(paramInfo);
            this->pendingParamIsArray = false;
            gera_data(paramName, "0"); // Parâmetros também precisam de espaço
            break;
        }
        case 14: { // #14: Em <optional_array_spec_param_suffix> para '[]'
            std::cerr << "DEBUG - Action #14 (PARAM_DEC - IS_ARRAY)" << std::endl;
            if (!currentFunctionParamsInfoList.empty()) {
                currentFunctionParamsInfoList.back().isArray = true;
                Symbol* lastParamSymbol = symbolTable.buscarParaModificacaoNoEscopo(
                                                currentFunctionParamsInfoList.back().nome,
                                                symbolTable.getEscopoAtual());
                if (lastParamSymbol) {
                    lastParamSymbol->vetor = true;
                }
            } else {
                std::cerr << "WARNING - Action #14: Chamada sem um parâmetro pendente na lista." << std::endl;
            }
            break;
        }
        case 15: { // #15: Em <optional_array_spec_param_suffix> para 'î'
            std::cerr << "DEBUG - Action #15 (PARAM_DEC - IS_SCALAR)" << std::endl;
            break;
        }
        // --- NOVAS AÇÕES PARA GERAÇÃO DE CÓDIGO ---
        case 16: { // #16: Após ID em <input> (LEIA)
            // Lógica semântica: Verificar se ID existe e pode receber E/S
            std::optional<Symbol> sym_opt = symbolTable.buscar(token->getLexeme());
            if (!sym_opt) throw SemanticError("Variável '" + token->getLexeme() + "' não declarada para LEIA.", token->getPosition());
            if (sym_opt->funcao) throw SemanticError("Não é possível ler em uma função ('" + token->getLexeme() + "').", token->getPosition());
            // GERAÇÃO DE CÓDIGO
            gera_cod("LD", "$in_port");
            gera_cod("STO", token->getLexeme());
            symbolTable.marcarInicializado(token->getLexeme());
            symbolTable.marcarUsado(token->getLexeme());
            break;
        }
        case 17: { // #17: Após <expression> em <output> (ESCREVA)
            // GERAÇÃO DE CÓDIGO
            gera_cod("STO", "$out_port");
            break;
        }
        case 18: { // #18: Após ID em <primary> (Expressão)
            std::string id = token->getLexeme();
            std::optional<Symbol> sym_opt = symbolTable.buscar(id);
            if (!sym_opt) throw SemanticError("Variável '" + id + "' não declarada.", token->getPosition());
            if (!sym_opt->inicializado && !sym_opt->parametro && !sym_opt->funcao) {
                 std::string warning_msg = "AVISO SEMÂNTICO: Variável '" + id + "' usada sem ter sido inicializada (Pos: " + std::to_string(token->getPosition()) + ").";
                 std::cerr << warning_msg << std::endl;
                 compilationWarnings.push_back(warning_msg);
            }
            symbolTable.marcarUsado(id);
            gera_cod("LD", id);
            break;
        }
        case 19: { // #19: Após NUM_INT/FLOAT em <primary> (Expressão)
            gera_cod("LDI", token->getLexeme());
            break;
        }
        case 20: { // #20: Fim do Programa
            gera_cod("HLT", "0");
            break;
        }
        case 21: { // #21: Após '+' em <expression_tail>
            std::cerr << "AVISO - Geração de código para '+' INCOMPLETA." << std::endl;
            break;
        }
         case 22: { // #22: Após '-' em <expression_tail>
            std::cerr << "AVISO - Geração de código para '-' INCOMPLETA." << std::endl;
            break;
        }
        default:
            std::cerr << "AVISO - Ação semântica #" << action
                      << " desconhecida ou não implementada." << std::endl;
            break;
    }
}

// --- Implementação de formatarTabelaSimbolos (VERSÃO ORIGINAL) ---

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
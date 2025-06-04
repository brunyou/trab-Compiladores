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
    // Temporários são escalares, aloca 1 posição com valor 0
    gera_data(temp_name, "0", 1);
    return temp_name;
}

void Semantico::free_temp(const std::string& temp) {
    // Em implementações futuras, poderia gerenciar um pool de temporários.
}

void Semantico::gera_cod(const std::string& instruction, const std::string& arg1, const std::string& arg2) {
    std::string line = "    " + instruction;
    if (!arg1.empty()) line += "\t" + arg1;
    if (!arg2.empty()) line += "\t" + arg2;
    text_section.push_back(line);
    std::cerr << "DEBUG - GERA_COD: " << line << std::endl;
}

// --- gera_data AJUSTADA para o formato "label: 0,0,0" ---
void Semantico::gera_data(const std::string& label, const std::string& value, int count) {
    // Garante que o rótulo não seja duplicado
    bool found = false;
    for(const auto& line : data_section) {
        // Procura por "label:" no início da linha, considerando espaços antes de ':'
        if(line.rfind(label + ":", 0) == 0 || line.rfind(label + " :", 0) == 0) {
            found = true;
            std::cerr << "AVISO - Tentativa de gerar dado duplicado para: " << label << " (já existe)." << std::endl;
            break;
        }
    }
    if (found) {
        return; // Não adiciona se o rótulo principal já existe
    }

    std::string data_line = label + ":\t" + value; // Primeiro valor
    for (int i = 1; i < count; ++i) { // Começa de 1 pois o primeiro já foi adicionado
        data_line += ", " + value;    // Adiciona ", valor" para os subsequentes
    }
    data_section.push_back(data_line); // Adiciona a linha única construída
    std::cerr << "DEBUG - GERA_DATA: " << data_line << std::endl;
}


std::string Semantico::get_generated_code() {
    std::stringstream ss;
    ss << ".data\n";
    for (const auto& line : data_section) {
        ss << "    " << line << "\n";
    }
    ss << "\n.text\n_PRINCIPAL:\n";
    for (const auto& line : text_section) {
        ss << line << "\n";
    }
    return ss.str();
}

// --- Implementação de executeAction (mantida como na última versão completa) ---
void Semantico::executeAction(int action, const Token *token) {
    Symbol symbol_for_table_insertion;

    std::cerr << "DEBUG - Semantico::executeAction: action=" << action;
    if (token) {
        std::cerr << ", token_lexeme='" << token->getLexeme() << "'"
                  << ", token_id=" << token->getId()
                  << ", token_pos=" << token->getPosition();
    } else {
        std::cerr << ", token=NULL (Esperado para algumas ações epsilon como #29)";
    }
    std::cerr << std::endl;

    if (!token && (action == 1 || action == 2 || action == 3 || action == 4 || action == 6 ||
                   action == 8 ||
                   action == 12 || action == 13 || action == 14 ||
                   action == 18 || action == 19 ||
                   action == 26 || action == 27 || action == 28 )) {
         if (action == 2 || action == 11 || action == 13 || action == 18 || action == 19 || action == 26 || action == 28) {
            throw SemanticError("Token nulo inesperado para Ação Semântica #" + std::to_string(action) + " que requer um lexema.", (token ? token->getPosition() : -1) );
         }
    }


    switch (action) {
        case 1: { // #1: DECLARAÇÃO: Após <type>
            if (!token) throw SemanticError("Token nulo para Ação #1 (tipo esperado).", -1);
            std::string recognized_type = token->getLexeme();
            std::string escopoAtual = symbolTable.getEscopoAtual();
            std::cerr << "DEBUG - Action #1: Processing " << pending_declarations_list.size()
                      << " declarações pendentes com tipo '" << recognized_type << "'" << std::endl;

            if (recognized_type == "void") {
                 if (!pending_declarations_list.empty()) {
                    throw SemanticError("Variáveis ou vetores não podem ser do tipo 'void'.", pending_declarations_list.front().position );
                 }
            } else if (recognized_type != "int" && recognized_type != "float" && recognized_type != "string" &&
                       recognized_type != "bool" && recognized_type != "char" && recognized_type != "double") {
                 if (!pending_declarations_list.empty() || token->getId() != t_VOID_KEYWORD) {
                    throw SemanticError("Tipo inválido na declaração: '" + recognized_type + "'", token->getPosition());
                 }
            }

            for (const auto& item : pending_declarations_list) {
                if (symbolTable.existeNoEscopoAtual(item.name)) {
                    throw SemanticError("Identificador '" + item.name + "' já declarado no escopo '" + escopoAtual + "'.", item.position);
                }
                symbol_for_table_insertion = Symbol(item.name, recognized_type, escopoAtual, item.position);
                symbol_for_table_insertion.vetor = item.is_vector;
                symbolTable.inserir(symbol_for_table_insertion);
                gera_data(item.name, "0", item.vector_size); // Passa o tamanho para gera_data
            }
            pending_declarations_list.clear();
            break;
        }
        case 2: { // #2: DECLARAÇÃO: Após ID em <id_declaration_spec>
            if (!token) throw SemanticError("Token nulo para Ação #2 (ID esperado).", -1);
            this->current_dec_id_name = token->getLexeme();
            this->current_dec_id_pos = token->getPosition();
            this->current_dec_is_vector = false;
            this->current_dec_vector_size = 1;
            std::cerr << "DEBUG - Action #2: ID pendente para declaração: '" << this->current_dec_id_name << "'" << std::endl;
            break;
        }
        case 3: { // #3: Após ID em <var_or_array_lhs> (LHS da atribuição)
            if (!token) throw SemanticError("Token nulo para Ação #3 (ID do LHS esperado).", -1);
            this->active_id_name = token->getLexeme();
            this->active_id_pos = token->getPosition();
            this->active_id_is_vector_access = false;
            this->active_id_context = ActiveIdContext::LHS_ASSIGNMENT;

            std::optional<Symbol> symbol_opt = symbolTable.buscar(this->active_id_name);
            if (!symbol_opt) {
                throw SemanticError("Variável '" + this->active_id_name + "' não declarada.", this->active_id_pos);
            }
            if (symbol_opt->funcao) {
                throw SemanticError("Identificador '" + this->active_id_name + "' é uma função e não pode receber atribuição.", this->active_id_pos);
            }
            this->currentLHS_idType = symbol_opt->tipo;
            this->currentLHS_idName = this->active_id_name;
            this->currentLHS_idPosition = this->active_id_pos;
            symbolTable.marcarUsado(this->active_id_name);
            break;
        }
        case 4: { // #4: ATRIBUIÇÃO: Após <expression> no RHS
            if (!token) throw SemanticError("Token nulo para Ação #4 (fim da expressão RHS esperado).", -1);
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
            } else {
                this->currentRHS_expressionType = "tipo_desconhecido_expr";
                 std::cerr << "AVISO - Ação #4: Tipo da expressão RHS não pôde ser determinado diretamente pelo último token." << std::endl;
            }
            break;
        }
        case 5: { // #5: ATRIBUIÇÃO: Final da atribuição (após ';')
            if (this->active_id_name.empty()) {
                 if(this->currentLHS_idName.empty()){
                    std::cerr << "AVISO - Ação #5 chamada sem ID de LHS ativo/current." << std::endl;
                    break;
                 }
                 this->active_id_name = this->currentLHS_idName;
                 this->active_id_pos = this->currentLHS_idPosition;
                 this->active_id_is_vector_access = false;
            }
            if (this->currentLHS_idType.empty() && !this->active_id_name.empty()) {
                 std::optional<Symbol> sym_opt_lhs = symbolTable.buscar(this->active_id_name);
                 if(sym_opt_lhs) this->currentLHS_idType = sym_opt_lhs->tipo;
            }
            if (this->currentLHS_idType.empty() || (this->currentRHS_expressionType.empty() || this->currentRHS_expressionType == "tipo_desconhecido_expr")) {
                 throw SemanticError("Não foi possível determinar tipos para atribuição a '" + this->active_id_name + "'. LHS: " + this->currentLHS_idType + ", RHS: " + this->currentRHS_expressionType, this->active_id_pos);
            }
            bool compatible = false;
            if (this->currentLHS_idType == this->currentRHS_expressionType) compatible = true;
            else if (this->currentLHS_idType == "float" && this->currentRHS_expressionType == "int") compatible = true;
            else if (this->currentLHS_idType == "double" && (this->currentRHS_expressionType == "int" || this->currentRHS_expressionType == "float")) compatible = true;
            if (!compatible) {
                throw SemanticError("Tipos incompatíveis na atribuição para '" + this->active_id_name +
                                    "': " + this->currentLHS_idType + " = " + this->currentRHS_expressionType,
                                    this->active_id_pos);
            }
            if (this->active_id_is_vector_access) {
                gera_cod("STOV", this->active_id_name);
            } else {
                gera_cod("STO", this->active_id_name);
            }
            symbolTable.marcarInicializado(this->active_id_name);
            this->active_id_name.clear(); this->active_id_pos = 0; this->active_id_is_vector_access = false; this->active_id_context = ActiveIdContext::NONE;
            this->currentLHS_idName.clear(); this->currentLHS_idType.clear(); this->currentLHS_idPosition = -1; this->currentRHS_expressionType.clear();
            break;
        }
        case 6: { symbolTable.entrarEscopoBloco(); break; }
        case 7: {
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
        case 8: {
            if (!token) throw SemanticError("Token nulo para Ação #8 (nome da função esperado).", -1);
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
            symbol_for_table_insertion = Symbol(this->currentProcessingFunctionName, this->pendingFunctionReturnType, this->currentFunctionDeclarationScope, funcPos);
            symbol_for_table_insertion.funcao = true;
            symbolTable.inserir(symbol_for_table_insertion);
            symbolTable.entrarEscopoFuncao(this->currentProcessingFunctionName);
            currentFunctionParamsInfoList.clear();
            paramPositionCounter = 0;
            gera_cod(this->currentProcessingFunctionName + ":");
            break;
        }
        case 9: {
            std::cerr << "DEBUG - Action #9 (FUNC_DEC - END PARAMS): Finalizando assinatura para func '" << this->currentProcessingFunctionName
                      << "' (declarada em '" << this->currentFunctionDeclarationScope << "') com "
                      << this->currentFunctionParamsInfoList.size() << " params." << std::endl;
            Symbol* funcSym = symbolTable.buscarParaModificacaoNoEscopo(this->currentProcessingFunctionName, this->currentFunctionDeclarationScope);
            if (funcSym && funcSym->funcao) {
                funcSym->assinaturaParametros = this->currentFunctionParamsInfoList;
            } else {
                throw SemanticError("Erro interno: Função '" + this->currentProcessingFunctionName + "' não encontrada no escopo '" + this->currentFunctionDeclarationScope + "' para atualizar assinatura.", (token ? token->getPosition() : -1));
            }
            currentFunctionParamsInfoList.clear();
            break;
        }
        case 10: {
            std::string escopoDaFuncaoQueTermina = symbolTable.getEscopoAtual();
            std::cerr << "DEBUG - Action #10 (FUNC_DEC - END BODY): Verificando não usados e saindo do escopo da função '" << escopoDaFuncaoQueTermina << "'" << std::endl;
            std::vector<Symbol> simbolosDoEscopoFunc = symbolTable.getSimbolosDoEscopo(escopoDaFuncaoQueTermina);
            for (const auto& sym : simbolosDoEscopoFunc) {
                if (!sym.usado && !sym.funcao ) {
                    std::string modalidade = sym.parametro ? "Parâmetro" : "Variável local";
                     if (sym.parametro && sym.usado) continue;
                    std::string warning_msg = "AVISO SEMÂNTICO: " + modalidade + " '" + sym.id + "' (tipo " + sym.tipo +
                                              ") declarado na função '" + (this->currentProcessingFunctionName.empty() ? escopoDaFuncaoQueTermina : this->currentProcessingFunctionName) +
                                              "' mas não foi usado (declarado na linha aprox. " +
                                              std::to_string(sym.linhaDeclaracao) + ").";
                    std::cerr << warning_msg << std::endl;
                    compilationWarnings.push_back(warning_msg);
                }
            }
            symbolTable.sairEscopo();
            gera_cod("RETURN");
            this->currentProcessingFunctionName.clear();
            this->pendingFunctionReturnType.clear();
            this->currentFunctionDeclarationScope.clear();
            break;
        }
        case 11: {
            if (!token) throw SemanticError("Token nulo para Ação #11 (tipo de retorno esperado).", -1);
            this->pendingFunctionReturnType = token->getLexeme();
            std::cerr << "DEBUG - Action #11 (FUNC_DEC - RETURN TYPE): Tipo de retorno pendente: '" << this->pendingFunctionReturnType << "'" << std::endl;
            break;
        }
        case 13: {
            if (!token) throw SemanticError("Token nulo para Ação #13 (tipo de parâmetro esperado).", -1);
            this->pendingParamType = token->getLexeme();
            this->pendingParamIsArray = false;
            std::cerr << "DEBUG - Action #13 (PARAM_DEC - TYPE): Tipo de parâmetro pendente: '" << this->pendingParamType << "'" << std::endl;
            break;
        }
        case 12: {
            if (!token) throw SemanticError("Token nulo para Ação #12 (ID de parâmetro esperado).", -1);
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
            symbol_for_table_insertion = Symbol(paramName, this->pendingParamType, escopoAtualFunc, paramTokenPos);
            symbol_for_table_insertion.parametro = true;
            symbol_for_table_insertion.inicializado = true;
            symbol_for_table_insertion.posicaoParametro = this->paramPositionCounter++;
            symbolTable.inserir(symbol_for_table_insertion);
            ParameterInfo paramInfo;
            paramInfo.nome = paramName;
            paramInfo.tipo = this->pendingParamType;
            paramInfo.isArray = false;
            paramInfo.linhaDeclaracaoParam = paramTokenPos;
            currentFunctionParamsInfoList.push_back(paramInfo);
            this->pendingParamIsArray = false;
            gera_data(paramName, "0");
            break;
        }
        case 14: {
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
        case 15: {
            std::cerr << "DEBUG - Action #15 (PARAM_DEC - IS_SCALAR)" << std::endl;
            break;
        }
        case 16: { // #16: FINAL de <input> (após ID #26 <optional_array_access (#23/#24 ou #25)>)
            if (active_id_name.empty() || active_id_context != ActiveIdContext::LEIA_TARGET) {
                throw SemanticError("Erro interno: ID alvo para LEIA não definido corretamente.", (token ? token->getPosition() : active_id_pos));
            }
            gera_cod("LD", "$in_port");
            if (this->active_id_is_vector_access) {
                gera_cod("STOV", this->active_id_name);
            } else {
                gera_cod("STO", this->active_id_name);
            }
            symbolTable.marcarInicializado(this->active_id_name);
            symbolTable.marcarUsado(this->active_id_name);
            this->active_id_name.clear(); this->active_id_pos = 0; this->active_id_is_vector_access = false; this->active_id_context = ActiveIdContext::NONE;
            break;
        }
        case 17: { // #17: Após <expression> em <output> (ESCREVA)
            gera_cod("STO", "$out_port");
            if (this->active_id_context == ActiveIdContext::RHS_EXPRESSION) {
                 this->active_id_name.clear(); this->active_id_pos = 0; this->active_id_is_vector_access = false; this->active_id_context = ActiveIdContext::NONE;
            }
            break;
        }
        case 18: { // #18: Após ID em <primary> (Antes de <optional_array_access>)
            if (!token) throw SemanticError("Token nulo para Ação #18 (ID em primário esperado).", -1);
            this->active_id_name = token->getLexeme();
            this->active_id_pos = token->getPosition();
            this->active_id_is_vector_access = false;
            this->active_id_context = ActiveIdContext::RHS_EXPRESSION;

            std::optional<Symbol> sym_opt = symbolTable.buscar(this->active_id_name);
            if (!sym_opt) throw SemanticError("Variável '" + this->active_id_name + "' não declarada.", this->active_id_pos);
            if (sym_opt->funcao && sym_opt->tipo == "void" && active_id_context == ActiveIdContext::RHS_EXPRESSION){
                 throw SemanticError("Procedimento '" + this->active_id_name + "' do tipo 'void' não pode ser usado em expressão.", this->active_id_pos);
            }
            if (!sym_opt->inicializado && !sym_opt->parametro && !sym_opt->funcao) {
                 std::string warning_msg = "AVISO SEMÂNTICO: Variável '" + this->active_id_name + "' usada sem ter sido inicializada (Pos: " + std::to_string(this->active_id_pos) + ").";
                 std::cerr << warning_msg << std::endl;
                 compilationWarnings.push_back(warning_msg);
            }
            symbolTable.marcarUsado(this->active_id_name);
            break;
        }
        case 19: { // #19: Após LITERAL (INTEGER, FLOAT, STRING) em <primary>
            if (!token) throw SemanticError("Token nulo para Ação #19 (Literal esperado).", -1);
            gera_cod("LDI", token->getLexeme());
            // Não reseta active_id_context aqui
            break;
        }
        case 20: { // #20: Fim do Programa (após <optional_stmt_list>)
            gera_cod("HLT", "0");
            break;
        }
        case 21: {
            std::cerr << "AVISO - Geração de código para '+' INCOMPLETA. Requer pilha semântica/AST." << std::endl;
            break;
        }
         case 22: {
            std::cerr << "AVISO - Geração de código para '-' INCOMPLETA. Requer pilha semântica/AST." << std::endl;
            break;
        }
        case 23: { // #23: Após <expression> do índice do vetor
            gera_cod("STO", "$indr");
            break;
        }
        case 24: { // #24: Após ']' em <optional_array_access>
            if (this->active_id_name.empty()){
                throw SemanticError("Erro interno: acesso a vetor sem ID ativo.", (token ? token->getPosition() : active_id_pos));
            }
            std::optional<Symbol> sym_opt = symbolTable.buscar(this->active_id_name);
            if (!sym_opt || !sym_opt->vetor) {
                 throw SemanticError("Identificador '" + this->active_id_name + "' não é um vetor ou não foi declarado como tal para acesso com [].", this->active_id_pos);
            }
            this->active_id_is_vector_access = true;
            std::cerr << "DEBUG - Action #24: ID '" << active_id_name << "' MARCADO como ACESSO VETORIAL." << std::endl;
            break;
        }
        case 25: { // #25: Após î em <optional_array_access> (escalar)
             if (!this->active_id_name.empty()) {
                std::optional<Symbol> sym_opt = symbolTable.buscar(this->active_id_name);
                if (sym_opt && sym_opt->vetor) {
                    throw SemanticError("Identificador '" + this->active_id_name + "' é um vetor e deve ser acessado com índice [].", this->active_id_pos);
                }
                this->active_id_is_vector_access = false;
             }
             std::cerr << "DEBUG - Action #25: ID '" << active_id_name << "' MARCADO como ACESSO ESCALAR." << std::endl;
            break;
        }
        case 26: { // #26: Após ID em <input> (Antes de <optional_array_access>)
            if (!token) throw SemanticError("Token nulo para Ação #26 (ID para LEIA esperado).", -1);
            this->active_id_name = token->getLexeme();
            this->active_id_pos = token->getPosition();
            this->active_id_is_vector_access = false;
            this->active_id_context = ActiveIdContext::LEIA_TARGET;

            std::optional<Symbol> sym_opt = symbolTable.buscar(this->active_id_name);
            if (!sym_opt) throw SemanticError("Variável '" + this->active_id_name + "' não declarada para LEIA.", this->active_id_pos);
            if (sym_opt->funcao) throw SemanticError("Não é possível ler em uma função ('" + this->active_id_name + "').", this->active_id_pos);
            break;
        }
        case 27: { // #27: Após <ID #18 <optional_array_access>> em <primary> (LOAD ID/LDV ID)
            if (active_id_name.empty() || active_id_context != ActiveIdContext::RHS_EXPRESSION) {
                throw SemanticError("Erro interno: ID para expressão RHS não definido corretamente antes de LD/LDV.", (token ? token->getPosition() : active_id_pos));
            }
            if (this->active_id_is_vector_access) {
                gera_cod("LDV", this->active_id_name);
            } else {
                gera_cod("LD", this->active_id_name);
            }
            break;
        }
        case 28: { // #28: Após INTEGER em <optional_curly_array_spec_for_decl> (DECLARAÇÃO DE VETOR)
            if (!token) throw SemanticError("Token nulo para Ação #28 (tamanho do vetor esperado).", -1);
            if (this->current_dec_id_name.empty()) {
                 throw SemanticError("Erro interno: ID para declaração de vetor não definido (Ação #2 falhou ou não foi chamada?).", token->getPosition());
            }
            int size = 0;
            try {
                size = std::stoi(token->getLexeme());
            } catch (const std::exception& e) {
                throw SemanticError("Tamanho do vetor inválido: '" + token->getLexeme() + "'.", token->getPosition());
            }
            if (size <= 0) {
                 throw SemanticError("Tamanho do vetor deve ser positivo: " + std::to_string(size) + ".", token->getPosition());
            }
            pending_declarations_list.push_back({this->current_dec_id_name, this->current_dec_id_pos, true, size});
            std::cerr << "DEBUG - Action #28: Pending VETOR '" << this->current_dec_id_name << "' size " << size << " adicionado à lista." << std::endl;
            this->current_dec_id_name.clear();
            this->current_dec_id_pos = 0;
            break;
        }
        case 29: { // #29: Após î em <optional_curly_array_spec_for_decl> (DECLARAÇÃO DE ESCALAR)
            if (this->current_dec_id_name.empty()) {
                 throw SemanticError("Erro interno: ID para declaração escalar não definido.", (token ? token->getPosition() : this->current_dec_id_pos));
            }
            pending_declarations_list.push_back({this->current_dec_id_name, this->current_dec_id_pos, false, 1});
            std::cerr << "DEBUG - Action #29: Pending ESCALAR '" << this->current_dec_id_name << "' adicionado à lista." << std::endl;
            this->current_dec_id_name.clear();
            this->current_dec_id_pos = 0;
            break;
        }

        default:
            std::cerr << "AVISO - Ação semântica #" << action
                      << " desconhecida ou não implementada." << std::endl;
            break;
    }
}

// --- formatarTabelaSimbolos (MANTIDA COMO A SUA ORIGINAL) ---
std::string Semantico::formatarTabelaSimbolos() {
    std::string output = "";
    std::vector<Symbol> tabela = symbolTable.getTabela();
    if (tabela.empty()) {
        return "";
    }
    output += "ID,Tipo,Inicializado,Usado,Escopo,Parametro,PosParametro,Vetor,Funcao,LinhaDec\n";
    for (const auto& s : tabela) {
        output += s.id + "," +
                  s.tipo + "," +
                  (s.inicializado ? "T" : "F") + "," +
                  (s.usado ? "T" : "F") + "," +
                  s.escopo + "," +
                  (s.parametro ? "T" : "F") + "," +
                  std::to_string(s.posicaoParametro) + "," +
                  (s.vetor ? "T" : "F") + "," +
                  (s.funcao ? "T" : "F") + "," +
                  std::to_string(s.linhaDeclaracao) + "\n";
    }
    return output;
}
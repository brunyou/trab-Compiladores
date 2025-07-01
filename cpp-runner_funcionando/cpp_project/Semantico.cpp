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
    ss << "T" << temp_address_start++; // Adiciona "T" antes do número
    std::string temp_name = ss.str();
    gera_data(temp_name, "0", 1);
    return temp_name;
}

void Semantico::free_temp(const std::string& temp) {
    std::cerr << "DEBUG - FREE_TEMP (simulado): " << temp << std::endl;
}

void Semantico::gera_cod(const std::string& instruction, const std::string& arg1, const std::string& arg2) {
    std::string line = "    " + instruction;
    if (!arg1.empty()) line += "\t" + arg1;
    if (!arg2.empty()) line += "\t" + arg2;
    text_section.push_back(line);
    std::cerr << "DEBUG - GERA_COD: " << line << std::endl;
}

void Semantico::gera_data(const std::string& label, const std::string& value, int count) {
    bool found = false;
    for(const auto& line : data_section) {
        if(line.rfind(label + ":", 0) == 0 || line.rfind(label + " :", 0) == 0) {
            found = true;
            std::cerr << "AVISO - Tentativa de gerar dado duplicado para: " << label << " (já existe)." << std::endl;
            break;
        }
    }
    if (found) return;

    std::string data_line = label + ":\t" + value;
    for (int i = 1; i < count; ++i) {
        data_line += ", " + value;
    }
    data_section.push_back(data_line);
    std::cerr << "DEBUG - GERA_DATA: " << data_line << std::endl;
}

std::string Semantico::get_generated_code() {
    std::stringstream ss;
    if (!data_section.empty()) {
        ss << ".data\n";
        for (const auto& line : data_section) {
            ss << "    " << line << "\n";
        }
        ss << "\n";
    }
    ss << ".text\n_PRINCIPAL:\n";
    for (const auto& line : text_section) {
        ss << line << "\n";
    }
    return ss.str();
}

// --- Implementação Principal das Ações Semânticas ---
void Semantico::executeAction(int action, const Token *token) {
    Symbol symbol_for_table_insertion;

    std::cerr << "DEBUG - Semantico::executeAction: action=" << action;
    if (token) {
        std::cerr << ", token_lexeme='" << token->getLexeme() << "'"
                  << ", token_id=" << token->getId()
                  << ", token_pos=" << token->getPosition();
    } else {
        std::cerr << ", token=NULL (Pode ser esperado para algumas ações)";
    }
    std::cerr << std::endl;

    if (!token && (action == 1 || action == 2 || action == 3 || action == 4 || action == 6 ||
                   action == 8 || action == 11 || action == 12 || action == 13 ||
                   action == 18 || action == 19 || action == 21 || action == 22 ||
                   action == 26 || action == 28 )) {
         throw SemanticError("Token nulo inesperado para Ação Semântica #" + std::to_string(action) + " que requer um lexema de token.", (token ? token->getPosition() : -1) );
    }


    switch (action) {
        // --- AÇÕES DE DECLARAÇÃO DE VARIÁVEIS E VETORES ---
        case 1: { // #1: Finaliza declarações: Após <type> em <dec_stmt>
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
                 if (!pending_declarations_list.empty() || (token->getId() != t_VOID_KEYWORD && pending_declarations_list.empty() && currentProcessingFunctionName.empty() )) {
                    throw SemanticError("Tipo inválido na declaração de variável/vetor: '" + recognized_type + "'", token->getPosition());
                 }
            }

            for (const auto& item : pending_declarations_list) {
                if (symbolTable.existeNoEscopoAtual(item.name)) {
                    throw SemanticError("Identificador '" + item.name + "' já declarado no escopo '" + escopoAtual + "'.", item.position);
                }
                symbol_for_table_insertion = Symbol(item.name, recognized_type, escopoAtual, item.position);
                symbol_for_table_insertion.vetor = item.is_vector;
                symbolTable.inserir(symbol_for_table_insertion);
                gera_data(item.name, "0", item.vector_size);
            }
            pending_declarations_list.clear();
            break;
        }
        case 2: { // #2: DECLARAÇÃO: Após ID em <single_id_or_array_decl>
            if (!token) throw SemanticError("Token nulo para Ação #2 (ID esperado).", -1);
            this->current_dec_id_name = token->getLexeme();
            this->current_dec_id_pos = token->getPosition();
            this->current_dec_is_vector = false;
            this->current_dec_vector_size = 1;
            std::cerr << "DEBUG - Action #2: ID pendente para declaração: '" << this->current_dec_id_name << "'" << std::endl;
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
                 throw SemanticError("Erro interno: ID para declaração escalar não definido.", this->current_dec_id_pos );
            }
            pending_declarations_list.push_back({this->current_dec_id_name, this->current_dec_id_pos, false, 1});
            std::cerr << "DEBUG - Action #29: Pending ESCALAR '" << this->current_dec_id_name << "' adicionado à lista." << std::endl;
            this->current_dec_id_name.clear();
            this->current_dec_id_pos = 0;
            break;
        }

        // --- AÇÕES DE ATRIBUIÇÃO ---
        case 3: { // #3: Após ID em <var_or_array_lhs> (LHS da atribuição)
            if (!token) throw SemanticError("Token nulo para Ação #3 (ID do LHS esperado).", -1);

            // Guarda as informações específicas do ALVO da atribuição
            this->assignment_target_id_name = token->getLexeme();
            this->currentLHS_idPosition = token->getPosition(); // Usado para mensagens de erro em #5
            this->assignment_target_is_vector_element = false;  // Será true se #24 for chamada para este LHS

            // active_id_name é usado para o processamento de <optional_array_access>
            this->active_id_name = token->getLexeme();
            this->active_id_pos = token->getPosition();
            this->active_id_is_vector_access = false; // #24 ou #25 definirão isso para active_id
            this->active_id_context = ActiveIdContext::LHS_ASSIGNMENT;

            std::optional<Symbol> symbol_opt = symbolTable.buscar(this->assignment_target_id_name);
            if (!symbol_opt) {
                throw SemanticError("Variável '" + this->assignment_target_id_name + "' não declarada.", this->active_id_pos);
            }
            if (symbol_opt->funcao) {
                throw SemanticError("Identificador '" + this->assignment_target_id_name + "' é uma função e não pode receber atribuição.", this->active_id_pos);
            }
            // Guarda tipo do LHS para verificação de tipo em #5
            this->currentLHS_idType = symbol_opt->tipo;

            symbolTable.marcarUsado(this->assignment_target_id_name);
            std::cerr << "DEBUG - Action #3: LHS target '" << this->assignment_target_id_name << "' (pos: " << this->active_id_pos << ") context LHS_ASSIGNMENT" << std::endl;
            break;
        }
        case 4: { // #4: ATRIBUIÇÃO: Após <expression> no RHS (Semântica de Tipo)
            if (!token && current_expression_accumulator_temp.empty()) throw SemanticError("Token nulo e acumulador vazio para Ação #4.", -1);
            std::string expr_final_type;
            if (!current_expression_accumulator_temp.empty()) {
                expr_final_type = "int"; // TODO: Melhorar inferência de tipo
                std::cerr << "AVISO - Action #4: Usando tipo 'int' default para expressão complexa RHS." << std::endl;
            } else if (token) {
                 if (token->getId() == t_INTEGER) { expr_final_type = "int"; }
                 else if (token->getId() == t_FLOAT) { expr_final_type = "float"; }
                 else if (token->getId() == t_STRING) { expr_final_type = "string"; }
                 else if (token->getId() == t_ID) {
                     std::optional<Symbol> rhs_sym_opt = symbolTable.buscar(token->getLexeme());
                     if (!rhs_sym_opt) throw SemanticError("Variável RHS '" + token->getLexeme() + "' não declarada.", token->getPosition());
                     if (rhs_sym_opt->funcao && rhs_sym_opt->tipo == "void") throw SemanticError("Procedimento void '" + token->getLexeme() + "' não pode ser usado em expressão.", token->getPosition());
                     if (!rhs_sym_opt->inicializado && !rhs_sym_opt->parametro && !rhs_sym_opt->funcao) { /* ... aviso ... */ }
                     symbolTable.marcarUsado(token->getLexeme());
                     expr_final_type = rhs_sym_opt->tipo;
                 } else {expr_final_type = "tipo_desconhecido_expr";}
            } else {
                throw SemanticError("Erro interno: Não foi possível determinar o tipo da expressão RHS na Ação #4.", -1);
            }
            this->currentRHS_expressionType = expr_final_type;
            std::cerr << "DEBUG - Action #4: Tipo RHS determinado como '" << this->currentRHS_expressionType << "'" << std::endl;
            break;
        }
        case 5: { // #5: ATRIBUIÇÃO: Final da atribuição (após ';')
            if (this->assignment_target_id_name.empty()) {
                std::cerr << "AVISO - Ação #5 chamada sem ID de LHS (assignment_target_id_name) definido." << std::endl;
                this->active_id_name.clear(); this->active_id_pos = 0; this->active_id_is_vector_access = false; this->active_id_context = ActiveIdContext::NONE;
                this->currentLHS_idType.clear(); this->currentLHS_idPosition = -1; this->currentRHS_expressionType.clear();
                if(!current_expression_accumulator_temp.empty()) {free_temp(current_expression_accumulator_temp); current_expression_accumulator_temp.clear();}
                break;
            }

            if (this->currentLHS_idType.empty() || (this->currentRHS_expressionType.empty() || this->currentRHS_expressionType == "tipo_desconhecido_expr")) {
                 throw SemanticError("Não foi possível determinar tipos para atribuição a '" + this->assignment_target_id_name + "'. LHS: " + this->currentLHS_idType + ", RHS: " + this->currentRHS_expressionType, this->currentLHS_idPosition);
            }
            bool compatible = (this->currentLHS_idType == this->currentRHS_expressionType) || (this->currentLHS_idType == "float" && this->currentRHS_expressionType == "int") || (this->currentLHS_idType == "double" && (this->currentRHS_expressionType == "int" || this->currentRHS_expressionType == "float"));
            if (!compatible) {
                throw SemanticError("Tipos incompatíveis na atribuição para '" + this->assignment_target_id_name +
                                    "': " + this->currentLHS_idType + " = " + this->currentRHS_expressionType,
                                    this->currentLHS_idPosition);
            }

            if (current_expression_accumulator_temp.empty()) {
                 std::cerr << "DEBUG - Action #5: RHS era simples, valor já no acumulador BIP." << std::endl;
            } else {
                gera_cod("LD", current_expression_accumulator_temp);
                free_temp(current_expression_accumulator_temp);
                current_expression_accumulator_temp.clear();
            }

            if (this->assignment_target_is_vector_element) { // Usa o flag específico do alvo da atribuição
                gera_cod("STOV", this->assignment_target_id_name);
            } else {
                gera_cod("STO", this->assignment_target_id_name);
            }
            symbolTable.marcarInicializado(this->assignment_target_id_name);

            // Limpeza
            this->assignment_target_id_name.clear();
            this->assignment_target_is_vector_element = false;
            this->active_id_name.clear();
            this->active_id_pos = 0;
            this->active_id_is_vector_access = false;
            this->active_id_context = ActiveIdContext::NONE;
            this->currentLHS_idType.clear();
            this->currentLHS_idPosition = -1;
            this->currentRHS_expressionType.clear();
            break;
        }

        // --- AÇÕES DE ESCOPO E FUNÇÕES (Lógica original do usuário integrada) ---
        case 6: { symbolTable.entrarEscopoBloco(); break; }
        case 7: { /* ... (lógica de sair escopo e avisos como você implementou) ... */
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
        case 8: { // Início da declaração de função (após ID da função)
            if (!token) throw SemanticError("Token nulo para Ação #8 (nome da função esperado).", -1);
            this->currentProcessingFunctionName = token->getLexeme();
            int funcPos = token->getPosition();
            if (this->pendingFunctionReturnType.empty()) {
                throw SemanticError("Erro interno: tipo de retorno da função não definido antes do nome da função.", funcPos);
            }
            this->currentFunctionDeclarationScope = symbolTable.getEscopoAtual();
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
        case 9: { // Fim da lista de parâmetros da função (após ')')
            Symbol* funcSym = symbolTable.buscarParaModificacaoNoEscopo(this->currentProcessingFunctionName, this->currentFunctionDeclarationScope);
            if (funcSym && funcSym->funcao) {
                funcSym->assinaturaParametros = this->currentFunctionParamsInfoList;
            } else {
                throw SemanticError("Erro interno: Função '" + this->currentProcessingFunctionName + "' não encontrada para atualizar assinatura.", (token ? token->getPosition() : -1));
            }
            currentFunctionParamsInfoList.clear();
            break;
        }
        case 10: { // Fim do corpo da função (após '}')
            std::string escopoDaFuncaoQueTermina = symbolTable.getEscopoAtual();
            // ... (lógica de avisos para não usados, como você implementou) ...
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
        case 11: { // Tipo de retorno da função
            if (!token) throw SemanticError("Token nulo para Ação #11 (tipo de retorno esperado).", -1);
            this->pendingFunctionReturnType = token->getLexeme();
            std::cerr << "DEBUG - Action #11 (FUNC_DEC - RETURN TYPE): Tipo de retorno pendente: '" << this->pendingFunctionReturnType << "'" << std::endl;
            break;
        }
        case 12: { // ID do parâmetro
            if (!token) throw SemanticError("Token nulo para Ação #12 (ID de parâmetro esperado).", -1);
            std::string paramName = token->getLexeme();
            int paramTokenPos = token->getPosition();
            std::string escopoAtualFunc = symbolTable.getEscopoAtual();
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
        case 13: { // Tipo do parâmetro
            if (!token) throw SemanticError("Token nulo para Ação #13 (tipo de parâmetro esperado).", -1);
            this->pendingParamType = token->getLexeme();
            this->pendingParamIsArray = false;
            std::cerr << "DEBUG - Action #13 (PARAM_DEC - TYPE): Tipo de parâmetro pendente: '" << this->pendingParamType << "'" << std::endl;
            break;
        }
        case 14: { // Parâmetro é array `[]`
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
                std::cerr << "WARNING - Action #14: Chamada sem um parâmetro pendente na lista de infos." << std::endl;
            }
            break;
        }
        case 15: { // Parâmetro é escalar (epsilon)
            std::cerr << "DEBUG - Action #15 (PARAM_DEC - IS_SCALAR)" << std::endl;
            break;
        }

        // --- AÇÕES DE LEITURA, ESCRITA, EXPRESSÃO SIMPLES, FIM ---
        case 16: { // #16: FINAL de <input_stmt>
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
        case 17: { // #17: Após <expression> em <output_stmt> (ESCREVA)
            if (current_expression_accumulator_temp.empty()) {
                 std::cerr << "DEBUG - Action #17: Expressão para ESCREVA era simples, valor já no acumulador BIP." << std::endl;
            } else {
                gera_cod("LD", current_expression_accumulator_temp);
                free_temp(current_expression_accumulator_temp);
                current_expression_accumulator_temp.clear();
            }
            gera_cod("STO", "$out_port");
            if (this->active_id_context == ActiveIdContext::RHS_EXPRESSION) {
                 this->active_id_name.clear(); this->active_id_pos = 0; this->active_id_is_vector_access = false; this->active_id_context = ActiveIdContext::NONE;
            }
            break;
        }
        case 18: { // #18: Após ID em <primary>
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
        case 19: { // #19: Após LITERAL em <primary>
            if (!token) throw SemanticError("Token nulo para Ação #19 (Literal esperado).", -1);
            gera_cod("LDI", token->getLexeme());
            break;
        }
        case 20: { // #20: Fim do Programa
            gera_cod("HLT", "0");
            break;
        }

        // --- AÇÕES PARA OPERADORES DE EXPRESSÃO (SOMA/SUBTRAÇÃO) ---
        case 21: { // #21: Após OPERATOR_PLUS
            if (!token) throw SemanticError("Token nulo para Ação #21 (OPERATOR_PLUS esperado).", -1);
            this->last_operator_token = "+";
            std::cerr << "DEBUG - Action #21: Operador '+' registrado." << std::endl;
            break;
        }
         case 22: { // #22: Após OPERATOR_MINUS
            if (!token) throw SemanticError("Token nulo para Ação #22 (OPERATOR_MINUS esperado).", -1);
            this->last_operator_token = "-";
            std::cerr << "DEBUG - Action #22: Operador '-' registrado." << std::endl;
            break;
        }

        // --- AÇÕES PARA ACESSO A VETORES ---
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
                 throw SemanticError("Identificador '" + this->active_id_name + "' não é um vetor.", this->active_id_pos);
            }
            this->active_id_is_vector_access = true;
            // Se o contexto for LHS_ASSIGNMENT, marca que o ALVO da atribuição é um elemento de vetor
            if (this->active_id_context == ActiveIdContext::LHS_ASSIGNMENT) {
                this->assignment_target_is_vector_element = true;
            }
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
                 if (this->active_id_context == ActiveIdContext::LHS_ASSIGNMENT) {
                    this->assignment_target_is_vector_element = false;
                }
             }
             std::cerr << "DEBUG - Action #25: ID '" << active_id_name << "' MARCADO como ACESSO ESCALAR." << std::endl;
            break;
        }
        case 26: { // #26: Após ID em <input_stmt> (Antes de <optional_array_access>)
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
            if (active_id_name.empty()) {
                throw SemanticError("Erro interno: ID para expressão RHS não definido antes de LD/LDV.", (token ? token->getPosition() : active_id_pos));
            }
            // Não verifica active_id_context aqui, pois esta ação é genérica para carregar um operando.
            // O contexto já foi usado em #18 para verificações semânticas.
            if (this->active_id_is_vector_access) { // Flag setada por #24/#25
                gera_cod("LDV", this->active_id_name);
            } else {
                gera_cod("LD", this->active_id_name);
            }
            // O valor está agora no "acumulador" do BIP.
            // Não limpa active_id_name, pois pode ser o primeiro operando de uma expressão (#30 o usará)
            // ou um operando direito (#31/#32 o usarão implicitamente do acumulador).
            break;
        }

        // --- NOVAS AÇÕES PARA AVALIAÇÃO DE EXPRESSÃO LINEAR ---
        case 30: { // #30: INIT_EXPR_ACCUMULATOR (Após o primeiro <term> de <expression>)
            if (!current_expression_accumulator_temp.empty()) {
                std::cerr << "AVISO - Action #30: current_expression_accumulator_temp já estava setado ("
                          << current_expression_accumulator_temp << "). Liberando e sobrescrevendo." << std::endl;
                free_temp(current_expression_accumulator_temp);
            }
            current_expression_accumulator_temp = new_temp();
            gera_cod("STO", current_expression_accumulator_temp);
            std::cerr << "DEBUG - Action #30: Primeiro operando da expressão armazenado em " << current_expression_accumulator_temp << std::endl;
            last_operator_token.clear();
            break;
        }
        case 31: { // #31: PERFORM_ADD (Após <term> que segue OPERATOR_PLUS #21)
            if (current_expression_accumulator_temp.empty()) {
                throw SemanticError("Erro interno: Acumulador de expressão não inicializado para ADD.", (token ? token->getPosition() : -1));
            }
            if (last_operator_token != "+") {
                throw SemanticError("Erro interno: Operador pendente não era '+' para Ação #31.", (token ? token->getPosition() : -1));
            }
            std::string temp_direita = new_temp();
            gera_cod("STO", temp_direita);
            gera_cod("LD", current_expression_accumulator_temp);
            gera_cod("ADD", temp_direita);
            free_temp(current_expression_accumulator_temp);
            free_temp(temp_direita);
            current_expression_accumulator_temp = new_temp();
            gera_cod("STO", current_expression_accumulator_temp);
            std::cerr << "DEBUG - Action #31: ADD executado. Novo resultado em " << current_expression_accumulator_temp << std::endl;
            last_operator_token.clear();
            break;
        }
        case 32: { // #32: PERFORM_SUB (Após <term> que segue OPERATOR_MINUS #22)
            if (current_expression_accumulator_temp.empty()) {
                throw SemanticError("Erro interno: Acumulador de expressão não inicializado para SUB.", (token ? token->getPosition() : -1));
            }
            if (last_operator_token != "-") {
                throw SemanticError("Erro interno: Operador pendente não era '-' para Ação #32.", (token ? token->getPosition() : -1));
            }
            std::string temp_direita = new_temp();
            gera_cod("STO", temp_direita);
            gera_cod("LD", current_expression_accumulator_temp);
            gera_cod("SUB", temp_direita);
            free_temp(current_expression_accumulator_temp);
            free_temp(temp_direita);
            current_expression_accumulator_temp = new_temp();
            gera_cod("STO", current_expression_accumulator_temp);
            std::cerr << "DEBUG - Action #32: SUB executado. Novo resultado em " << current_expression_accumulator_temp << std::endl;
            last_operator_token.clear();
            break;
        }
        
        case 33: { // #33: Após <op_relacional>
            if (!token) throw SemanticError("Token nulo para Ação #33 (operador relacional).", -1);
            this->currentRelationalOperator = token->getLexeme();
            std::cerr << "DEBUG - Action #33: Operador relacional '" << this->currentRelationalOperator << "' armazenado." << std::endl;
            break;
        }

        // Realiza a comparação entre o operando da esquerda (já acumulado) e o da direita (recém-processado).
        case 34: { // #34: Após o <simple> do lado direito da relação
            if (this->currentRelationalOperator.empty()) {
                // Isso pode acontecer se a expressão não for uma relação (ex: if (a+b) ). Não fazemos nada.
                std::cerr << "DEBUG - Action #34: Nenhuma operação relacional pendente. Ignorando." << std::endl;
                break;
            }
            if (this->current_expression_accumulator_temp.empty()) {
                throw SemanticError("Erro interno: Lado esquerdo da comparação não foi acumulado em um temporário.", (token ? token->getPosition() : -1));
            }

            // O operando da direita está no acumulador do BIP (processado por #19 ou #27).
            // O operando da esquerda está no nosso temporário 'current_expression_accumulator_temp'.
            std::string temp_direita = new_temp();
            gera_cod("STO", temp_direita); // Salva o operando da direita
            gera_cod("LD", current_expression_accumulator_temp); // Carrega o operando da esquerda
            gera_cod("SUB", temp_direita); // Calcula (esquerda - direita). O resultado fica no acumulador.

            // Salva o resultado da comparação de volta no temporário principal da expressão.
            gera_cod("STO", current_expression_accumulator_temp);
            free_temp(temp_direita);
            std::cerr << "DEBUG - Action #34: Comparação '" << currentRelationalOperator << "' realizada. Resultado em " << current_expression_accumulator_temp << std::endl;
            break;
        }

        // Gera o desvio condicional com base no resultado da expressão/comparação.
        case 35: { // #35: Após ')' da <condition> do IF
            if (current_expression_accumulator_temp.empty()) {
                 throw SemanticError("Erro interno: Condição do IF não produziu um resultado em temporário.", (token ? token->getPosition() : -1));
            }

            // Carrega o resultado final da condição para o acumulador do BIP.
            gera_cod("LD", current_expression_accumulator_temp);
            free_temp(current_expression_accumulator_temp);
            current_expression_accumulator_temp.clear();

            std::string label_destino = new_label(); // Rótulo para pular se a condição for FALSA.
            labels_stack.push(label_destino);

            // Mapeia o operador para a instrução de desvio INVERSA do BIP.
            if (currentRelationalOperator == ">")       gera_cod("BLE", label_destino);
            else if (currentRelationalOperator == "<")  gera_cod("BGE", label_destino);
            else if (currentRelationalOperator == ">=") gera_cod("BLT", label_destino);
            else if (currentRelationalOperator == "<=") gera_cod("BGT", label_destino);
            else if (currentRelationalOperator == "==") gera_cod("BNE", label_destino);
            else if (currentRelationalOperator == "!=") gera_cod("BEQ", label_destino);
            else {
                // Caso sem operador relacional (ex: if(variavel)).
                // A condição é falsa se o valor for 0.
                gera_cod("BEQ", label_destino);
            }

            currentRelationalOperator.clear(); // Limpa para a próxima condição.
            std::cerr << "DEBUG - Action #35: Desvio condicional para " << label_destino << " gerado." << std::endl;
            break;
        }

        // Trata o início do bloco 'else'.
        case 36: { // #36: Após a palavra-chave ELSE
            if (labels_stack.empty()) {
                throw SemanticError("Erro de aninhamento: 'else' sem um 'if' correspondente.", (token ? token->getPosition() : -1));
            }
            std::string label_fim_if = new_label(); // Novo rótulo para o fim de toda a estrutura.
            std::string label_do_if = labels_stack.top(); // Pega o rótulo que o if usaria para pular.
            labels_stack.pop();

            gera_cod("JMP", label_fim_if); // Se o bloco IF foi executado, pula o bloco ELSE.
            text_section.push_back(label_do_if + ":"); // Posiciona o rótulo para onde o IF pula.

            labels_stack.push(label_fim_if); // Empilha o novo rótulo de fim para a ação #37.
            std::cerr << "DEBUG - Action #36: JMP para " << label_fim_if << " gerado; Rótulo " << label_do_if << " posicionado." << std::endl;
            break;
        }

        // Finaliza a estrutura 'if' ou 'if-else', posicionando o rótulo final.
        case 37: { // #37: No final da estrutura condicional.
            if (labels_stack.empty()) {
                throw SemanticError("Erro de aninhamento: fim de condicional inesperado.", (token ? token->getPosition() : -1));
            }
            std::string label_final = labels_stack.top();
            labels_stack.pop();
            text_section.push_back(label_final + ":"); // Adiciona o rótulo diretamente ao código.
            std::cerr << "DEBUG - Action #37: Rótulo final '" << label_final << "' posicionado." << std::endl;
            break;
        }
         case 38: { // #38: Após o primeiro <simple> de uma <relation>
            // O valor do operando esquerdo (ex: 'a' em "a > b") acabou de ser carregado
            // para o acumulador do BIP pela ação #27.
            if (!current_expression_accumulator_temp.empty()) {
                // Em casos futuros mais complexos, talvez seja preciso gerenciar uma pilha de temps.
                // Por agora, apenas avisamos e sobrescrevemos.
                std::cerr << "AVISO - Action #38: Sobrescrevendo temporário de expressão existente: "
                          << current_expression_accumulator_temp << std::endl;
                free_temp(current_expression_accumulator_temp);
            }
            
            current_expression_accumulator_temp = new_temp();
            gera_cod("STO", current_expression_accumulator_temp);
            std::cerr << "DEBUG - Action #38: Operando esquerdo da relação salvo em " << current_expression_accumulator_temp << std::endl;
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
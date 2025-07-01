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


switch (action)
{
    // --- AÇÕES DE DECLARAÇÃO DE VARIÁVEIS E VETORES ---
    case 1: { // #1: Finaliza declarações: Após <type> em <dec_stmt>
        std::string recognized_type = token->getLexeme();
        for (const auto& item : pending_declarations_list) {
            if (symbolTable.existeNoEscopoAtual(item.name)) {
                throw SemanticError("Identificador '" + item.name + "' já declarado.", item.position);
            }
            Symbol symbol = Symbol(item.name, recognized_type, symbolTable.getEscopoAtual(), item.position);
            symbol.vetor = item.is_vector;
            symbolTable.inserir(symbol);
            gera_data(item.name, "0", item.vector_size);
        }
        pending_declarations_list.clear();
        break;
    }
    case 2: { // #2: Após ID em <single_id_or_array_decl>
        this->current_dec_id_name = token->getLexeme();
        this->current_dec_id_pos = token->getPosition();
        break;
    }
    case 28: { // #28: Declaração de VETOR
        int size = std::stoi(token->getLexeme());
        if (size <= 0) throw SemanticError("Tamanho do vetor deve ser positivo.", token->getPosition());
        pending_declarations_list.push_back({this->current_dec_id_name, this->current_dec_id_pos, true, size});
        this->current_dec_id_name.clear();
        break;
    }
    case 29: { // #29: Declaração de ESCALAR
        pending_declarations_list.push_back({this->current_dec_id_name, this->current_dec_id_pos, false, 1});
        this->current_dec_id_name.clear();
        break;
    }

    // --- AÇÕES DE ATRIBUIÇÃO E EXPRESSÃO (LÓGICA DA PILHA SEMÂNTICA) ---
    case 3: { // #3: Após ID no Lado Esquerdo (LHS) da atribuição
        this->assignment_target_id_name = token->getLexeme();
        this->currentLHS_idPosition = token->getPosition();
        this->assignment_target_is_vector_element = false;
        this->active_id_name = token->getLexeme();
        this->active_id_pos = token->getPosition();
        std::optional<Symbol> symbol_opt = symbolTable.buscar(this->assignment_target_id_name);
        if (!symbol_opt) throw SemanticError("Variável '" + this->assignment_target_id_name + "' não declarada.", this->active_id_pos);
        if (symbol_opt->funcao) throw SemanticError("Não se pode atribuir a uma função.", this->active_id_pos);
        this->currentLHS_idType = symbol_opt->tipo;
        symbolTable.marcarUsado(this->assignment_target_id_name);
        break;
    }
    case 4: { // #4: Após <expression> no RHS (Análise de Tipo)
        // Com a pilha, a análise de tipo se torna mais complexa.
        // Por enquanto, vamos manter uma verificação simplificada.
        std::string expr_final_type = "int"; // Assumindo int para todas as expressões por enquanto.
        this->currentRHS_expressionType = expr_final_type;
        // Validação de compatibilidade de tipos
        bool compatible = (this->currentLHS_idType == this->currentRHS_expressionType) || (this->currentLHS_idType == "float" && this->currentRHS_expressionType == "int");
        if (!compatible) {
            throw SemanticError("Tipos incompatíveis na atribuição para '" + this->assignment_target_id_name + "'.", this->currentLHS_idPosition);
        }
        break;
    }
    case 5: { // #5: Final da Atribuição (;)
        if (operand_stack.empty()) throw SemanticError("Erro interno: expressão do lado direito não produziu resultado na pilha.", -1);
        std::string result_temp = operand_stack.top(); operand_stack.pop();
        gera_cod("LD", result_temp);
        if (this->assignment_target_is_vector_element) {
            gera_cod("STOV", this->assignment_target_id_name);
        } else {
            gera_cod("STO", this->assignment_target_id_name);
        }
        free_temp(result_temp);
        symbolTable.marcarInicializado(this->assignment_target_id_name);
        this->assignment_target_id_name.clear();
        this->assignment_target_is_vector_element = false;
        break;
    }

    // --- AÇÕES DE I/O ---
    case 16: { // #16: Final de <input_stmt>
        gera_cod("LD", "$in_port");
        if (this->active_id_is_vector_access) {
            gera_cod("STOV", this->active_id_name);
        } else {
            gera_cod("STO", this->active_id_name);
        }
        symbolTable.marcarInicializado(this->active_id_name);
        break;
    }
    case 17: { // #17: Final de <output_stmt>
        if (operand_stack.empty()) throw SemanticError("Erro: expressão para ESCREVA não produziu resultado.", -1);
        std::string result_temp = operand_stack.top(); operand_stack.pop();
        gera_cod("LD", result_temp);
        gera_cod("STO", "$out_port");
        free_temp(result_temp);
        break;
    }
    
    // --- CONSTRUÇÃO DE EXPRESSÕES ---
    case 18: { // #18: Após ID em <primary>
        this->active_id_name = token->getLexeme();
        this->active_id_pos = token->getPosition();
        // Verificações semânticas do ID
        auto sym = symbolTable.buscar(active_id_name);
        if(!sym) throw SemanticError("Variável '" + active_id_name + "' não declarada.", active_id_pos);
        if(!sym->inicializado && !sym->parametro) compilationWarnings.push_back("AVISO: Variável '" + active_id_name + "' usada sem inicialização.");
        symbolTable.marcarUsado(active_id_name);
        break;
    }
    case 19: { // #19: Após LITERAL (INTEGER, FLOAT, etc.) em <primary>
        std::string temp = new_temp();
        gera_cod("LDI", token->getLexeme());
        gera_cod("STO", temp);
        operand_stack.push(temp);
        break;
    }
    case 27: { // #27: Após processar um ID ou acesso a vetor em uma expressão
        std::string temp = new_temp();
        if (this->active_id_is_vector_access) {
            gera_cod("LDV", this->active_id_name);
        } else {
            gera_cod("LD", this->active_id_name);
        }
        gera_cod("STO", temp);
        operand_stack.push(temp);
        break;
    }

    // --- OPERAÇÕES ARITMÉTICAS E RELACIONAIS ---
    case 31: { // ADD
        if (operand_stack.size() < 2) throw SemanticError("Operandos insuficientes para soma.",-1);
        std::string op2 = operand_stack.top(); operand_stack.pop();
        std::string op1 = operand_stack.top(); operand_stack.pop();
        gera_cod("LD", op1);
        gera_cod("ADD", op2);
        std::string result_temp = new_temp();
        gera_cod("STO", result_temp);
        operand_stack.push(result_temp);
        free_temp(op1); free_temp(op2);
        break;
    }
    case 32: { // SUB
        if (operand_stack.size() < 2) throw SemanticError("Operandos insuficientes para subtracao.",-1);
        std::string op2 = operand_stack.top(); operand_stack.pop();
        std::string op1 = operand_stack.top(); operand_stack.pop();
        gera_cod("LD", op1);
        gera_cod("SUB", op2);
        std::string result_temp = new_temp();
        gera_cod("STO", result_temp);
        operand_stack.push(result_temp);
        free_temp(op1); free_temp(op2);
        break;
    }
    case 33: { // Guarda operador relacional
        this->currentRelationalOperator = token->getLexeme();
        break;
    }
    case 34: { // Realiza comparação
        if (operand_stack.size() < 2) throw SemanticError("Operandos insuficientes para comparacao.", -1);
        std::string op2 = operand_stack.top(); operand_stack.pop();
        std::string op1 = operand_stack.top(); operand_stack.pop();
        gera_cod("LD", op1);
        gera_cod("SUB", op2);
        std::string result_temp = new_temp();
        gera_cod("STO", result_temp);
        operand_stack.push(result_temp);
        free_temp(op1); free_temp(op2);
        break;
    }
    
    // --- CONTROLE DE FLUXO (IF, WHILE) ---
    case 35: // IF
    case 40: { // WHILE
        if (operand_stack.empty()) throw SemanticError("Condicao nao produziu resultado na pilha.", -1);
        std::string result_temp = operand_stack.top(); operand_stack.pop();
        gera_cod("LD", result_temp);
        free_temp(result_temp);
        std::string label_destino = new_label();
        labels_stack.push(label_destino);
        if (currentRelationalOperator == ">") gera_cod("BLE", label_destino);
        else if (currentRelationalOperator == "<") gera_cod("BGE", label_destino);
        else if (currentRelationalOperator == ">=") gera_cod("BLT", label_destino);
        else if (currentRelationalOperator == "<=") gera_cod("BGT", label_destino);
        else if (currentRelationalOperator == "==") gera_cod("BNE", label_destino);
        else if (currentRelationalOperator == "!=") gera_cod("BEQ", label_destino);
        else { gera_cod("BEQ", label_destino); }
        currentRelationalOperator.clear();
        break;
    }
    case 36: { // ELSE
        if (labels_stack.empty()) throw SemanticError("Erro de aninhamento: 'else' sem 'if'.",-1);
        std::string label_fim_if = new_label();
        std::string label_do_if = labels_stack.top(); labels_stack.pop();
        gera_cod("JMP", label_fim_if);
        text_section.push_back(label_do_if + ":");
        labels_stack.push(label_fim_if);
        break;
    }
    case 37: { // Fim do IF/IF-ELSE
        if (labels_stack.empty()) throw SemanticError("Erro de aninhamento: fim de condicional inesperado.",-1);
        std::string label_final = labels_stack.top(); labels_stack.pop();
        text_section.push_back(label_final + ":");
        break;
    }
    case 39: { // Início do WHILE
        std::string inicio_label = new_label();
        text_section.push_back(inicio_label + ":");
        labels_stack.push(inicio_label);
        break;
    }
    case 41: { // Fim do WHILE
        if (labels_stack.size() < 2) throw SemanticError("Erro de aninhamento: fim de 'while' inesperado.", -1);
        std::string fim_label = labels_stack.top(); labels_stack.pop();
        std::string inicio_label = labels_stack.top(); labels_stack.pop();
        gera_cod("JMP", inicio_label);
        text_section.push_back(fim_label + ":");
        break;
    }

    // --- AÇÕES OBSOLETAS OU NÃO MENCIONADAS (MANTENHA OUTRAS QUE VOCÊ TENHA) ---
    case 20: { gera_cod("HLT", "0"); break; }
    case 30: // Obsoleta
    case 38: { // Obsoleta
        std::cerr << "DEBUG - Action #" << action << ": Ignorada (obsoleta com a nova logica de pilha)." << std::endl;
        break;
    }
    case 42: { // #42: Após a palavra-chave DO
            std::string inicio_label = new_label();
            text_section.push_back(inicio_label + ":"); // Posiciona o rótulo
            labels_stack.push(inicio_label); // Empilha para a ação #43 saber para onde voltar
            std::cerr << "DEBUG - Action #42 (DO_WHILE_START): Rótulo '" << inicio_label << "' gerado e empilhado." << std::endl;
            break;
        }
    case 43: { // #43: Após a condição do DO-WHILE
            if (operand_stack.empty()) {
                throw SemanticError("Condição do 'do-while' não produziu resultado na pilha.", (token ? token->getPosition() : -1));
            }
            if (labels_stack.empty()) {
                throw SemanticError("Erro de aninhamento: fim de laço 'do-while' inesperado.", (token ? token->getPosition() : -1));
            }

            std::string inicio_label = labels_stack.top();
            labels_stack.pop();

            std::string result_temp = operand_stack.top();
            operand_stack.pop();

            gera_cod("LD", result_temp); // Carrega o resultado da condição
            free_temp(result_temp);

            // Lógica de desvio DIRETO (diferente do IF e WHILE)
            // Pula de volta para o início se a condição for VERDADEIRA.
            if (currentRelationalOperator == ">")       gera_cod("BGT", inicio_label);
            else if (currentRelationalOperator == "<")  gera_cod("BLT", inicio_label);
            else if (currentRelationalOperator == ">=") gera_cod("BGE", inicio_label);
            else if (currentRelationalOperator == "<=") gera_cod("BLE", inicio_label);
            else if (currentRelationalOperator == "==") gera_cod("BEQ", inicio_label);
            else if (currentRelationalOperator == "!=") gera_cod("BNE", inicio_label);
            else {
                // Caso sem operador (ex: while(a)). Pula se 'a' for diferente de 0.
                gera_cod("BNE", inicio_label);
            }
            
            currentRelationalOperator.clear();
            std::cerr << "DEBUG - Action #43 (DO_WHILE_END): Desvio condicional para '" << inicio_label << "' gerado." << std::endl;
            break;
        }

    // Adicione aqui quaisquer outras ações que seu compilador usa e que não foram mencionadas...
    // case 6: case 7: case 8: ... etc.

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
#include "Semantico.h"
#include "Constants.h"
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

// --- FUNÇÕES AUXILIARES DE GERAÇÃO DE CÓDIGO ---
std::string Semantico::new_label() {
    return "L" + std::to_string(label_counter++);
}

std::string Semantico::new_temp() {
    std::string temp_name = "T" + std::to_string(temp_address_start++);
    gera_data(temp_name, "0", 1);
    return temp_name;
}

void Semantico::free_temp(const std::string& temp) {
    // Em uma implementação futura, poderíamos reutilizar os temporários.
    // Por enquanto, apenas logamos a liberação.
    std::cerr << "DEBUG - FREE_TEMP: " << temp << std::endl;
}

void Semantico::gera_cod(const std::string& instruction, const std::string& arg1, const std::string& arg2) {
    std::string line = "    " + instruction;
    if (!arg1.empty()) line += "\t" + arg1;
    if (!arg2.empty()) line += "\t" + arg2;

    if (this->is_in_for_post_op_capture) {
        for_post_op_code_buffer.push_back(line);
    } else if (this->is_in_for_body_capture) {
        for_body_code_buffer.push_back(line);
    } else {
        text_section.push_back(line);
    }
}

void Semantico::gera_data(const std::string& label, const std::string& value, int count) {
    bool found = false;
    for(const auto& line : data_section) {
        if(line.rfind(label + ":", 0) == 0) {
            found = true;
            break;
        }
    }
    if (found) return;

    std::string data_line = label + ":\t" + value;
    data_section.push_back(data_line);
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

// --- EXECUÇÃO DAS AÇÕES SEMÂNTICAS ---
void Semantico::executeAction(int action, const Token *token) {
    switch (action) {
        // --- DECLARAÇÕES ---
        case 1: {
            std::string type = token->getLexeme();
            for (const auto& item : pending_declarations_list) {
                if (symbolTable.existeNoEscopoAtual(item.name)) throw SemanticError("Identificador '" + item.name + "' já declarado.", item.position);
                Symbol s(item.name, type, symbolTable.getEscopoAtual(), item.position);
                s.vetor = item.is_vector;
                symbolTable.inserir(s);
                gera_data(item.name, "0", item.vector_size);
            }
            pending_declarations_list.clear();
            break;
        }
        case 2: {
            this->current_dec_id_name = token->getLexeme();
            this->current_dec_id_pos = token->getPosition();
            break;
        }
        case 28: {
            int size = std::stoi(token->getLexeme());
            if (size <= 0) throw SemanticError("Tamanho do vetor deve ser positivo.", token->getPosition());
            pending_declarations_list.push_back({current_dec_id_name, current_dec_id_pos, true, size});
            current_dec_id_name.clear();
            break;
        }
        case 29: {
            pending_declarations_list.push_back({current_dec_id_name, current_dec_id_pos, false, 1});
            current_dec_id_name.clear();
            break;
        }

        // --- ATRIBUIÇÃO E EXPRESSÕES ---
        case 3: {
            assignment_target_id_name = token->getLexeme();
            active_id_name = token->getLexeme();
            break;
        }
        case 5: {
            if (operand_stack.empty()) throw SemanticError("Expressão RHS inválida.",-1);
            std::string result_temp = operand_stack.top(); operand_stack.pop();
            gera_cod("LD", result_temp);
            if (this->assignment_target_is_vector_element) {
                gera_cod("STOV", this->assignment_target_id_name);
            } else {
                gera_cod("STO", this->assignment_target_id_name);
            }
            free_temp(result_temp);
            symbolTable.marcarInicializado(this->assignment_target_id_name);
            assignment_target_id_name.clear();
            break;
        }
        case 19: {
            std::string temp = new_temp();
            gera_cod("LDI", token->getLexeme());
            gera_cod("STO", temp);
            operand_stack.push(temp);
            break;
        }
        case 27: {
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

        // --- OPERAÇÕES ---
        case 31: { // ADD
            if (operand_stack.size() < 2) throw SemanticError("Operandos insuficientes.",-1);
            std::string op2 = operand_stack.top(); operand_stack.pop();
            std::string op1 = operand_stack.top(); operand_stack.pop();
            gera_cod("LD", op1); gera_cod("ADD", op2);
            std::string result_temp = new_temp();
            gera_cod("STO", result_temp);
            operand_stack.push(result_temp);
            free_temp(op1); free_temp(op2);
            break;
        }
        case 32: { // SUB
            if (operand_stack.size() < 2) throw SemanticError("Operandos insuficientes.", -1);
            std::string op2 = operand_stack.top(); operand_stack.pop();
            std::string op1 = operand_stack.top(); operand_stack.pop();
            gera_cod("LD", op1); gera_cod("SUB", op2);
            std::string result_temp = new_temp();
            gera_cod("STO", result_temp);
            operand_stack.push(result_temp);
            free_temp(op1); free_temp(op2);
            break;
        }
        case 34: { // Comparação (usa SUB)
            if (operand_stack.size() < 2) throw SemanticError("Operandos insuficientes.", -1);
            std::string op2 = operand_stack.top(); operand_stack.pop();
            std::string op1 = operand_stack.top(); operand_stack.pop();
            gera_cod("LD", op1); gera_cod("SUB", op2);
            std::string result_temp = new_temp();
            gera_cod("STO", result_temp);
            operand_stack.push(result_temp);
            free_temp(op1); free_temp(op2);
            break;
        }

        // --- CONTROLE DE FLUXO ---
        case 33: { currentRelationalOperator = token->getLexeme(); break; }
        case 35: case 40: { // IF e WHILE (condição)
            if (operand_stack.empty()) throw SemanticError("Condição inválida.", -1);
            std::string result_temp = operand_stack.top(); operand_stack.pop();
            gera_cod("LD", result_temp);
            free_temp(result_temp);
            std::string label = new_label();
            labels_stack.push(label);
            if (currentRelationalOperator == ">") gera_cod("BLE", label);
            else if (currentRelationalOperator == "<") gera_cod("BGE", label);
            else if (currentRelationalOperator == "==") gera_cod("BNE", label);
            else if (currentRelationalOperator == "!=") gera_cod("BEQ", label);
            else if (currentRelationalOperator == ">=") gera_cod("BLT", label);
            else if (currentRelationalOperator == "<=") gera_cod("BGT", label);
            else gera_cod("BEQ", label);
            currentRelationalOperator.clear();
            break;
        }
        case 36: { // ELSE
            std::string fim_if = new_label();
            std::string inicio_else = labels_stack.top(); labels_stack.pop();
            gera_cod("JMP", fim_if);
            text_section.push_back(inicio_else + ":");
            labels_stack.push(fim_if);
            break;
        }
        case 37: { // Fim do IF/ELSE
            std::string label = labels_stack.top(); labels_stack.pop();
            text_section.push_back(label + ":");
            break;
        }
        case 39: case 42: { // Início do WHILE e DO-WHILE
            std::string label = new_label();
            text_section.push_back(label + ":");
            labels_stack.push(label);
            break;
        }
        case 41: { // Fim do WHILE
            std::string fim_label = labels_stack.top(); labels_stack.pop();
            std::string inicio_label = labels_stack.top(); labels_stack.pop();
            gera_cod("JMP", inicio_label);
            text_section.push_back(fim_label + ":");
            break;
        }
        case 43: { // Fim do DO-WHILE
            if (operand_stack.empty()) throw SemanticError("Condição inválida.", -1);
            std::string result_temp = operand_stack.top(); operand_stack.pop();
            std::string inicio_label = labels_stack.top(); labels_stack.pop();
            gera_cod("LD", result_temp);
            free_temp(result_temp);
            if (currentRelationalOperator == ">") gera_cod("BGT", inicio_label);
            else if (currentRelationalOperator == "<") gera_cod("BLT", inicio_label);
            else if (currentRelationalOperator == "==") gera_cod("BEQ", inicio_label);
            else if (currentRelationalOperator == "!=") gera_cod("BNE", inicio_label);
            else if (currentRelationalOperator == ">=") gera_cod("BGE", inicio_label);
            else if (currentRelationalOperator == "<=") gera_cod("BLE", inicio_label);
            else gera_cod("BNE", inicio_label);
            currentRelationalOperator.clear();
            break;
        }
        case 44: { // FOR: Início da condição
            std::string label = new_label();
            text_section.push_back(label + ":");
            labels_stack.push(label);
            break;
        }
        case 45: { // FOR: Após a condição
            if (operand_stack.empty()) throw SemanticError("Condição inválida.", -1);
            std::string result_temp = operand_stack.top(); operand_stack.pop();
            gera_cod("LD", result_temp);
            free_temp(result_temp);
            std::string fim_label = new_label();
            labels_stack.push(fim_label);
            if (currentRelationalOperator == ">") gera_cod("BLE", fim_label);
            else if (currentRelationalOperator == "<") gera_cod("BGE", fim_label);
            else if (currentRelationalOperator == "==") gera_cod("BNE", fim_label);
            else { gera_cod("BEQ", fim_label); }
            currentRelationalOperator.clear();
            break;
        }
        case 46: { is_in_for_post_op_capture = true; for_post_op_code_buffer.clear(); break; }
        case 47: { is_in_for_post_op_capture = false; break; }
        case 48: { // FOR: Fim de tudo
            for(const auto& line : for_body_code_buffer) text_section.push_back(line);
            for(const auto& line : for_post_op_code_buffer) text_section.push_back(line);
            std::string fim_label = labels_stack.top(); labels_stack.pop();
            std::string cond_label = labels_stack.top(); labels_stack.pop();
            gera_cod("JMP", cond_label);
            text_section.push_back(fim_label + ":");
            break;
        }
        case 49: { this->active_id_name = token->getLexeme(); break; }
        case 50: { // i++
            gera_cod("LD", active_id_name);
            gera_cod("ADDI", "1");
            gera_cod("STO", active_id_name);
            active_id_name.clear();
            break;
        }
        case 51: { // i--
            gera_cod("LD", active_id_name);
            gera_cod("SUBI", "1");
            gera_cod("STO", active_id_name);
            active_id_name.clear();
            break;
        }
        case 52: { is_in_for_body_capture = true; for_body_code_buffer.clear(); break; }
        case 53: { is_in_for_body_capture = false; break; }

        // --- OUTRAS AÇÕES E DEFAULT ---
        case 20: { gera_cod("HLT", "0"); break; }
        default:
             std::cerr << "AVISO - Ação semântica #" << action << " não implementada ou obsoleta." << std::endl;
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
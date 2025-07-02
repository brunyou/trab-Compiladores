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
std::string Semantico::new_label() { return "L" + std::to_string(label_counter++); }
std::string Semantico::new_temp() {
    std::string temp_name = "T" + std::to_string(temp_address_start++);
    gera_data(temp_name, "0", 1);
    return temp_name;
}
void Semantico::free_temp(const std::string& temp) { }

void Semantico::gera_cod(const std::string& instruction, const std::string& arg1, const std::string& arg2) {
    std::string line = "    " + instruction;
    if (!arg1.empty()) line += "\t" + arg1;
    if (!arg2.empty()) line += "\t" + arg2;

    if (this->is_in_for_post_op_capture) { for_post_op_code_buffer.push_back(line); } 
    else if (this->is_in_for_body_capture) { for_body_code_buffer.push_back(line); } 
    else { text_section.push_back(line); }
}

void Semantico::gera_data(const std::string& label, const std::string& value, int count) {
    for(const auto& line : data_section) { if(line.rfind(label + ":", 0) == 0) return; }
    data_section.push_back(label + ":\t" + value);
}

std::string Semantico::get_generated_code() {
    std::stringstream ss;
    if (!data_section.empty()) {
        ss << ".data\n";
        for (const auto& line : data_section) { ss << "    " << line << "\n"; }
        ss << "\n";
    }
    ss << ".text\n_PRINCIPAL:\n";
    for (const auto& line : text_section) { ss << line << "\n"; }
    return ss.str();
}

// --- EXECUÇÃO DAS AÇÕES SEMÂNTICAS ---
void Semantico::executeAction(int action, const Token *token) {
    switch (action) {
        case 1: { // Finaliza declarações
            std::string type = token->getLexeme();
            for (const auto& item : pending_declarations_list) {
                if (symbolTable.existeNoEscopoAtual(item.name)) throw SemanticError("ID '" + item.name + "' já declarado.", item.position);
                Symbol s(item.name, type, symbolTable.getEscopoAtual(), item.position);
                s.vetor = item.is_vector;
                symbolTable.inserir(s);
                gera_data(item.name, "0", item.vector_size);
            }
            pending_declarations_list.clear();
            break;
        }
        case 2: { current_dec_id_name = token->getLexeme(); current_dec_id_pos = token->getPosition(); break; }
        case 3: { assignment_target_id_name = token->getLexeme(); active_id_name = token->getLexeme(); active_id_pos = token->getPosition(); auto s = symbolTable.buscar(assignment_target_id_name); if(!s) throw SemanticError("ID '"+assignment_target_id_name+"' não declarado.", active_id_pos); currentLHS_idType = s->tipo; break; }
        case 4: { currentRHS_expressionType = "int"; if (currentLHS_idType != currentRHS_expressionType) throw SemanticError("Tipos incompatíveis.", active_id_pos); break; }
        case 5: {
            if (operand_stack.empty()) throw SemanticError("Expressão RHS inválida.",-1);
            std::string temp = operand_stack.top(); operand_stack.pop();
            gera_cod("LD", temp);
            if (assignment_target_is_vector_element) { gera_cod("STOV", assignment_target_id_name); } else { gera_cod("STO", assignment_target_id_name); }
            free_temp(temp);
            symbolTable.marcarInicializado(assignment_target_id_name);
            break;
        }
        case 6: { symbolTable.entrarEscopoBloco(); break; }
        case 7: { symbolTable.sairEscopo(); break; }
        case 8: {
            currentProcessingFunctionName = token->getLexeme();
            if (symbolTable.existeNoEscopoAtual(currentProcessingFunctionName)) throw SemanticError("ID '" + currentProcessingFunctionName + "' já declarado.", token->getPosition());
            Symbol s(currentProcessingFunctionName, pendingFunctionReturnType, symbolTable.getEscopoAtual(), token->getPosition());
            s.funcao = true;
            symbolTable.inserir(s);
            symbolTable.entrarEscopoFuncao(currentProcessingFunctionName);
            currentFunctionParamsInfoList.clear();
            text_section.push_back("_" + currentProcessingFunctionName + ":");
            break;
        }
        case 9: { Symbol* s = symbolTable.buscarParaModificacao(currentProcessingFunctionName); if(s) s->assinaturaParametros = currentFunctionParamsInfoList; break; }
        case 10: { symbolTable.sairEscopo(); gera_cod("RETURN"); break; }
        case 11: { pendingFunctionReturnType = token->getLexeme(); break; }
        case 12: {
            std::string name = token->getLexeme();
            if (symbolTable.existeNoEscopoAtual(name)) throw SemanticError("Parâmetro '" + name + "' já declarado.", token->getPosition());
            Symbol s(name, pendingParamType, symbolTable.getEscopoAtual(), token->getPosition());
            s.parametro = true; s.inicializado = true;
            symbolTable.inserir(s);
            currentFunctionParamsInfoList.push_back({name, pendingParamType, false, token->getPosition()});
            gera_data(name, "0");
            break;
        }
        case 13: { pendingParamType = token->getLexeme(); break; }
        case 14: { if (!currentFunctionParamsInfoList.empty()) { currentFunctionParamsInfoList.back().isArray = true; Symbol* s = symbolTable.buscarParaModificacao(currentFunctionParamsInfoList.back().nome); if (s) s->vetor = true; } break; }
        case 15: { break; }
        case 16: { gera_cod("LD", "$in_port"); if (active_id_is_vector_access) { gera_cod("STOV", active_id_name); } else { gera_cod("STO", active_id_name); } symbolTable.marcarInicializado(active_id_name); break; }
        case 17: { if (operand_stack.empty()) throw SemanticError("Expressão para 'escreva' inválida.",-1); std::string temp = operand_stack.top(); operand_stack.pop(); gera_cod("LD", temp); gera_cod("STO", "$out_port"); free_temp(temp); break; }
        case 18: { active_id_name = token->getLexeme(); active_id_pos = token->getPosition(); if(!symbolTable.buscar(active_id_name)) throw SemanticError("ID '"+active_id_name+"' não declarado.", active_id_pos); symbolTable.marcarUsado(active_id_name); break; }
        case 19: { std::string temp = new_temp(); gera_cod("LDI", token->getLexeme()); gera_cod("STO", temp); operand_stack.push(temp); break; }
        case 20: { gera_cod("HLT", "0"); break; }
        case 21: case 31: { if(operand_stack.size()<2) throw SemanticError("Operandos insuficientes.",-1); std::string op2=operand_stack.top(); operand_stack.pop(); std::string op1=operand_stack.top(); operand_stack.pop(); gera_cod("LD",op1); gera_cod("ADD",op2); std::string temp=new_temp(); gera_cod("STO",temp); operand_stack.push(temp); free_temp(op1); free_temp(op2); break; }
        case 22: case 32: { if(operand_stack.size()<2) throw SemanticError("Operandos insuficientes.",-1); std::string op2=operand_stack.top(); operand_stack.pop(); std::string op1=operand_stack.top(); operand_stack.pop(); gera_cod("LD",op1); gera_cod("SUB",op2); std::string temp=new_temp(); gera_cod("STO",temp); operand_stack.push(temp); free_temp(op1); free_temp(op2); break; }
        case 23: { gera_cod("STO", "$indr"); break; }
        case 24: { active_id_is_vector_access = true; if(active_id_context == ActiveIdContext::LHS_ASSIGNMENT) { assignment_target_is_vector_element = true; } break; }
        case 25: { active_id_is_vector_access = false; if(active_id_context == ActiveIdContext::LHS_ASSIGNMENT) { assignment_target_is_vector_element = false; } break; }
        case 26: { active_id_name = token->getLexeme(); break; }
        case 27: { std::string temp = new_temp(); if (active_id_is_vector_access) { gera_cod("LDV", active_id_name); } else { gera_cod("LD", active_id_name); } gera_cod("STO", temp); operand_stack.push(temp); break; }
        case 28: { int s=std::stoi(token->getLexeme()); if(s<=0) throw SemanticError("Tamanho do vetor inválido.",token->getPosition()); pending_declarations_list.push_back({current_dec_id_name,current_dec_id_pos,true,s}); current_dec_id_name.clear(); break; }
        case 29: { pending_declarations_list.push_back({current_dec_id_name,current_dec_id_pos,false,1}); current_dec_id_name.clear(); break; }
        case 30: case 38: { break; }
        case 33: { currentRelationalOperator = token->getLexeme(); break; }
        case 34: { if(operand_stack.size()<2) throw SemanticError("Operandos insuficientes.",-1); std::string op2=operand_stack.top(); operand_stack.pop(); std::string op1=operand_stack.top(); operand_stack.pop(); gera_cod("LD",op1); gera_cod("SUB",op2); std::string temp=new_temp(); gera_cod("STO",temp); operand_stack.push(temp); free_temp(op1); free_temp(op2); break; }
        case 35: case 40: case 45: {
            if (operand_stack.empty()) throw SemanticError("Condição inválida.", -1);
            std::string result_temp = operand_stack.top(); operand_stack.pop();
            gera_cod("LD", result_temp); free_temp(result_temp);
            std::string label = new_label(); labels_stack.push(label);
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
        case 36: { std::string fim_if=new_label(); std::string inicio_else=labels_stack.top(); labels_stack.pop(); gera_cod("JMP", fim_if); text_section.push_back(inicio_else + ":"); labels_stack.push(fim_if); break; }
        case 37: { std::string label=labels_stack.top(); labels_stack.pop(); text_section.push_back(label + ":"); break; }
        case 39: case 42: { std::string label=new_label(); text_section.push_back(label + ":"); labels_stack.push(label); break; }
        case 41: { std::string f=labels_stack.top();labels_stack.pop(); std::string i=labels_stack.top();labels_stack.pop(); gera_cod("JMP",i); text_section.push_back(f+":"); break; }
        case 43: { if(operand_stack.empty()||labels_stack.empty())throw SemanticError("Estrutura do-while inválida.",-1); std::string r=operand_stack.top();operand_stack.pop(); std::string i=labels_stack.top();labels_stack.pop(); gera_cod("LD",r); free_temp(r); if(currentRelationalOperator==">")gera_cod("BGT",i); else if(currentRelationalOperator=="<")gera_cod("BLT",i); else if(currentRelationalOperator=="==")gera_cod("BEQ",i); else if(currentRelationalOperator=="!=")gera_cod("BNE",i); else if(currentRelationalOperator==">=")gera_cod("BGE",i); else if(currentRelationalOperator=="<=")gera_cod("BLE",i); else gera_cod("BNE",i); currentRelationalOperator.clear(); break; }
        case 44: { std::string l=new_label(); text_section.push_back(l+":"); labels_stack.push(l); break; }
        case 46: { is_in_for_post_op_capture = true; for_post_op_code_buffer.clear(); break; }
        case 47: { is_in_for_post_op_capture = false; break; }
        case 48: { for(const auto& l:for_body_code_buffer)text_section.push_back(l); for(const auto& l:for_post_op_code_buffer)text_section.push_back(l); std::string f=labels_stack.top();labels_stack.pop(); std::string c=labels_stack.top();labels_stack.pop(); gera_cod("JMP",c); text_section.push_back(f+":"); break; }
        case 49: { active_id_name = token->getLexeme(); break; }
        case 50: { gera_cod("LD", active_id_name); gera_cod("ADDI", "1"); gera_cod("STO", active_id_name); break; }
        case 51: { gera_cod("LD", active_id_name); gera_cod("SUBI", "1"); gera_cod("STO", active_id_name); break; }
        case 52: { is_in_for_body_capture = true; for_body_code_buffer.clear(); break; }
        case 53: { is_in_for_body_capture = false; break; }
        case 54: {
            active_function_call_name = token->getLexeme();
            auto s = symbolTable.buscar(active_function_call_name);
            if (!s) throw SemanticError("Rotina '" + active_function_call_name + "' não existe.", token->getPosition());
            if (!s->funcao) throw SemanticError("'" + active_function_call_name + "' não é uma função.", token->getPosition());
            arg_count = 0; arg_types.clear(); arg_temps.clear();
            break;
        }
        case 55: {
            auto s = symbolTable.buscar(active_function_call_name);
            if (arg_count != s->assinaturaParametros.size()) {
                throw SemanticError("Função '" + active_function_call_name + "' esperava " + std::to_string(s->assinaturaParametros.size()) + " params, mas " + std::to_string(arg_count) + " foram passados.", -1);
            }
            for (int i = 0; i < arg_count; ++i) {
                if (arg_types[i] != s->assinaturaParametros[i].tipo) throw SemanticError("Tipo incompatível para o param " + std::to_string(i+1) + " na chamada de '" + active_function_call_name + "'.", -1);
                gera_cod("LD", arg_temps[i]);
                gera_cod("STO", s->assinaturaParametros[i].nome);
                free_temp(arg_temps[i]);
            }
            break;
        }
        case 56: {
            auto s = symbolTable.buscar(active_function_call_name);
            gera_cod("CALL", "_" + active_function_call_name);
            if (s->tipo != "void") {
                std::string temp = new_temp();
                gera_cod("STO", temp);
                operand_stack.push(temp);
            }
            break;
        }
        case 57: {
            if (operand_stack.empty()) throw SemanticError("Argumento de função inválido.",-1);
            arg_temps.push_back(operand_stack.top());
            operand_stack.pop();
            arg_types.push_back("int");
            arg_count++;
            break;
        }
        case 58: {
            auto s = symbolTable.buscar(currentProcessingFunctionName);
            if (!s) throw SemanticError("'return' fora de uma função.", -1);
            if (operand_stack.empty()) { if (s->tipo != "void") throw SemanticError("Função '" + currentProcessingFunctionName + "' deve retornar um valor.", -1); } 
            else {
                if (s->tipo == "void") throw SemanticError("Procedimento 'void' não pode retornar um valor.", -1);
                std::string temp = operand_stack.top(); operand_stack.pop();
                gera_cod("LD", temp);
                free_temp(temp);
            }
            gera_cod("RETURN");
            break;
        }
        default:
            std::cerr << "AVISO - Ação semântica #" << action << " não implementada ou desconhecida." << std::endl;
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
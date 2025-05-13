#include <iostream>
#include <fstream>
#include <string>
#include <locale>
#include "Lexico.h"
#include "Sintatico.h"
#include "Constants.h" // Importante
#include "Semantico.h"
#include "SymbolTable.h"
#include "LexicalError.h"   // Seus arquivos de erro
#include "SyntacticError.h" // Seus arquivos de erro
#include "SemanticError.h"  // Seus arquivos de erro
#include "AnalysisError.h"  // Seus arquivos de erro

int main() {
    std::setlocale(LC_ALL, "pt_BR.UTF-8");

    std::ifstream file("cpp_project/input.txt");
    if (!file.is_open()) {
        std::cerr << "ERRO_COMPILADOR: Erro ao abrir o arquivo de entrada 'cpp_project/input.txt'." << std::endl;
        return 1;
    }

    std::string input_code((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    file.close();

    if (input_code.empty()) {
        std::cerr << "AVISO_COMPILADOR: Arquivo de entrada vazio." << std::endl;
    }

    Lexico lex;
    SymbolTable symTable; 
    Semantico sem(symTable); 
    Sintatico sint;

    lex.setInput(input_code.c_str());

    try {
        sint.parse(&lex, &sem);
        std::cout << "Compilado com sucesso!\n";
        std::cout << "---INICIO_TABELA_SIMBOLOS---\n";
        std::cout << sem.formatarTabelaSimbolos();
        std::cout << "---FIM_TABELA_SIMBOLOS---\n";

        // TODO: Lógica para "declarado e não usado" no escopo global pode vir aqui,
        //       após o parse e antes de terminar, iterando sobre symTable.getTabela()
        //       e filtrando pelo escopo "global".
        //       Ex:
        //       for (const auto& sym : symTable.getTabela()) {
        //           if (sym.escopo == "global" && !sym.usado && !sym.funcao /* etc. */) {
        //               std::cerr << "AVISO_FINAL: Identificador global '" << sym.id << "' declarado mas não usado." << std::endl;
        //           }
        //       }

    } catch (const LexicalError &err) {
        std::cerr << "ERRO_LEXICO: " << err.getMessage()
                  << " (na posição " << err.getPosition() << ")" << std::endl;
        return 1;
    } catch (const SyntacticError &err) { 
        std::cerr << "ERRO_SINTATICO: " << err.getMessage()
                    << " (na posição " << err.getPosition() << ")" << std::endl;
        return 1;
    } catch (const SemanticError &err) {
        std::cerr << "ERRO_SEMANTICO: " << err.getMessage()
                   << " (próximo à posição " << err.getPosition() << ")" << std::endl;
        return 1;
    } catch (const AnalysisError &err) { 
        std::cerr << "ERRO_ANALISE: " << err.getMessage()
                   << " (na posição " << err.getPosition() << ")" << std::endl;
        return 1;
    } catch (const std::exception &e) { 
        std::cerr << "ERRO_GERAL_COMPILADOR_STD: " << e.what() << std::endl;
        return 1;
    } catch (...) { 
        std::cerr << "ERRO_COMPILADOR_DESCONHECIDO: Uma exceção não tratada ocorreu." << std::endl;
        return 1;
    }

    return 0;
}
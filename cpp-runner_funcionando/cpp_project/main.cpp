#include <iostream>
#include <fstream>
#include <string>
#include <locale>
#include "Lexico.h"
#include "Sintatico.h"
#include "Constants.h"
#include "Semantico.h"
#include "SymbolTable.h"
#include "LexicalError.h"
#include "SyntacticError.h"
#include "SemanticError.h"
#include "AnalysisError.h"

int main() {
    std::setlocale(LC_ALL, "pt_BR.UTF-8");

    // --- Nome do Arquivo de Entrada ---
    std::string input_filename = "cpp_project/input.txt";
    // --- Nome do Arquivo de Saída ---
    std::string output_filename = "output.bip";

    std::ifstream file(input_filename);
    if (!file.is_open()) {
        std::cerr << "ERRO_COMPILADOR: Erro ao abrir o arquivo de entrada '" << input_filename << "'." << std::endl;
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
        std::cout << "DEBUG - CHAMANDO sint.parse...\n"; // <--- DEBUG ADICIONADO
        sint.parse(&lex, &sem);
        std::cout << "DEBUG - sint.parse CONCLUÍDO COM SUCESSO!\n"; // <--- DEBUG ADICIONADO

        std::cout << "Compilado com sucesso!\n";

        // --- Saída da Tabela de Símbolos ---
        std::cout << "\n---INICIO_TABELA_SIMBOLOS---\n";
        std::cout << sem.formatarTabelaSimbolos();
        std::cout << "---FIM_TABELA_SIMBOLOS---\n";

        // --- Saída dos Avisos de Compilação ---
        std::vector<std::string> warnings = sem.getCompilationWarnings();
        if (!warnings.empty()) {
            std::cout << "\n---INICIO_AVISOS_COMPILACAO---\n";
            for (const auto& w : warnings) {
                std::cout << w << std::endl;
            }
            std::cout << "---FIM_AVISOS_COMPILACAO---\n";
        }


        // --- Saída do Código Gerado ---
        std::cout << "DEBUG - CHAMANDO sem.get_generated_code...\n"; // <--- DEBUG ADICIONADO
        std::string generated_code = sem.get_generated_code();
        std::cout << "\n---INICIO_CODIGO_GERADO---\n";
        std::cout << generated_code;
        std::cout << "---FIM_CODIGO_GERADO---\n";

        // --- Salvar Código Gerado em Arquivo ---
        std::ofstream outfile(output_filename);
        if (outfile.is_open()) {
            outfile << generated_code;
            outfile.close();
            std::cout << "\nCódigo gerado salvo em '" << output_filename << "'.\n";
        } else {
            std::cerr << "ERRO_COMPILADOR: Não foi possível abrir '" << output_filename << "' para escrita.\n";
        }

    } catch (const LexicalError &err) {
        std::cerr << "DEBUG - CAIU NO CATCH LEXICO!\n"; // <--- DEBUG ADICIONADO
        std::cerr << "ERRO_LEXICO: " << err.getMessage()
                  << " (na posição " << err.getPosition() << ")" << std::endl;
        return 1;
    } catch (const SyntacticError &err) {
        std::cerr << "DEBUG - CAIU NO CATCH SINTATICO!\n"; // <--- DEBUG ADICIONADO
        std::cerr << "ERRO_SINTATICO: " << err.getMessage()
                    << " (na posição " << err.getPosition() << ")" << std::endl;
        return 1;
    } catch (const SemanticError &err) {
        std::cerr << "DEBUG - CAIU NO CATCH SEMANTICO!\n"; // <--- DEBUG ADICIONADO
        std::cerr << "ERRO_SEMANTICO: " << err.getMessage()
                   << " (próximo à posição " << err.getPosition() << ")" << std::endl;
        return 1;
    } catch (const AnalysisError &err) {
        std::cerr << "DEBUG - CAIU NO CATCH ANALISE!\n"; // <--- DEBUG ADICIONADO
        std::cerr << "ERRO_ANALISE: " << err.getMessage()
                   << " (na posição " << err.getPosition() << ")" << std::endl;
        return 1;
    } catch (const std::exception &e) {
        std::cerr << "DEBUG - CAIU NO CATCH STD::EXCEPTION!\n"; // <--- DEBUG ADICIONADO
        std::cerr << "ERRO_GERAL_COMPILADOR_STD: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "DEBUG - CAIU NO CATCH GENÉRICO!\n"; // <--- DEBUG ADICIONADO
        std::cerr << "ERRO_COMPILADOR_DESCONHECIDO: Uma exceção não tratada ocorreu." << std::endl;
        return 1;
    }

    return 0;
}
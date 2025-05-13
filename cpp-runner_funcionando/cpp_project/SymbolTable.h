#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "Symbol.h" 
#include <vector>
#include <string>
#include <optional>
#include <stack>
#include <algorithm> 
#include <functional> 
#include <iostream> 

class SymbolTable {
private:
    std::vector<Symbol> tabela;
    std::stack<std::string> pilhaEscopos;
    int proximoIdEscopoUnico; 

    std::string gerarNomeEscopoUnicoAninhado(const std::string& prefixoBase) {
        return getEscopoAtual() + "/" + prefixoBase + std::to_string(proximoIdEscopoUnico++);
    }

public:
    SymbolTable() : proximoIdEscopoUnico(0) {
        pilhaEscopos.push("global");
    }

    std::string getEscopoAtual() const {
        if (pilhaEscopos.empty()) { return "_ERRO_ESCOPO_VAZIO_"; }
        return pilhaEscopos.top();
    }

    void entrarEscopoBloco() {
        std::string novoEscopo = gerarNomeEscopoUnicoAninhado("b"); 
        pilhaEscopos.push(novoEscopo);
        proximoIdEscopoUnico = 0; 
        std::cerr << "DEBUG_SymbolTable: Entrou no escopo de BLOCO '" << novoEscopo << "'" << std::endl;
    }
    
    void entrarEscopoFuncao(const std::string& nomeFuncao) {
        std::string escopoPai = getEscopoAtual(); 
        std::string novoEscopoInterno = escopoPai + "/func_" + nomeFuncao;
        pilhaEscopos.push(novoEscopoInterno);
        proximoIdEscopoUnico = 0; 
        std::cerr << "DEBUG_SymbolTable: Entrou no escopo INTERNO da FUNÇÃO '" << novoEscopoInterno << "' (Função '" << nomeFuncao << "' declarada em '" << escopoPai << "')" << std::endl;
    }
    
    void sairEscopo() {
        if (pilhaEscopos.empty() || pilhaEscopos.top() == "global") {
            std::cerr << "WARNING_SymbolTable: Tentativa de sair de escopo inválida." << std::endl;
            return; 
        }
        std::string escopoSaindo = pilhaEscopos.top();
        pilhaEscopos.pop();
        std::cerr << "DEBUG_SymbolTable: Saiu do escopo '" << escopoSaindo 
                  << "'. Escopo atual: '" << getEscopoAtual() << "'" << std::endl;
    }

    void inserir(const Symbol& simbolo) { 
        tabela.push_back(simbolo);
        std::cerr << "DEBUG_SymbolTable: Inserido ID:'" << simbolo.id << "' Tipo:'" << simbolo.tipo 
                  << "' Modalidade:(func=" << simbolo.funcao << ",param=" << simbolo.parametro 
                  << ",vet=" << simbolo.vetor << ") Escopo:'" << simbolo.escopo 
                  << "' LinhaDec:" << simbolo.linhaDeclaracao << std::endl;
    }

    std::optional<Symbol> buscar(const std::string& id) const {
        std::stack<std::string> escoposParaBusca = pilhaEscopos;
        while (!escoposParaBusca.empty()) {
            std::string escopoAtualDaBusca = escoposParaBusca.top();
            for (auto it = tabela.rbegin(); it != tabela.rend(); ++it) {
                if (it->id == id && it->escopo == escopoAtualDaBusca) {
                    std::cerr << "DEBUG_SymbolTable: Encontrado '" << id << "' no escopo '" << escopoAtualDaBusca << "'" << std::endl;
                    return *it;
                }
            }
            escoposParaBusca.pop();
        }
        std::cerr << "DEBUG_SymbolTable: Símbolo '" << id << "' não encontrado em nenhum escopo ativo." << std::endl;
        return std::nullopt;
    }
    
    Symbol* buscarParaModificacaoNoEscopo(const std::string& id, const std::string& escopoAlvo) {
        for (auto& s : tabela) { 
            if (s.id == id && s.escopo == escopoAlvo) {
                 std::cerr << "DEBUG_SymbolTable: Encontrado '" << id << "' no escopo '" << escopoAlvo << "' para modificação." << std::endl;
                return &s;
            }
        }
        std::cerr << "DEBUG_SymbolTable: Símbolo '" << id << "' NÃO encontrado no escopo '" << escopoAlvo << "' para modificação." << std::endl;
        return nullptr;
    }

    bool existeNoEscopoAtual(const std::string& id) const {
        std::string escopoAtual = getEscopoAtual();
        for (const auto& s : tabela) {
            if (s.id == id && s.escopo == escopoAtual) {
                return true;
            }
        }
        return false;
    }
    
    bool atualizarAtributoSimbViaBuscaHierarquica(const std::string& id, std::function<void(Symbol&)> updateFn) {
        std::stack<std::string> escoposParaBusca = pilhaEscopos;
        while (!escoposParaBusca.empty()) {
            std::string escopoAtualDaBusca = escoposParaBusca.top();
            for (int i = tabela.size() - 1; i >= 0; --i) { 
                if (tabela[i].id == id && tabela[i].escopo == escopoAtualDaBusca) {
                    updateFn(tabela[i]); 
                    return true;
                }
            }
            escoposParaBusca.pop();
        }
        return false; 
    }

    void marcarUsado(const std::string& id) {
        if (atualizarAtributoSimbViaBuscaHierarquica(id, [](Symbol& s){ s.usado = true; })) {
             std::cerr << "DEBUG_SymbolTable: Símbolo '" << id << "' marcado como usado." << std::endl;
        }
    }

    void marcarInicializado(const std::string& id) {
        if (atualizarAtributoSimbViaBuscaHierarquica(id, [](Symbol& s){ s.inicializado = true; })) {
             std::cerr << "DEBUG_SymbolTable: Símbolo '" << id << "' marcado como inicializado." << std::endl;
        }
    }
    
    std::vector<Symbol> getSimbolosDoEscopo(const std::string& nomeEscopo) const {
        std::vector<Symbol> simbolosDoEscopo;
        for (const auto& s : tabela) {
            if (s.escopo == nomeEscopo) {
                simbolosDoEscopo.push_back(s);
            }
        }
        return simbolosDoEscopo;
    }

    std::vector<Symbol> getTabela() const {
        return tabela;
    }
};

#endif
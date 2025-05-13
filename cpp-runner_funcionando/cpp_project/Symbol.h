#ifndef SYMBOL_H
#define SYMBOL_H

#include <string>
#include <vector>

struct ParameterInfo {
    std::string nome; 
    std::string tipo;
    bool isArray = false; 
    int linhaDeclaracaoParam = -1; 
};

struct Symbol {
    std::string id;
    std::string tipo; 
    bool inicializado = false;
    bool usado = false;
    std::string escopo = "global";
    int linhaDeclaracao = -1; 

    bool parametro = false;      
    int posicaoParametro = -1;  

    bool vetor = false;          
    bool matriz = false;         
    bool referencia = false;     

    bool funcao = false;         
    std::vector<ParameterInfo> assinaturaParametros; 

    // Construtor com todos os parâmetros default serve como construtor padrão.
    // Symbol() = default; // Removido para evitar ambiguidade
    Symbol(const std::string& id_val = "", 
           const std::string& tipo_val = "", 
           const std::string& escopo_val = "global",
           int linha_dec_val = -1)
        : id(id_val), tipo(tipo_val), escopo(escopo_val), 
          inicializado(false), usado(false), 
          parametro(false), posicaoParametro(-1), 
          vetor(false), matriz(false), referencia(false), 
          funcao(false), linhaDeclaracao(linha_dec_val) {}
};

#endif
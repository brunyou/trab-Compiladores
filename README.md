# Compilador com Interface Web

Projeto desenvolvido para a disciplina de **Compiladores**. Consiste em um **compilador escrito em C++** acessível por meio de uma **interface web (HTML/CSS/JS)** servida por um **servidor Node.js enxuto**. O usuário escreve código no navegador, envia para o servidor, que por sua vez invoca o binário do compilador e retorna diagnósticos (erros/avisos) e resultados (por exemplo: código intermediário, saída da execução simulada, ou binário gerado — dependendo da fase implementada).


---

## 📌 Objetivo

Fornecer um ambiente simples e didático para experimentação com a linguagem de programação usada na disciplina. O projeto busca tornar tangíveis as etapas do pipeline de compilação (análise léxica, sintática, semântica, geração/interpretação) e viabilizar ciclos rápidos de teste diretamente no navegador.

---

## 🛠 Tecnologias Utilizadas

* **C++** – Núcleo do compilador: analisador léxico, parser, checagens semânticas e geração de código / interpretação.
* **Node.js** – Servidor minimalista que recebe o código, chama o compilador e devolve o resultado em JSON.
* **HTML + CSS + JavaScript** – Interface web leve para edição, envio e exibição dos resultados de compilação.
* **WEB GALS** - Gerar os códigos base do compilador à partir da linguagem BNF criada.
---

## 🧩 Arquitetura (Visão Geral)

**Passo a passo resumido:**

1. Usuário escreve código e clica em *Compilar*.
2. Frontend envia o código (e flags de compilação opcionais) para o endpoint `/compile`.
3. Node grava o código em arquivo temporário e executa o compilador C++.
4. A saída padrão e o código de retorno são capturados.
5. Resposta estruturada é enviada ao navegador e exibida ao usuário.

---

## 🚀 Instalação & Execução

### 1️⃣ Compilar o Compilador (C++)

Entre no diretório `compiler/` e execute (exemplo usando *g++*):

```bash
g++ -std=c++17 -O2 src/main.cpp src/lexer.cpp src/parser.cpp src/semantic.cpp src/codegen.cpp -Iinclude -o compiler
```

> Ajuste a lista de arquivos conforme sua estrutura real ou use `make` / `cmake`.

### 2️⃣ Configurar o Servidor Node.js

No diretório `server/`:

```bash
npm install
```

Crie (ou copie de `config.example.json`) um arquivo `config.json` apontando para o caminho do binário do compilador:

```json
{
  "compilerPath": "../compiler/compiler"
}
```

> Caminhos relativos partem do diretório do servidor. Ajuste conforme necessário.

Inicie o servidor:

```bash
node index.js
```

> Opcional: `npm start` se definido no *package.json*.

### 3️⃣ Acessar a Interface Web

Abra no navegador:

```
http://localhost:3000
```

Se estiver servindo conteúdo estático de outro host/porta, ajuste conforme sua config.

---

## ✅ Status das Funcionalidades

| Fase                 | Status             | Notas                                        |
| -------------------- | ------------------ | -------------------------------------------- |
| Analisador Léxico    | ✅ Concluído        | Tokens básicos implementados.                |
| Analisador Sintático | ✅ Concluído        | Baseada em gramática LL(1). TODO: confirmar. |
| Semântica            | ⚠️ Parcial         | Tipagem básica; escopos aninhados pendentes. |
| Geração de Código    | ❌ Não implementado | Somente árvore intermediária.                |
| Interface Web        | ✅ Básica           | Edição + botão Compilar.                     |
| Integração Node      | ✅                  | Invoca binário e retorna JSON.               |



---

## 👨‍💻 Equipe

* **Bruno Pereira Freitas** – Cd. Aluno: 7419414 – Desenvolvimento.


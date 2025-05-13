const express = require("express");
const { exec } = require("child_process");
const fs = require("fs");
const path = require("path");
const cors = require("cors");

const app = express();
app.use(cors());
app.use(express.json());

// Função para extrair a tabela de símbolos da saída do C++ usando delimitadores
function extrairTabelaSimbolos(saidaCompleta) {
    const inicioDelim = "---INICIO_TABELA_SIMBOLOS---";
    const fimDelim = "---FIM_TABELA_SIMBOLOS---";

    const inicioIdx = saidaCompleta.indexOf(inicioDelim);
    const fimIdx = saidaCompleta.indexOf(fimDelim);

    if (inicioIdx === -1 || fimIdx === -1 || fimIdx <= inicioIdx) {
        console.warn("Delimitadores da tabela de símbolos não encontrados ou em ordem incorreta na saída.");
        return []; // Retorna array vazio se não encontrar os delimitadores
    }

    // Extrai apenas a string da tabela entre os delimitadores
    const dadosTabelaCSV = saidaCompleta.substring(inicioIdx + inicioDelim.length, fimIdx).trim();
    
    if (!dadosTabelaCSV) { // Se a string entre delimitadores for vazia
        console.warn("Dados da tabela de símbolos (CSV) estão vazios entre os delimitadores.");
        return [];
    }
    
    const linhas = dadosTabelaCSV.split('\n');
    // A primeira linha deve ser o cabeçalho. Se só tiver ela e estiver vazia, ou não tiver linhas, retorna vazio.
    if (linhas.length === 0 || (linhas.length === 1 && linhas[0].trim() === "")) {
        console.warn("Nenhuma linha de dados ou cabeçalho válido na tabela CSV.");
        return [];
    }

    const cabecalhoArray = linhas[0].split(',').map(h => h.trim());
    if (cabecalhoArray.length === 0 || cabecalhoArray[0] === "") { // Verifica se o cabeçalho é válido
        console.warn("Cabeçalho da tabela CSV inválido ou vazio.");
        return [];
    }
    
    const tabela = [];

    for (let i = 1; i < linhas.length; i++) { // Começa do 1 para pular o cabeçalho
        const linhaAtual = linhas[i].trim();
        if (linhaAtual === "") continue; // Pular linhas completamente vazias

        const valores = linhaAtual.split(',').map(v => v.trim());
        if (valores.length === cabecalhoArray.length) {
            const linhaObj = {};
            for (let j = 0; j < cabecalhoArray.length; j++) {
                linhaObj[cabecalhoArray[j]] = valores[j];
            }
            tabela.push(linhaObj);
        } else {
            console.warn(`Linha da tabela CSV com número incorreto de colunas: '${linhaAtual}'. Esperado: ${cabecalhoArray.length}, Recebido: ${valores.length}`);
        }
    }
    return tabela;
}


// Função para formatar o array de objetos da tabela para HTML
function formatarTabelaParaHTML(tabelaArray) {
    if (!tabelaArray || tabelaArray.length === 0) {
        return "<p>Tabela de símbolos não gerada ou vazia.</p>"; // Mensagem padrão
    }

    let html = "<table class='symbol-table'><thead><tr>"; // Adicione uma classe para estilização
    // Usando o primeiro objeto para obter as chaves do cabeçalho (se houver dados)
    // ou o array de cabeçalho diretamente se a extração o fornecer
    const chavesCabecalho = Object.keys(tabelaArray[0]);
    for (const chave of chavesCabecalho) {
        html += `<th>${chave}</th>`;
    }
    html += "</tr></thead><tbody>";

    for (const linha of tabelaArray) {
        html += "<tr>";
        for (const chave of chavesCabecalho) { // Garante a ordem das colunas
            html += `<td>${linha[chave] !== undefined ? linha[chave] : ''}</td>`; // Trata valores undefined
        }
        html += "</tr>";
    }

    html += "</tbody></table>";
    return html;
}


app.post("/executar", (req, res) => {
    const codigo = req.body.codigo;

    const inputPath = path.join(__dirname, "cpp_project", "input.txt");
    // O caminho para o executável precisa ser correto em relação ao local de server.js
    // Se analisador.exe está em cpp_project e server.js em cpp-runner (um nível acima)
    const execPath = path.join(__dirname, "cpp_project", "analisador.exe"); 
    // Se server.js está na mesma pasta que cpp_project:
    // const execPath = path.join("cpp_project", "analisador.exe");

    fs.writeFileSync(inputPath, codigo, { encoding: 'utf8' });

    // Comando para executar. Se o execPath tiver espaços, pode precisar de aspas.
    const command = `"${execPath}"`; // Adicionar aspas para caminhos com espaços

    exec(command, { timeout: 7000, windowsHide: true }, (error, stdout, stderr) => {
        console.log("--- SAÍDA BRUTA DO C++ (stdout) ---");
        console.log(stdout);
        console.log("--- FIM SAÍDA BRUTA ---");
        console.log("--- SAÍDA DE ERRO DO C++ (stderr) ---");
        console.log(stderr);
        console.log("--- FIM SAÍDA DE ERRO ---");

        if (error) {
            console.error("Erro ao executar o processo C++:", error.message);
            // Tentar enviar stderr se houver, senão a mensagem de erro do processo
            let erroMsg = stderr || error.message;
            if (stderr.includes("ERRO_LEXICO") || stderr.includes("ERRO_SINTATICO") || stderr.includes("ERRO_SEMANTICO") || stderr.includes("ERRO_COMPILADOR") || stderr.includes("ERRO_GERAL_COMPILADOR")) {
                erroMsg = stderr; // Usar a saída de erro do compilador diretamente
            }
            return res.status(400).json({ erro: erroMsg, saida: stdout }); // Envia status 400 para erros
        }
        
        let saidaPrincipal = stdout; // Pode ser só a mensagem "Compilado com sucesso!"
        let tabelaHTMLGerada = "<p>Tabela de símbolos não disponível.</p>"; // Default

        try {
            // Extrair a mensagem principal antes da tabela
            const inicioDelimTabela = stdout.indexOf("---INICIO_TABELA_SIMBOLOS---");
            if (inicioDelimTabela !== -1) {
                saidaPrincipal = stdout.substring(0, inicioDelimTabela).trim();
            } else {
                // Se não há delimitador, mas há "Compilado com sucesso!", usar o que vem depois.
                const compiladoSucessoMsg = "Compilado com sucesso!\n";
                const idxCompilado = stdout.indexOf(compiladoSucessoMsg);
                if (idxCompilado !== -1) {
                    saidaPrincipal = compiladoSucessoMsg.trim(); // Mantém a mensagem de sucesso
                } else {
                    saidaPrincipal = stdout.trim(); // Caso contrário, usa stdout inteiro como saída principal
                }
            }


            const arrayDeObjetosTabela = extrairTabelaSimbolos(stdout); // Passa o stdout inteiro
            console.log("--- TABELA COMO ARRAY DE OBJETOS (extrairTabelaSimbolos) ---");
            console.log(JSON.stringify(arrayDeObjetosTabela, null, 2));
            
            if (arrayDeObjetosTabela.length > 0) {
                tabelaHTMLGerada = formatarTabelaParaHTML(arrayDeObjetosTabela);
            } else if (stdout.includes("---INICIO_TABELA_SIMBOLOS---")) { 
                // Se os delimitadores estavam lá mas a extração resultou em array vazio (ex: só cabeçalho)
                tabelaHTMLGerada = "<p>Tabela de símbolos gerada, mas vazia ou com formato inesperado.</p>";
            }
            // Se não, mantém a mensagem default de "não disponível" ou a de erro da extração

            console.log("--- TABELA COMO HTML (formatarTabelaParaHTML) ---");
            console.log(tabelaHTMLGerada);

        } catch (e) {
            console.error("Erro no servidor ao processar ou formatar a tabela de símbolos:", e.message);
            tabelaHTMLGerada = "<p>Erro no servidor ao processar a tabela de símbolos.</p>";
        }

        // Se stderr tiver avisos do compilador, envie-os também
        let avisos = stderr.trim();

        res.json({ 
            saida: saidaPrincipal, 
            avisos: avisos ? `AVISOS DO COMPILADOR:\n${avisos}` : "", 
            tabela: tabelaHTMLGerada 
        });
    });
});

const PORT = 3000;
app.listen(PORT, () => {
    console.log(`Servidor rodando na porta ${PORT}`);
});
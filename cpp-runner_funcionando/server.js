const express = require("express");
const { exec } = require("child_process");
const fs = require("fs");
const path = require("path");
const cors = require("cors");

const app = express();
app.use(cors());
app.use(express.json());

// Função genérica para extrair uma seção delimitada
function extrairSecaoDelimitada(saidaCompleta, inicioDelim, fimDelim, trimResult = true) {
    const inicioIdx = saidaCompleta.indexOf(inicioDelim);
    const fimIdx = saidaCompleta.indexOf(fimDelim);

    if (inicioIdx === -1 || fimIdx === -1 || fimIdx <= inicioIdx) {
        console.warn(`Delimitadores '${inicioDelim}'/'${fimDelim}' não encontrados ou em ordem incorreta na saída.`);
        return ""; // Retorna string vazia se não encontrar
    }

    let secao = saidaCompleta.substring(inicioIdx + inicioDelim.length, fimIdx);
    return trimResult ? secao.trim() : secao;
}


// Função para extrair a tabela de símbolos da saída do C++ usando delimitadores
function extrairTabelaSimbolos(saidaCompleta) {
    const dadosTabelaCSV = extrairSecaoDelimitada(saidaCompleta, "---INICIO_TABELA_SIMBOLOS---", "---FIM_TABELA_SIMBOLOS---");

    if (!dadosTabelaCSV) {
        console.warn("Dados da tabela de símbolos (CSV) estão vazios entre os delimitadores.");
        return [];
    }

    const linhas = dadosTabelaCSV.split('\n');
    if (linhas.length === 0 || (linhas.length === 1 && linhas[0].trim() === "")) {
        console.warn("Nenhuma linha de dados ou cabeçalho válido na tabela CSV.");
        return [];
    }

    const cabecalhoArray = linhas[0].split(',').map(h => h.trim());
    if (cabecalhoArray.length === 0 || cabecalhoArray[0] === "") {
        console.warn("Cabeçalho da tabela CSV inválido ou vazio.");
        return [];
    }

    const tabela = [];
    for (let i = 1; i < linhas.length; i++) {
        const linhaAtual = linhas[i].trim();
        if (linhaAtual === "") continue;

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
        return "<p>Tabela de símbolos não gerada ou vazia.</p>";
    }

    let html = "<table class='symbol-table'><thead><tr>";
    const chavesCabecalho = Object.keys(tabelaArray[0]);
    for (const chave of chavesCabecalho) {
        html += `<th>${chave}</th>`;
    }
    html += "</tr></thead><tbody>";

    for (const linha of tabelaArray) {
        html += "<tr>";
        for (const chave of chavesCabecalho) {
            html += `<td>${linha[chave] !== undefined ? linha[chave] : ''}</td>`;
        }
        html += "</tr>";
    }

    html += "</tbody></table>";
    return html;
}


app.post("/executar", (req, res) => {
    const codigo = req.body.codigo;

    const inputPath = path.join(__dirname, "cpp_project", "input.txt");
    const execPath = path.join(__dirname, "cpp_project", "analisador.exe");

    fs.writeFileSync(inputPath, codigo, { encoding: 'utf8' });
    const command = `"${execPath}"`;

    exec(command, { timeout: 7000, windowsHide: true }, (error, stdout, stderr) => {
        console.log("--- SAÍDA BRUTA DO C++ (stdout) ---");
        console.log(stdout);
        console.log("--- FIM SAÍDA BRUTA ---");
        console.log("--- SAÍDA DE ERRO DO C++ (stderr) ---");
        console.log(stderr);
        console.log("--- FIM SAÍDA DE ERRO ---");

        let erroMsgParaFrontend = "";
        let saidaPrincipalParaFrontend = stdout; // Default para stdout inteiro
        let avisosParaFrontend = stderr.trim();
        let tabelaHTMLGerada = "<p>Tabela de símbolos não disponível.</p>";
        let codigoBIPExtraido = "";


        if (error) {
            console.error("Erro ao executar o processo C++:", error.message);
            erroMsgParaFrontend = stderr || stdout || error.message; // Prioriza stderr, depois stdout, depois error.message
            // Tenta manter alguma saída principal mesmo em erro, se houver
            saidaPrincipalParaFrontend = stdout.includes("Compilado com sucesso!") ? stdout : erroMsgParaFrontend;
             // Extrai código BIP mesmo em erro, se presente
            codigoBIPExtraido = extrairSecaoDelimitada(stdout, "---INICIO_CODIGO_GERADO---", "---FIM_CODIGO_GERADO---");
            return res.status(400).json({
                erro: erroMsgParaFrontend,
                saida: stdout, // Envia o stdout bruto em caso de erro também
                avisos: avisosParaFrontend ? `AVISOS DO COMPILADOR (ou erro):\n${avisosParaFrontend}` : "",
                tabela: tabelaHTMLGerada, // Pode não ter sido gerada
                codigoBIP: codigoBIPExtraido
            });
        }

        // Processamento para sucesso
        const inicioDelimTabela = stdout.indexOf("---INICIO_TABELA_SIMBOLOS---");
        const inicioDelimCodigo = stdout.indexOf("---INICIO_CODIGO_GERADO---");

        if (inicioDelimTabela !== -1) {
            saidaPrincipalParaFrontend = stdout.substring(0, inicioDelimTabela).trim();
        } else if (inicioDelimCodigo !== -1) {
            saidaPrincipalParaFrontend = stdout.substring(0, inicioDelimCodigo).trim();
        } else {
             // Se não há delimitadores, mas há "Compilado com sucesso!", usar o que vem depois.
            const compiladoSucessoMsg = "Compilado com sucesso!\n";
            const idxCompilado = stdout.indexOf(compiladoSucessoMsg);
            if (idxCompilado !== -1) {
                saidaPrincipalParaFrontend = compiladoSucessoMsg.trim();
            } else {
                saidaPrincipalParaFrontend = stdout.trim();
            }
        }


        try {
            const arrayDeObjetosTabela = extrairTabelaSimbolos(stdout);
            console.log("--- TABELA COMO ARRAY DE OBJETOS (extrairTabelaSimbolos) ---");
            console.log(JSON.stringify(arrayDeObjetosTabela, null, 2));
            if (arrayDeObjetosTabela.length > 0) {
                tabelaHTMLGerada = formatarTabelaParaHTML(arrayDeObjetosTabela);
            } else if (stdout.includes("---INICIO_TABELA_SIMBOLOS---")) {
                tabelaHTMLGerada = "<p>Tabela de símbolos gerada, mas vazia ou com formato inesperado.</p>";
            }
        } catch (e) {
            console.error("Erro no servidor ao processar ou formatar a tabela de símbolos:", e.message);
            tabelaHTMLGerada = "<p>Erro no servidor ao processar a tabela de símbolos.</p>";
        }

        codigoBIPExtraido = extrairSecaoDelimitada(stdout, "---INICIO_CODIGO_GERADO---", "---FIM_CODIGO_GERADO---");
        console.log("--- CÓDIGO BIP EXTRAÍDO ---");
        console.log(codigoBIPExtraido);


        res.json({
            saida: saidaPrincipalParaFrontend,
            avisos: avisosParaFrontend ? `AVISOS DO COMPILADOR:\n${avisosParaFrontend}` : "",
            tabela: tabelaHTMLGerada,
            codigoBIP: codigoBIPExtraido
        });
    });
});

const PORT = 3000;
app.listen(PORT, () => {
    console.log(`Servidor rodando na porta ${PORT}`);
});
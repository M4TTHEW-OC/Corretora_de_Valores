// main.c - Versão 1.4 + compra/venda/integrado + correção comprarAtivoRV
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <locale.h>
#include <time.h>

/* ======= Config ======= */
#define MAX_TRANSACOES 200
#define MAX_ATIVOS 50
#define MAX_NOME 50
#define MAX_CPF 16
#define MAX_SENHA 20

/* ======= Tipos ======= */

/* Registro de transação para extrato */
typedef struct {
    char tipo[32];       // "PIX", "TED", "Transferência", "Compra", "Venda", "Resgate", "Provento"
    char descricao[80];  // descrição curta
    float valor;         // valor (positivo = entrada, negativo = saída)
    float taxa;          // taxa aplicada (se houver)
    float saldoFinalBanco; // saldo do banco após operação (para referência)
    char dataHora[20];   // "dd/mm HH:MM"
} Transacao;

/* Ativo disponível na simulação */
typedef struct {
    char ticker[16];
    char nome[50];
    float preco;                 // preço unitário atual (fixo na simulação)
    float dividend_per_period;   // dividendo por cota por pagamento
    int periods_per_year;        // quantas vezes paga por ano (1,2,4,12)
    bool isFII;                  // FII tem rendimento isento (sim)
} AtivoRV;

/* Entrada de carteira (posse do usuário) */
typedef struct {
    char ticker[16];
    int quantidade;
    float precoMedio; // preço médio de compra
} AtivoCarteira;

/* Conta de investimento: saldo em caixa + carteira + extrato próprio */
typedef struct {
    float saldo;                       // caixa disponível para investir / resgatar
    AtivoCarteira carteira[MAX_ATIVOS];
    int numAtivos;
    Transacao extrato[MAX_TRANSACOES];
    int numTransacoes;
} ContaInvestimento;

/* Conta do banco: saldo + extrato */
typedef struct {
    float saldo;
    Transacao extrato[MAX_TRANSACOES];
    int numTransacoes;
} ContaBanco;

/* Usuário */
typedef struct {
    char nome[MAX_NOME];
    char cpf[MAX_CPF];
    char senha[MAX_SENHA];
    ContaBanco banco;
    ContaInvestimento investimento;
} Usuario;

/* ======= Ativos pré-definidos ======= */
/* Valores ilustrativos — ajuste se quiser */
AtivoRV ativosDisponiveis[] = {
    { "SANEPAR", "Sanepar",        20.00f, 1.50f, 1,  false },
    { "CEMIG",   "Cemig",          10.00f, 0.60f, 2,  false },
    { "BBAS3",   "Banco do Brasil",30.00f, 0.35f, 4,  false },
    { "ITAU",    "Itaú",           25.00f, 0.05f,12,  false },
    { "HGLG11",  "FII HGLG11",     80.00f, 0.60f,12,  true  }
};
const int NUM_ATIVOS = sizeof(ativosDisponiveis) / sizeof(AtivoRV);

/* ======= Protótipos ======= */
/* utilitários */
void clear_input(void);
void read_line(char *buf, int size);
void timestamp_now(char *out, int size);

/* extrato */
void registrarTransacaoBanco(Usuario *u, const char *tipo, const char *desc, float valor, float taxa);
void registrarTransacaoInvest(Usuario *u, const char *tipo, const char *desc, float valor, float taxa);
void exibirExtratoBanco(Usuario *u);
void exibirExtratoInvest(Usuario *u);

/* cadastro/login */
void cadastrarUsuario(Usuario *u);
bool validarLogin(Usuario *u);

/* menus */
void menuPrincipal(Usuario *u);
void menuContaBanco(Usuario *u);
void menuContaInvestimento(Usuario *u);
void submenuAtivos(Usuario *u);

/* banco/transações */
void depositarBanco(Usuario *u);
void transferirParaInvestimento(Usuario *u);
void transferirParaBanco(Usuario *u);
void transferirParaBancoExterno(Usuario *u);

/* renda variável */
void listarAtivosDisponiveis(void);
void comprarAtivoRV(Usuario *u);
void venderAtivoRV(Usuario *u);
void mostrarCarteira(Usuario *u);

/* === NOVO: simulação de proventos RV === */
void simularProventosRV(Usuario *u);

/* ======= Implementações utilitárias ======= */

void clear_input(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

void read_line(char *buf, int size) {
    if (fgets(buf, size, stdin) == NULL) { buf[0] = '\0'; return; }
    size_t len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
}

void timestamp_now(char *out, int size) {
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    if (lt) strftime(out, size, "%d/%m %H:%M", lt);
    else strncpy(out, "00/00 00:00", size);
}

/* ======= Extrato / registro de transações ======= */

/* registra em extrato do banco (usa saldo do banco como saldoFinalBanco) */
void registrarTransacaoBanco(Usuario *u, const char *tipo, const char *desc, float valor, float taxa) {
    if (u->banco.numTransacoes >= MAX_TRANSACOES) return;
    Transacao *t = &u->banco.extrato[u->banco.numTransacoes++];
    strncpy(t->tipo, tipo, sizeof(t->tipo)-1); t->tipo[sizeof(t->tipo)-1] = '\0';
    strncpy(t->descricao, desc, sizeof(t->descricao)-1); t->descricao[sizeof(t->descricao)-1] = '\0';
    t->valor = valor;
    t->taxa = taxa;
    t->saldoFinalBanco = u->banco.saldo;
    timestamp_now(t->dataHora, sizeof(t->dataHora));
}

/* registra em extrato do investimento (usa saldo do investimento como referência) */
void registrarTransacaoInvest(Usuario *u, const char *tipo, const char *desc, float valor, float taxa) {
    if (u->investimento.numTransacoes >= MAX_TRANSACOES) return;
    Transacao *t = &u->investimento.extrato[u->investimento.numTransacoes++];
    strncpy(t->tipo, tipo, sizeof(t->tipo)-1); t->tipo[sizeof(t->tipo)-1] = '\0';
    strncpy(t->descricao, desc, sizeof(t->descricao)-1); t->descricao[sizeof(t->descricao)-1] = '\0';
    t->valor = valor;
    t->taxa = taxa;
    t->saldoFinalBanco = u->investimento.saldo; // aqui usamos como "saldo pós-op" do caixa invest
    timestamp_now(t->dataHora, sizeof(t->dataHora));
}

void exibirExtratoBanco(Usuario *u) {
    printf("\n=== EXTRATO - CONTA BANCO ===\n");
    if (u->banco.numTransacoes == 0) printf("Nenhuma transação no banco.\n");
    for (int i = 0; i < u->banco.numTransacoes; ++i) {
        Transacao *t = &u->banco.extrato[i];
        printf("[%s] %-12s | %-30s | Valor: R$ %.2f | Taxa: R$ %.2f | Saldo pós-op: R$ %.2f\n",
               t->dataHora, t->tipo, t->descricao, t->valor, t->taxa, t->saldoFinalBanco);
    }
    printf("Saldo Banco: R$ %.2f\n", u->banco.saldo);
}

void exibirExtratoInvest(Usuario *u) {
    printf("\n=== EXTRATO - CONTA INVESTIMENTO (CAIXA) ===\n");
    if (u->investimento.numTransacoes == 0) printf("Nenhuma transação no investimento.\n");
    for (int i = 0; i < u->investimento.numTransacoes; ++i) {
        Transacao *t = &u->investimento.extrato[i];
        printf("[%s] %-12s | %-30s | Valor: R$ %.2f | Taxa: R$ %.2f | Saldo pós-op (caixa): R$ %.2f\n",
               t->dataHora, t->tipo, t->descricao, t->valor, t->taxa, t->saldoFinalBanco);
    }
    printf("Saldo caixa investimento: R$ %.2f\n", u->investimento.saldo);
}

/* ======= Cadastro / Login ======= */

void cadastrarUsuario(Usuario *u) {
    printf("\n=== Cadastro de Usuário ===\n");
    clear_input();
    printf("Nome: ");
    read_line(u->nome, sizeof(u->nome));
    printf("CPF (somente números): ");
    scanf("%15s", u->cpf);
    printf("Senha: ");
    scanf("%19s", u->senha);

    /* inicializa contas e histórico */
    u->banco.saldo = 0.0f;
    u->banco.numTransacoes = 0;
    u->investimento.saldo = 0.0f;
    u->investimento.numAtivos = 0;
    u->investimento.numTransacoes = 0;

    printf("Usuário '%s' cadastrado com sucesso!\n", u->nome);
}

bool validarLogin(Usuario *u) {
    char cpf[16], senha[21];
    printf("\n=== Login ===\n");
    printf("CPF: ");
    scanf("%15s", cpf);
    printf("Senha: ");
    scanf("%20s", senha);

    if (strcmp(cpf, u->cpf) == 0 && strcmp(senha, u->senha) == 0) {
        printf("Login efetuado! Bem-vindo, %s.\n", u->nome);
        return true;
    } else {
        printf("CPF ou senha incorretos.\n");
        return false;
    }
}

/* ======= Banco / Transferências ======= */

void depositarBanco(Usuario *u) {
    int tipo;
    printf("\n=== Depósito na Conta do Banco ===\n");
    printf("1 - PIX (sem taxa)\n");
    printf("2 - TED (com taxa de 1%%)\n");
    printf("Escolha: ");
    if (scanf("%d", &tipo) != 1) { clear_input(); printf("Entrada inválida.\n"); return; }

    float valor;
    printf("Digite o valor do depósito: R$ ");
    if (scanf("%f", &valor) != 1 || valor <= 0.0f) { clear_input(); printf("Valor inválido.\n"); return; }

    if (tipo == 2) {
        float taxa = valor * 0.01f;
        u->banco.saldo += (valor - taxa);
        char desc[80]; snprintf(desc, sizeof(desc), "Depósito TED R$ %.2f", valor);
        registrarTransacaoBanco(u, "TED", desc, valor - taxa, taxa);
    } else {
        u->banco.saldo += valor;
        char desc[80]; snprintf(desc, sizeof(desc), "Depósito PIX R$ %.2f", valor);
        registrarTransacaoBanco(u, "PIX", desc, valor, 0.0f);
    }

    printf("Depósito realizado. Saldo banco: R$ %.2f\n", u->banco.saldo);
}

void transferirParaInvestimento(Usuario *u) {
    float valor;
    printf("\n=== Transferência Banco -> Investimentos ===\n");
    printf("Saldo banco: R$ %.2f\n", u->banco.saldo);
    printf("Digite o valor para transferir: R$ ");
    if (scanf("%f", &valor) != 1 || valor <= 0.0f) { clear_input(); printf("Valor inválido.\n"); return; }

    if (valor > u->banco.saldo) { printf("Saldo insuficiente.\n"); return; }

    u->banco.saldo -= valor;
    u->investimento.saldo += valor;
    char desc[80]; snprintf(desc, sizeof(desc), "Transferência p/ Investimento R$ %.2f", valor);
    registrarTransacaoBanco(u, "Transferência", desc, -valor, 0.0f); // no banco fica como saída
    registrarTransacaoInvest(u, "Recebido", desc, valor, 0.0f); // no invest como entrada
    printf("Transferência concluída. Saldo caixa investimento: R$ %.2f\n", u->investimento.saldo);
}

void transferirParaBanco(Usuario *u) {
    float valor;
    printf("\n=== Resgate Investimentos -> Banco ===\n");
    printf("Saldo caixa em investimento: R$ %.2f\n", u->investimento.saldo);
    printf("Digite o valor para resgatar: R$ ");
    if (scanf("%f", &valor) != 1 || valor <= 0.0f) { clear_input(); printf("Valor inválido.\n"); return; }

    if (valor > u->investimento.saldo) { printf("Saldo insuficiente.\n"); return; }

    u->investimento.saldo -= valor;
    u->banco.saldo += valor;
    char desc[80]; snprintf(desc, sizeof(desc), "Resgate p/ Banco R$ %.2f", valor);
    registrarTransacaoInvest(u, "Resgate", desc, -valor, 0.0f);
    registrarTransacaoBanco(u, "Recebimento", desc, valor, 0.0f);
    printf("Resgate realizado. Saldo banco: R$ %.2f | saldo invest: R$ %.2f\n", u->banco.saldo, u->investimento.saldo);
}

void transferirParaBancoExterno(Usuario *u) {
    float valor;
    printf("\n=== Transferência Banco -> Externo ===\n");
    printf("Saldo banco: R$ %.2f\n", u->banco.saldo);
    printf("Digite o valor para transferir: R$ ");
    if (scanf("%f", &valor) != 1 || valor <= 0.0f) { clear_input(); printf("Valor inválido.\n"); return; }

    if (valor > u->banco.saldo) { printf("Saldo insuficiente.\n"); return; }

    u->banco.saldo -= valor;
    char desc[80]; snprintf(desc, sizeof(desc), "Transferência Externa R$ %.2f", valor);
    registrarTransacaoBanco(u, "Transferência Ext", desc, -valor, 0.0f);
    printf("Transferência externa concluída. Saldo banco: R$ %.2f\n", u->banco.saldo);
}

/* ======= Renda Variável: listagem, compra, venda, carteira ======= */

void listarAtivosDisponiveis(void) {
    printf("\n=== ATIVOS DISPONÍVEIS ===\n");
    for (int i = 0; i < NUM_ATIVOS; ++i) {
        AtivoRV *a = &ativosDisponiveis[i];
        printf("%2d) %s (%s) | Preço: R$ %.2f | Dividendo/p: R$ %.2f | %dx/ano | %s\n",
               i+1, a->nome, a->ticker, a->preco, a->dividend_per_period, a->periods_per_year, a->isFII ? "FII (isento)" : "Ação");
    }
}

/* compra de ativo: usa o saldo da conta de investimento (caixa) */
/* CORREÇÃO aplicada aqui: removi o uso inválido de 'quantity' e reescrevi a lógica */
void comprarAtivoRV(Usuario *u) {
    int escolha, quantidade;
    listarAtivosDisponiveis();
    printf("\nDigite o número do ativo para comprar (0 p/ cancelar): ");
    if (scanf("%d", &escolha) != 1) { clear_input(); printf("Entrada inválida.\n"); return; }
    if (escolha == 0) return;
    if (escolha < 1 || escolha > NUM_ATIVOS) { printf("Ativo inválido.\n"); return; }

    AtivoRV *a = &ativosDisponiveis[escolha - 1];
    printf("Quantidade de cotas para %s: ", a->ticker);
    if (scanf("%d", &quantidade) != 1 || quantidade <= 0) { clear_input(); printf("Quantidade inválida.\n"); return; }

    float custoTotal = a->preco * (float)quantidade;
    if (custoTotal > u->investimento.saldo) {
        printf("Saldo insuficiente! Caixa invest: R$ %.2f | Custo: R$ %.2f\n", u->investimento.saldo, custoTotal);
        return;
    }

    /* debita caixa */
    u->investimento.saldo -= custoTotal;

    /* atualiza carteira: acha por ticker */
    bool achou = false;
    for (int i = 0; i < u->investimento.numAtivos; ++i) {
        if (strcmp(u->investimento.carteira[i].ticker, a->ticker) == 0) {
            /* recalcula preço médio */
            int oldQtd = u->investimento.carteira[i].quantidade;
            float oldPM = u->investimento.carteira[i].precoMedio;
            int newQtd = oldQtd + quantidade;
            float newPM = ((oldPM * oldQtd) + (a->preco * quantidade)) / (float)newQtd;
            u->investimento.carteira[i].quantidade = newQtd;
            u->investimento.carteira[i].precoMedio = newPM;
            achou = true;
            break;
        }
    }
    if (!achou) {
        if (u->investimento.numAtivos >= MAX_ATIVOS) {
            printf("Limite carteira atingido.\n");
            /* opcional: re-credite o saldo -- aqui vamos creditar de volta */
            u->investimento.saldo += custoTotal;
            return;
        }
        int idx = u->investimento.numAtivos++;
        strncpy(u->investimento.carteira[idx].ticker, a->ticker, sizeof(u->investimento.carteira[idx].ticker)-1);
        u->investimento.carteira[idx].ticker[sizeof(u->investimento.carteira[idx].ticker)-1] = '\0';
        u->investimento.carteira[idx].quantidade = quantidade;
        u->investimento.carteira[idx].precoMedio = a->preco;
    }

    /* registra transação de compra no extrato de investimento */
    char desc[80]; snprintf(desc, sizeof(desc), "Compra %dx %s @ R$ %.2f", quantidade, a->ticker, a->preco);
    registrarTransacaoInvest(u, "Compra Ativo", desc, -custoTotal, 0.0f);

    printf("Compra efetuada: %d cotas de %s | Custo: R$ %.2f\n", quantidade, a->ticker, custoTotal);
    printf("Saldo caixa invest: R$ %.2f\n", u->investimento.saldo);
}

/* venda de ativo: crédito no caixa do investimento, remove ou diminui posição */
void venderAtivoRV(Usuario *u) {
    if (u->investimento.numAtivos == 0) {
        printf("\nCarteira vazia. Nada a vender.\n");
        return;
    }

    printf("\n=== VENDA DE ATIVOS ===\n");
    /* lista carteira com preços atuais */
    for (int i = 0; i < u->investimento.numAtivos; ++i) {
        AtivoCarteira *c = &u->investimento.carteira[i];
        /* encontra preço atual */
        float precoAtual = 0.0f;
        for (int j = 0; j < NUM_ATIVOS; ++j) {
            if (strcmp(c->ticker, ativosDisponiveis[j].ticker) == 0) {
                precoAtual = ativosDisponiveis[j].preco;
                break;
            }
        }
        printf("%2d) %s | Quant: %d | Preço atual: R$ %.2f | P. médio: R$ %.2f\n",
               i+1, c->ticker, c->quantidade, precoAtual, c->precoMedio);
    }

    int escolha;
    printf("Escolha o ativo para vender (0 p/ cancelar): ");
    if (scanf("%d", &escolha) != 1) { clear_input(); printf("Entrada inválida.\n"); return; }
    if (escolha == 0) return;
    if (escolha < 1 || escolha > u->investimento.numAtivos) { printf("Opção inválida.\n"); return; }

    AtivoCarteira *pos = &u->investimento.carteira[escolha - 1];
    int qtdVenda;
    printf("Quantidade para vender (%d disponível): ", pos->quantidade);
    if (scanf("%d", &qtdVenda) != 1 || qtdVenda <= 0) { clear_input(); printf("Quantidade inválida.\n"); return; }
    if (qtdVenda > pos->quantidade) { printf("Quantidade maior que a posição.\n"); return; }

    /* achar preço atual */
    float precoAtual = 0.0f;
    for (int j = 0; j < NUM_ATIVOS; ++j) {
        if (strcmp(pos->ticker, ativosDisponiveis[j].ticker) == 0) {
            precoAtual = ativosDisponiveis[j].preco;
            break;
        }
    }

    float valorVenda = precoAtual * (float)qtdVenda;
    /* credita na conta de investimento (caixa) */
    u->investimento.saldo += valorVenda;

    /* atualiza posição */
    pos->quantidade -= qtdVenda;
    if (pos->quantidade == 0) {
        /* remove entry (shift) */
        for (int k = escolha - 1; k < u->investimento.numAtivos - 1; ++k) {
            u->investimento.carteira[k] = u->investimento.carteira[k + 1];
        }
        u->investimento.numAtivos--;
    }

    /* registra transação de venda */
    char desc[80]; snprintf(desc, sizeof(desc), "Venda %dx %s @ R$ %.2f", qtdVenda, pos->ticker, precoAtual);
    registrarTransacaoInvest(u, "Venda Ativo", desc, valorVenda, 0.0f);

    printf("Venda efetuada! Recebeu R$ %.2f no caixa de investimento.\n", valorVenda);
    printf("Saldo caixa investimento: R$ %.2f\n", u->investimento.saldo);
}

/* mostra carteira com % alocado (caixa + ativos) */
void mostrarCarteira(Usuario *u) {
    printf("\n=== SUA CARTEIRA ===\n");
    if (u->investimento.numAtivos == 0) {
        printf("Carteira vazia.\n");
        printf("Saldo caixa (investimento): R$ %.2f\n", u->investimento.saldo);
        return;
    }

    /* calcula valor total = caixa + valor de mercado dos ativos */
    float total = u->investimento.saldo;
    for (int i = 0; i < u->investimento.numAtivos; ++i) {
        /* preço atual */
        float precoAtual = 0.0f;
        for (int j = 0; j < NUM_ATIVOS; ++j) {
            if (strcmp(u->investimento.carteira[i].ticker, ativosDisponiveis[j].ticker) == 0) {
                precoAtual = ativosDisponiveis[j].preco;
                break;
            }
        }
        total += precoAtual * (float)u->investimento.carteira[i].quantidade;
    }

    printf("Saldo caixa (investimento): R$ %.2f\n", u->investimento.saldo);
    printf("Valor total (caixa + ativos): R$ %.2f\n", total);

    for (int i = 0; i < u->investimento.numAtivos; ++i) {
        float precoAtual = 0.0f;
        char nomeAtivo[64] = "";
        for (int j = 0; j < NUM_ATIVOS; ++j) {
            if (strcmp(u->investimento.carteira[i].ticker, ativosDisponiveis[j].ticker) == 0) {
                precoAtual = ativosDisponiveis[j].preco;
                strncpy(nomeAtivo, ativosDisponiveis[j].nome, sizeof(nomeAtivo)-1);
                nomeAtivo[sizeof(nomeAtivo)-1] = '\0';
                break;
            }
        }
        float valorAtivo = precoAtual * (float)u->investimento.carteira[i].quantidade;
        float perc = (total > 0.0f) ? (valorAtivo / total * 100.0f) : 0.0f;
        printf("- %s (%s): %d cotas | Preço atual: R$ %.2f | Valor: R$ %.2f | %.2f%% | P. médio: R$ %.2f\n",
               u->investimento.carteira[i].ticker,
               nomeAtivo,
               u->investimento.carteira[i].quantidade,
               precoAtual,
               valorAtivo,
               perc,
               u->investimento.carteira[i].precoMedio);
    }
}

/* ========== NOVO: simulação de proventos RV ========== */

void simularProventosRV(Usuario *u) {
    int meses;
    printf("\nQuantos meses deseja simular? ");
    if (scanf("%d", &meses) != 1 || meses <= 0) { clear_input(); printf("Entrada inválida.\n"); return; }

    float totalRendimento = 0.0f;
    float perAssetTotals[MAX_ATIVOS] = {0.0f};

    printf("\n=== Simulação de Proventos (Renda Variável) por %d meses ===\n", meses);

    for (int m = 1; m <= meses; ++m) {
        for (int i = 0; i < u->investimento.numAtivos; ++i) {
            AtivoCarteira *c = &u->investimento.carteira[i];
            if (c->quantidade <= 0) continue;

            /* encontra ativo na lista global */
            int idx = -1;
            for (int j = 0; j < NUM_ATIVOS; ++j) {
                if (strcmp(c->ticker, ativosDisponiveis[j].ticker) == 0) { idx = j; break; }
            }
            if (idx == -1) continue;

            AtivoRV *a = &ativosDisponiveis[idx];
            if (a->periods_per_year <= 0) continue;

            int interval = 12 / a->periods_per_year;
            if (interval <= 0) interval = 12;

            if (m % interval == 0) {
                /* paga provento */
                float rendimento = a->dividend_per_period * (float)c->quantidade;
                /* creditamos no saldo do banco (conforme sua exigência) */
                u->banco.saldo += rendimento;
                totalRendimento += rendimento;
                perAssetTotals[i] += rendimento;

                char desc[100];
                snprintf(desc, sizeof(desc), "Provento %s (mês %d) R$ %.2f", a->ticker, m, rendimento);
                registrarTransacaoBanco(u, "Provento", desc, rendimento, 0.0f);
                /* também registrar como recebido no extrato do investimento (opcional) */
                char desc2[100];
                snprintf(desc2, sizeof(desc2), "Recebido %s (mês %d) R$ %.2f", a->ticker, m, rendimento);
                registrarTransacaoInvest(u, "Provento", desc2, rendimento, 0.0f);
            }
        }
    }

    /* exibe resumo por ativo (somente os da carteira) */
    printf("\n--- Resumo da Simulação ---\n");
    for (int i = 0; i < u->investimento.numAtivos; ++i) {
        if (perAssetTotals[i] != 0.0f) {
            printf("%s -> R$ %.2f\n", u->investimento.carteira[i].ticker, perAssetTotals[i]);
        }
    }
    printf("Total creditado na Conta Banco (proventos): R$ %.2f\n", totalRendimento);
}

/* submenu de ativos (compra, venda, carteira) */
void submenuAtivos(Usuario *u) {
    int op;
    do {
        printf("\n=== ATIVOS (Renda Variável) ===\n");
        printf("1 - Listar ativos disponíveis\n");
        printf("2 - Comprar ativo\n");
        printf("3 - Vender ativo\n");
        printf("4 - Mostrar carteira\n");
        printf("5 - Simular Proventos (RV)\n"); /* nova opção */
        printf("0 - Voltar\n");
        printf("Escolha: ");
        if (scanf("%d", &op) != 1) { clear_input(); printf("Entrada inválida.\n"); op = -1; }

        switch(op) {
            case 1: listarAtivosDisponiveis(); break;
            case 2: comprarAtivoRV(u); break;
            case 3: venderAtivoRV(u); break;
            case 4: mostrarCarteira(u); break;
            case 5: simularProventosRV(u); break; /* chama a simulação */
            case 0: break;
            default: printf("Opção inválida.\n"); break;
        }
    } while(op != 0);
}

/* ======= Menus principais ======= */

void menuContaBanco(Usuario *u) {
    int opc;
    do {
        printf("\n=== CONTA DO BANCO ===\n");
        printf("Olá %s | Saldo banco: R$ %.2f\n", u->nome, u->banco.saldo);
        printf("1 - Depositar (PIX/TED)\n");
        printf("2 - Transferir para Investimentos\n");
        printf("3 - Transferir para banco externo\n");
        printf("4 - Extrato (Banco)\n");
        printf("0 - Voltar\n");
        printf("Escolha: ");
        if (scanf("%d", &opc) != 1) { clear_input(); printf("Entrada inválida.\n"); opc = -1; }

        switch(opc) {
            case 1: depositarBanco(u); break;
            case 2: transferirParaInvestimento(u); break;
            case 3: transferirParaBancoExterno(u); break;
            case 4: exibirExtratoBanco(u); break;
            case 0: break;
            default: printf("Opção inválida.\n"); break;
        }
    } while(opc != 0);
}

void menuContaInvestimento(Usuario *u) {
    int opc;
    do {
        printf("\n=== CONTA DE INVESTIMENTOS ===\n");
        printf("Olá %s | Saldo caixa investimento: R$ %.2f\n", u->nome, u->investimento.saldo);
        printf("1 - Resgatar para Banco\n");
        printf("2 - Ativos (submenu)\n");
        printf("3 - Mostrar carteira\n");
        printf("4 - Extrato (Investimento)\n");
        printf("0 - Voltar\n");
        printf("Escolha: ");
        if (scanf("%d", &opc) != 1) { clear_input(); printf("Entrada inválida.\n"); opc = -1; }

        switch(opc) {
            case 1: transferirParaBanco(u); break;
            case 2: submenuAtivos(u); break;
            case 3: mostrarCarteira(u); break;
            case 4: exibirExtratoInvest(u); break;
            case 0: break;
            default: printf("Opção inválida.\n"); break;
        }
    } while(opc != 0);
}

void menuPrincipal(Usuario *u) {
    int opc;
    do {
        printf("\n=== MENU PRINCIPAL ===\n");
        printf("1 - Conta do Banco\n");
        printf("2 - Conta de Investimentos\n");
        printf("3 - Extrato completo (ambos)\n");
        printf("0 - Logout\n");
        printf("Escolha: ");
        if (scanf("%d", &opc) != 1) { clear_input(); printf("Entrada inválida.\n"); opc = -1; }

        switch(opc) {
            case 1: menuContaBanco(u); break;
            case 2: menuContaInvestimento(u); break;
            case 3:
                exibirExtratoBanco(u);
                exibirExtratoInvest(u);
                break;
            case 0: printf("Logout...\n"); break;
            default: printf("Opção inválida.\n"); break;
        }
    } while(opc != 0);
}

/* ======= Main ======= */

int main(void) {
    setlocale(LC_ALL, "");
    Usuario usuario;
    memset(&usuario, 0, sizeof(usuario)); // zera tudo

    int opc;
    do {
        printf("\n=== Sistema Corretora ===\n");
        printf("1 - Cadastrar Usuário\n");
        printf("2 - Login\n");
        printf("0 - Sair\n");
        printf("Escolha: ");
        if (scanf("%d", &opc) != 1) { clear_input(); printf("Entrada inválida.\n"); opc = -1; }

        switch(opc) {
            case 1:
                cadastrarUsuario(&usuario);
                break;
            case 2:
                if (strlen(usuario.cpf) == 0) {
                    printf("Nenhum usuário cadastrado. Cadastre primeiro.\n");
                    break;
                }
                if (validarLogin(&usuario)) {
                    menuPrincipal(&usuario);
                }
                break;
            case 0: printf("Encerrando...\n"); break;
            default: printf("Opção inválida.\n"); break;
        }
    } while(opc != 0);

    return 0;
}

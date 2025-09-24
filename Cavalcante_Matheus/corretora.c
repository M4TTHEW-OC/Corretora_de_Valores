// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <locale.h>
#include <time.h>

/* ===== Constantes ===== */
#define MAX_TRANSACOES 200
#define MAX_ATIVOS 20
#define MAX_NOME 50
#define MAX_CPF 15
#define MAX_SENHA 20

/* ===== Tipos / Estruturas ===== */
typedef struct {
    float saldo;
} ContaBanco;

/* Carteira do usuário - por ativo */
typedef struct {
    char ticker[16];
    int quantidade;
    float precoMedio; // preço médio de compra por cota
} AtivoCarteira;

typedef struct {
    float saldo;
    AtivoCarteira carteira[MAX_ATIVOS];
    int numAtivos;
} ContaInvestimento;

/* Transação para extrato */
typedef struct {
    char tipo[30];      // ex: "PIX", "TED", "Compra", "Resgate"
    char descricao[60]; // ex: "Compra 10x CEMIG"
    float valor;        // valor movimentado
    float taxa;         // taxa aplicada (se houver)
    float saldoFinal;   // saldo da conta do banco após operação (para extrato do banco)
    char dataHora[20];  // formato dd/mm HH:MM
} Transacao;

/* Ativo disponível na simulação (renda variável) */
typedef struct {
    char ticker[16];
    char nome[50];
    float preco;           // preço por cota
    float dividendo;       // dividendo por cota por período (simulação)
    int freqPerYear;       // quantas vezes por ano paga
    bool isFII;            // se for FII (rendimentos isentos)
} AtivoRV;

/* Usuário do sistema */
typedef struct {
    char nome[MAX_NOME];
    char cpf[MAX_CPF];
    char senha[MAX_SENHA];
    ContaBanco banco;
    ContaInvestimento investimento;
    Transacao historico[MAX_TRANSACOES];
    int qtdTransacoes;
} Usuario;

/* ===== Ativos pré-definidos ===== */
AtivoRV ativosDisponiveis[] = {
    { "SANEPAR", "Sanepar",         20.00f, 1.50f, 1,  false },
    { "CEMIG",   "Cemig",           10.00f, 0.60f, 2,  false },
    { "BBAS3",   "Banco do Brasil", 30.00f, 0.35f, 4,  false },
    { "ITAU",    "Itaú",            25.00f, 0.05f,12,  false },
    { "HGLG11",  "FII HGLG11",      80.00f, 0.60f,12,  true  }
};
const int NUM_ATIVOS = sizeof(ativosDisponiveis) / sizeof(AtivoRV);

/* ===== Protótipos ===== */
/* utilitários */
void clear_input(void);
void read_line(char *buf, int size);
void registrarTransacao(Usuario *u, const char *tipo, const char *descricao, float valor, float taxa, float saldoFinal);
void exibirExtrato(Usuario *u);

/* cadastro/login */
void cadastrarUsuario(Usuario *u);
bool validarLogin(Usuario *u);

/* menus */
void menuPrincipal(Usuario *u);
void menuContaBanco(Usuario *u);
void menuContaInvestimento(Usuario *u);
void submenuAtivos(Usuario *u);

/* banco / transações */
void depositarBanco(Usuario *u);
void depositarBancoTED(Usuario *u);
void transferirParaInvestimento(Usuario *u);
void transferirParaBanco(Usuario *u);
void transferirParaBancoExterno(Usuario *u);

/* renda variável */
void listarAtivosDisponiveis(void);
void comprarAtivoRV(Usuario *u);
void mostrarCarteira(Usuario *u);

/* ===== Implementações ===== */

void clear_input(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

void read_line(char *buf, int size) {
    if (fgets(buf, size, stdin) == NULL) { buf[0] = '\0'; return; }
    size_t len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
}

/* registra uma transação no histórico (apenas para extrato didático) */
void registrarTransacao(Usuario *u, const char *tipo, const char *descricao, float valor, float taxa, float saldoFinal) {
    if (u->qtdTransacoes >= MAX_TRANSACOES) return;
    Transacao *t = &u->historico[u->qtdTransacoes++];
    strncpy(t->tipo, tipo, sizeof(t->tipo)-1); t->tipo[sizeof(t->tipo)-1] = '\0';
    strncpy(t->descricao, descricao, sizeof(t->descricao)-1); t->descricao[sizeof(t->descricao)-1] = '\0';
    t->valor = valor;
    t->taxa = taxa;
    t->saldoFinal = saldoFinal;
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    if (lt) strftime(t->dataHora, sizeof(t->dataHora), "%d/%m %H:%M", lt);
    else strncpy(t->dataHora, "00/00 00:00", sizeof(t->dataHora));
}

void exibirExtrato(Usuario *u) {
    printf("\n===== EXTRATO (últimas %d) =====\n", u->qtdTransacoes);
    if (u->qtdTransacoes == 0) {
        printf("Nenhuma transação registrada.\n");
    } else {
        for (int i = 0; i < u->qtdTransacoes; ++i) {
            Transacao *t = &u->historico[i];
            printf("[%s] %-10s | %-30s | Valor: R$ %.2f | Taxa: R$ %.2f | Saldo pós-op: R$ %.2f\n",
                   t->dataHora, t->tipo, t->descricao, t->valor, t->taxa, t->saldoFinal);
        }
    }
    printf("Saldo Banco: R$ %.2f | Saldo Investimento (caixa): R$ %.2f\n",
           u->banco.saldo, u->investimento.saldo);
}

/* ========== Cadastro / Login ========== */
void cadastrarUsuario(Usuario *u) {
    printf("\n=== Cadastro de Usuário ===\n");
    clear_input();
    printf("Nome: ");
    read_line(u->nome, sizeof(u->nome));
    printf("CPF: ");
    scanf("%14s", u->cpf);
    printf("Senha: ");
    scanf("%19s", u->senha);

    u->banco.saldo = 0.0f;
    u->investimento.saldo = 0.0f;
    u->investimento.numAtivos = 0;
    u->qtdTransacoes = 0;

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

/* ========== Banco e Transações ========== */

void depositarBanco(Usuario *u) {
    int tipo;
    printf("\n=== Depósito na Conta do Banco ===\n");
    printf("1 - PIX (sem taxa)\n");
    printf("2 - TED (c/ taxa de 1%%)\n");
    printf("Escolha: ");
    if (scanf("%d", &tipo) != 1) { clear_input(); printf("Entrada inválida.\n"); return; }

    float valor;
    printf("Digite o valor do depósito: R$ ");
    if (scanf("%f", &valor) != 1 || valor <= 0.0f) { clear_input(); printf("Valor inválido.\n"); return; }

    if (tipo == 2) {
        float taxa = valor * 0.01f;
        u->banco.saldo += (valor - taxa);
        char desc[60]; snprintf(desc, sizeof(desc), "Depósito TED R$ %.2f", valor);
        registrarTransacao(u, "TED", desc, valor, taxa, u->banco.saldo);
    } else {
        u->banco.saldo += valor;
        char desc[60]; snprintf(desc, sizeof(desc), "Depósito PIX R$ %.2f", valor);
        registrarTransacao(u, "PIX", desc, valor, 0.0f, u->banco.saldo);
    }

    printf("Depósito realizado. Saldo atual do banco: R$ %.2f\n", u->banco.saldo);
}

void transferirParaInvestimento(Usuario *u) {
    float valor;
    printf("\n=== Transferência Banco -> Investimentos ===\n");
    printf("Saldo disponível no banco: R$ %.2f\n", u->banco.saldo);
    printf("Digite o valor para transferir: R$ ");
    if (scanf("%f", &valor) != 1 || valor <= 0.0f) { clear_input(); printf("Valor inválido.\n"); return; }

    if (valor <= u->banco.saldo) {
        u->banco.saldo -= valor;
        u->investimento.saldo += valor;
        char desc[80]; snprintf(desc, sizeof(desc), "Transferência p/ Invest. R$ %.2f", valor);
        registrarTransacao(u, "Transferência", desc, valor, 0.0f, u->banco.saldo);
        printf("Transferência concluída. Saldo investimento (caixa): R$ %.2f\n", u->investimento.saldo);
    } else {
        printf("Saldo insuficiente.\n");
    }
}

void transferirParaBanco(Usuario *u) {
    float valor;
    printf("\n=== Resgate Investimentos -> Banco ===\n");
    printf("Saldo disponível em investimentos (caixa): R$ %.2f\n", u->investimento.saldo);
    printf("Digite o valor para resgatar: R$ ");
    if (scanf("%f", &valor) != 1 || valor <= 0.0f) { clear_input(); printf("Valor inválido.\n"); return; }

    if (valor <= u->investimento.saldo) {
        u->investimento.saldo -= valor;
        u->banco.saldo += valor;
        char desc[80]; snprintf(desc, sizeof(desc), "Resgate p/ Banco R$ %.2f", valor);
        registrarTransacao(u, "Resgate", desc, valor, 0.0f, u->banco.saldo);
        printf("Resgate realizado. Saldo banco: R$ %.2f | Saldo investimento: R$ %.2f\n",
               u->banco.saldo, u->investimento.saldo);
    } else {
        printf("Saldo insuficiente.\n");
    }
}

void transferirParaBancoExterno(Usuario *u) {
    float valor;
    printf("\n=== Transferência Banco -> Externo ===\n");
    printf("Saldo disponível no banco: R$ %.2f\n", u->banco.saldo);
    printf("Digite o valor para transferir: R$ ");
    if (scanf("%f", &valor) != 1 || valor <= 0.0f) { clear_input(); printf("Valor inválido.\n"); return; }

    if (valor <= u->banco.saldo) {
        u->banco.saldo -= valor;
        char desc[80]; snprintf(desc, sizeof(desc), "Transferência Externa R$ %.2f", valor);
        registrarTransacao(u, "Transferência Ext", desc, valor, 0.0f, u->banco.saldo);
        printf("Transferência externa concluída. Saldo banco: R$ %.2f\n", u->banco.saldo);
    } else {
        printf("Saldo insuficiente.\n");
    }
}

/* ========== Renda Variável (compra / carteira) ========== */

void listarAtivosDisponiveis(void) {
    printf("\n=== ATIVOS DISPONÍVEIS (Renda Variável) ===\n");
    for (int i = 0; i < NUM_ATIVOS; ++i) {
        AtivoRV *a = &ativosDisponiveis[i];
        printf("%2d) %s - %s | Preço: R$ %.2f | Dividendo/p: R$ %.2f | %dx/ano | %s\n",
               i+1, a->ticker, a->nome, a->preco, a->dividendo, a->freqPerYear, a->isFII ? "FII (isento)" : "Ação");
    }
}

void comprarAtivoRV(Usuario *u) {
    int escolha, quantidade;
    listarAtivosDisponiveis();
    printf("\nDigite o número do ativo que deseja comprar (0 p/ cancelar): ");
    if (scanf("%d", &escolha) != 1) { clear_input(); printf("Entrada inválida.\n"); return; }
    if (escolha == 0) return;
    if (escolha < 1 || escolha > NUM_ATIVOS) { printf("Ativo inválido.\n"); return; }
    AtivoRV *a = &ativosDisponiveis[escolha - 1];

    printf("Quantidade de cotas para %s: ", a->ticker);
    if (scanf("%d", &quantidade) != 1 || quantidade <= 0) { clear_input(); printf("Quantidade inválida.\n"); return; }

    float custoTotal = a->preco * (float)quantidade;
    if (custoTotal > u->investimento.saldo) {
        printf("Saldo insuficiente! Saldo investimento (caixa): R$ %.2f | Custo: R$ %.2f\n",
               u->investimento.saldo, custoTotal);
        return;
    }

    // debita caixa da conta de investimento
    u->investimento.saldo -= custoTotal;

    // procura se já tem o ativo na carteira
    bool found = false;
    for (int i = 0; i < u->investimento.numAtivos; ++i) {
        if (strcmp(u->investimento.carteira[i].ticker, a->ticker) == 0) {
            // recalcula preço médio
            int oldQtd = u->investimento.carteira[i].quantidade;
            float oldPM = u->investimento.carteira[i].precoMedio;
            int newQtd = oldQtd + quantidade;
            float newPM = ((oldPM * (float)oldQtd) + (a->preco * (float)quantidade)) / (float)newQtd;
            u->investimento.carteira[i].quantidade = newQtd;
            u->investimento.carteira[i].precoMedio = newPM;
            found = true;
            break;
        }
    }

    if (!found) {
        if (u->investimento.numAtivos >= MAX_ATIVOS) {
            printf("Limite de ativos na carteira atingido.\n");
            // se quiser, poderia credit back o saldo (aqui não reverte)
            return;
        }
        strncpy(u->investimento.carteira[u->investimento.numAtivos].ticker, a->ticker, sizeof(u->investimento.carteira[0].ticker)-1);
        u->investimento.carteira[u->investimento.numAtivos].ticker[sizeof(u->investimento.carteira[0].ticker)-1] = '\0';
        u->investimento.carteira[u->investimento.numAtivos].quantidade = quantidade;
        u->investimento.carteira[u->investimento.numAtivos].precoMedio = a->preco;
        u->investimento.numAtivos++;
    }

    // registrar transação (compra) no extrato do banco (ou no histórico geral)
    char desc[80];
    snprintf(desc, sizeof(desc), "Compra %dx %s @ R$ %.2f", quantidade, a->ticker, a->preco);
    registrarTransacao(u, "Compra Ativo", desc, custoTotal, 0.0f, u->banco.saldo);

    printf("Compra realizada: %d cotas de %s | Custo total: R$ %.2f\n", quantidade, a->ticker, custoTotal);
    printf("Saldo caixa em investimento agora: R$ %.2f\n", u->investimento.saldo);
}

void mostrarCarteira(Usuario *u) {
    printf("\n=== SUA CARTEIRA ===\n");
    if (u->investimento.numAtivos == 0) {
        printf("Carteira vazia.\n");
        printf("Saldo caixa (investimento): R$ %.2f\n", u->investimento.saldo);
        return;
    }

    // soma total considerando caixa + valor de mercado dos ativos
    float total = u->investimento.saldo;
    for (int i = 0; i < u->investimento.numAtivos; ++i) {
        // encontra preço atual do ativo
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
    printf("Valor total da carteira (caixa + ativos): R$ %.2f\n", total);

    for (int i = 0; i < u->investimento.numAtivos; ++i) {
        // encontra preço atual do ativo
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
        printf("- %s (%s): %d cotas | Preço atual: R$ %.2f | Valor: R$ %.2f | %.2f%% da carteira | P. médio: R$ %.2f\n",
               u->investimento.carteira[i].ticker,
               nomeAtivo,
               u->investimento.carteira[i].quantidade,
               precoAtual,
               valorAtivo,
               perc,
               u->investimento.carteira[i].precoMedio);
    }
}

/* submenu de ativos (apenas lista + compra por enquanto) */
void submenuAtivos(Usuario *u) {
    int op;
    do {
        printf("\n=== ATIVOS (Renda Variável) ===\n");
        printf("1 - Listar ativos disponíveis\n");
        printf("2 - Comprar ativo\n");
        printf("3 - Mostrar carteira\n");
        printf("0 - Voltar\n");
        printf("Escolha: ");
        if (scanf("%d", &op) != 1) { clear_input(); printf("Entrada inválida.\n"); op = -1; }

        switch(op) {
            case 1: listarAtivosDisponiveis(); break;
            case 2: comprarAtivoRV(u); break;
            case 3: mostrarCarteira(u); break;
            case 0: break;
            default: printf("Opção inválida.\n"); break;
        }
    } while(op != 0);
}

/* ========== Menus principais ========== */

void menuContaBanco(Usuario *u) {
    int opc;
    do {
        printf("\n=== CONTA DO BANCO ===\n");
        printf("Olá %s | Saldo banco: R$ %.2f\n", u->nome, u->banco.saldo);
        printf("1 - Depositar (PIX/TED)\n");
        printf("2 - Transferir para Investimentos\n");
        printf("3 - Transferir para banco externo\n");
        printf("4 - Extrato\n");
        printf("0 - Voltar\n");
        printf("Escolha: ");
        if (scanf("%d", &opc) != 1) { clear_input(); printf("Entrada inválida.\n"); opc = -1; }

        switch(opc) {
            case 1: depositarBanco(u); break;
            case 2: transferirParaInvestimento(u); break;
            case 3: transferirParaBancoExterno(u); break;
            case 4: exibirExtrato(u); break;
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
        printf("2 - Ativos de Renda Variável (submenu)\n");
        printf("3 - Mostrar carteira\n");
        printf("4 - Extrato\n");
        printf("0 - Voltar\n");
        printf("Escolha: ");
        if (scanf("%d", &opc) != 1) { clear_input(); printf("Entrada inválida.\n"); opc = -1; }

        switch(opc) {
            case 1: transferirParaBanco(u); break;
            case 2: submenuAtivos(u); break;
            case 3: mostrarCarteira(u); break;
            case 4: exibirExtrato(u); break;
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
        printf("3 - Extrato completo\n");
        printf("0 - Logout\n");
        printf("Escolha: ");
        if (scanf("%d", &opc) != 1) { clear_input(); printf("Entrada inválida.\n"); opc = -1; }

        switch(opc) {
            case 1: menuContaBanco(u); break;
            case 2: menuContaInvestimento(u); break;
            case 3: exibirExtrato(u); break;
            case 0: printf("Logout...\n"); break;
            default: printf("Opção inválida.\n"); break;
        }
    } while(opc != 0);
}



/* ========== main ========== */

int main(void) {
    setlocale(LC_ALL, "");
    Usuario usuario;
    memset(&usuario, 0, sizeof(usuario)); // garante zero inicial

    int opc;
    bool logado = false;

    do {
        printf("\n=== Sistema Corretora ===\n");
        printf("1 - Cadastrar Usuário\n");
        printf("2 - Login\n");
        printf("0 - Sair\n");
        printf("Escolha: ");
        if (scanf("%d", &opc) != 1) { clear_input(); printf("Entrada inválida.\n"); opc = -1; }

        switch(opc) {
            case 1: cadastrarUsuario(&usuario); break;
            case 2:
                if (strlen(usuario.cpf) == 0) {
                    printf("Nenhum usuário cadastrado ainda. Cadastre-se primeiro.\n");
                    break;
                }
                if (validarLogin(&usuario)) {
                    logado = true;
                    menuPrincipal(&usuario);
                }
                break;
            case 0: printf("Encerrando...\n"); break;
            default: printf("Opção inválida.\n"); break;
        }
    } while(opc != 0);

    return 0;
}

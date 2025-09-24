#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <locale.h>
#include <errno.h>

// ===== Constantes =====
#define MAX_TRANSACOES 100
#define MAX_ATIVOS 50

// ===== Estruturas =====
typedef struct {
    char descricao[100];
    float valor;
    float taxa;
} Transacao;

typedef struct {
    Transacao historico[MAX_TRANSACOES];
    int totalTransacoes;
    float saldo;
} ContaBanco;

typedef struct {
    char ticker[10];
    int quantidade;
    float precoMedio;
} AtivoCarteira;

typedef struct {
    Transacao historico[MAX_TRANSACOES];
    int totalTransacoes;
    float saldo;
    AtivoCarteira carteira[MAX_ATIVOS];
    int numAtivos;
} ContaInvestimento;

typedef struct {
    char ticker[10];
    char nome[50];
    float preco;
    float dividendo; 
    float acumulado;
    int frequencia; // vezes ao ano
    bool isFII;
} AtivoRV;

typedef struct {
    char nome[50];
    char cpf[15];
    char senha[20];
    ContaBanco banco;
    ContaInvestimento investimento;
} Usuario;

// ===== Ativos disponíveis =====
AtivoRV ativosDisponiveis[] = {
    { "SANEPAR", "Sanepar",      20.00f,  1.50f, 0.0f, 1,  false },
    { "CEMIG",   "Cemig",        10.00f,  0.60f, 0.0f, 2,  false },
    { "BBAS3",   "Banco do Brasil",30.0f, 0.35f, 0.0f, 4, false },
    { "ITAU",    "Itaú",         25.00f,  0.05f, 0.0f, 12, false },
    { "HGLG11",  "FII HGLG11",   80.00f,  0.60f, 0.0f, 12, true }
};
const int NUM_ATIVOS = sizeof(ativosDisponiveis) / sizeof(AtivoRV);

// ===== Protótipos =====
void cadastrarUsuario(Usuario *u);
bool validarLogin(Usuario *u);
void menuPrincipal(Usuario *u);
void menuContaBanco(Usuario *u);
void menuContaInvestimento(Usuario *u);
void depositarBanco(Usuario *u);
void transferirParaInvestimento(Usuario *u);
void transferirParaBanco(Usuario *u);
void transferirParaBancoExterno(Usuario *u);
void verExtratoBanco(Usuario *u);
void verExtratoInvest(Usuario *u);
void listarAtivosDisponiveis(void);
void comprarAtivoRV(Usuario *u);
void venderAtivoRV(Usuario *u);
void mostrarCarteira(Usuario *u);
void simularPassagemTempo(Usuario *u);

// ===== Funções de apoio =====
void clear_input() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void registrarTransacaoBanco(Usuario *u, const char *desc, float valor, float taxa) {
    if (u->banco.totalTransacoes < MAX_TRANSACOES) {
        Transacao *t = &u->banco.historico[u->banco.totalTransacoes++];
        strncpy(t->descricao, desc, sizeof(t->descricao));
        t->valor = valor;
        t->taxa = taxa;
    }
}

void registrarTransacaoInvest(Usuario *u, const char *desc, float valor, float taxa) {
    if (u->investimento.totalTransacoes < MAX_TRANSACOES) {
        Transacao *t = &u->investimento.historico[u->investimento.totalTransacoes++];
        strncpy(t->descricao, desc, sizeof(t->descricao));
        t->valor = valor;
        t->taxa = taxa;
    }
}

// ===== Cadastro/Login =====
void cadastrarUsuario(Usuario *u) {
    printf("\n=== Cadastro de Usuário ===\n");
    printf("Nome: ");
    scanf(" %[^\n]", u->nome);
    printf("CPF: ");
    scanf("%s", u->cpf);
    printf("Senha: ");
    scanf("%s", u->senha);

    u->banco.saldo = 0;
    u->banco.totalTransacoes = 0;
    u->investimento.saldo = 0;
    u->investimento.totalTransacoes = 0;
    u->investimento.numAtivos = 0;

    printf("Usuário %s cadastrado com sucesso!\n", u->nome);
}

bool validarLogin(Usuario *u) {
    char cpf[15], senha[20];
    printf("\n=== Login ===\n");
    printf("CPF: ");
    scanf("%s", cpf);
    printf("Senha: ");
    scanf("%s", senha);

    if (strcmp(cpf, u->cpf) == 0 && strcmp(senha, u->senha) == 0) {
        printf("Login realizado com sucesso! Bem-vindo, %s.\n", u->nome);
        return true;
    } else {
        printf("CPF ou senha incorretos.\n");
        return false;
    }
}

// ===== Conta Banco =====
void depositarBanco(Usuario *u) {
    float valor;
    printf("Valor do depósito: R$ ");
    scanf("%f", &valor);
    if (valor > 0) {
        u->banco.saldo += valor;
        registrarTransacaoBanco(u, "Depósito", valor, 0);
        printf("Depósito realizado. Novo saldo: R$ %.2f\n", u->banco.saldo);
    }
}

void transferirParaInvestimento(Usuario *u) {
    float valor;
    printf("Valor para transferir para investimentos: R$ ");
    scanf("%f", &valor);
    if (valor <= u->banco.saldo) {
        u->banco.saldo -= valor;
        u->investimento.saldo += valor;
        registrarTransacaoBanco(u, "Transferência para Investimento", -valor, 0);
        registrarTransacaoInvest(u, "Transferência do Banco", valor, 0);
        printf("Transferência realizada.\n");
    } else {
        printf("Saldo insuficiente.\n");
    }
}

void transferirParaBanco(Usuario *u) {
    float valor;
    printf("Valor para transferir para banco: R$ ");
    scanf("%f", &valor);
    if (valor <= u->investimento.saldo) {
        u->investimento.saldo -= valor;
        u->banco.saldo += valor;
        registrarTransacaoInvest(u, "Transferência para Banco", -valor, 0);
        registrarTransacaoBanco(u, "Transferência de Investimento", valor, 0);
        printf("Transferência realizada.\n");
    } else {
        printf("Saldo insuficiente.\n");
    }
}

void verExtratoBanco(Usuario *u) {
    printf("\n=== Extrato Banco ===\n");
    for (int i = 0; i < u->banco.totalTransacoes; i++) {
        printf("%s: %.2f (taxa: %.2f)\n",
               u->banco.historico[i].descricao,
               u->banco.historico[i].valor,
               u->banco.historico[i].taxa);
    }
    printf("Saldo atual: %.2f\n", u->banco.saldo);
}

// ===== Conta Investimentos =====
void listarAtivosDisponiveis(void) {
    printf("\n=== Ativos Disponíveis ===\n");
    for (int i = 0; i < NUM_ATIVOS; i++) {
        printf("%s (%s) - Preço: %.2f | Dividendo: %.2f | Frequência: %d\n",
               ativosDisponiveis[i].ticker,
               ativosDisponiveis[i].nome,
               ativosDisponiveis[i].preco,
               ativosDisponiveis[i].dividendo,
               ativosDisponiveis[i].frequencia);
    }
}

void comprarAtivoRV(Usuario *u) {
    listarAtivosDisponiveis();
    char ticker[10];
    int qtd;
    printf("Digite o ticker para comprar: ");
    scanf("%s", ticker);
    printf("Quantidade: ");
    scanf("%d", &qtd);

    for (int i = 0; i < NUM_ATIVOS; i++) {
        if (strcmp(ativosDisponiveis[i].ticker, ticker) == 0) {
            float custo = ativosDisponiveis[i].preco * qtd;
            if (u->investimento.saldo >= custo) {
                u->investimento.saldo -= custo;
                int encontrado = -1;
                for (int j = 0; j < u->investimento.numAtivos; j++) {
                    if (strcmp(u->investimento.carteira[j].ticker, ticker) == 0) {
                        encontrado = j;
                        break;
                    }
                }
                if (encontrado == -1) {
                    AtivoCarteira novo;
                    strcpy(novo.ticker, ticker);
                    novo.quantidade = qtd;
                    novo.precoMedio = ativosDisponiveis[i].preco;
                    u->investimento.carteira[u->investimento.numAtivos++] = novo;
                } else {
                    AtivoCarteira *a = &u->investimento.carteira[encontrado];
                    a->precoMedio = ((a->precoMedio * a->quantidade) + custo) / (a->quantidade + qtd);
                    a->quantidade += qtd;
                }
                registrarTransacaoInvest(u, "Compra de ativo", -custo, 0);
                printf("Compra realizada.\n");
            } else {
                printf("Saldo insuficiente.\n");
            }
            return;
        }
    }
    printf("Ativo não encontrado.\n");
}

void venderAtivoRV(Usuario *u) {
    mostrarCarteira(u);
    char ticker[10];
    int qtd;
    printf("Digite o ticker para vender: ");
    scanf("%s", ticker);
    printf("Quantidade: ");
    scanf("%d", &qtd);

    for (int i = 0; i < u->investimento.numAtivos; i++) {
        if (strcmp(u->investimento.carteira[i].ticker, ticker) == 0) {
            if (u->investimento.carteira[i].quantidade >= qtd) {
                for (int j = 0; j < NUM_ATIVOS; j++) {
                    if (strcmp(ativosDisponiveis[j].ticker, ticker) == 0) {
                        float valorVenda = ativosDisponiveis[j].preco * qtd;
                        u->investimento.saldo += valorVenda;
                        u->investimento.carteira[i].quantidade -= qtd;
                        registrarTransacaoInvest(u, "Venda de ativo", valorVenda, 0);
                        printf("Venda realizada: %.2f\n", valorVenda);
                        if (u->investimento.carteira[i].quantidade == 0) {
                            for (int k = i; k < u->investimento.numAtivos - 1; k++) {
                                u->investimento.carteira[k] = u->investimento.carteira[k+1];
                            }
                            u->investimento.numAtivos--;
                        }
                        return;
                    }
                }
            } else {
                printf("Quantidade insuficiente.\n");
                return;
            }
        }
    }
    printf("Ativo não encontrado na carteira.\n");
}

void mostrarCarteira(Usuario *u) {
    printf("\n=== Carteira ===\n");
    float total = 0;
    for (int i = 0; i < u->investimento.numAtivos; i++) {
        for (int j = 0; j < NUM_ATIVOS; j++) {
            if (strcmp(u->investimento.carteira[i].ticker, ativosDisponiveis[j].ticker) == 0) {
                float val = u->investimento.carteira[i].quantidade * ativosDisponiveis[j].preco;
                total += val;
                printf("%s - Qtd: %d | Valor: %.2f\n",
                       u->investimento.carteira[i].ticker,
                       u->investimento.carteira[i].quantidade,
                       val);
            }
        }
    }
    printf("Total carteira: %.2f | Saldo disponível: %.2f\n", total, u->investimento.saldo);
}

void verExtratoInvest(Usuario *u) {
    printf("\n=== Extrato Investimentos ===\n");
    for (int i = 0; i < u->investimento.totalTransacoes; i++) {
        printf("%s: %.2f (taxa: %.2f)\n",
               u->investimento.historico[i].descricao,
               u->investimento.historico[i].valor,
               u->investimento.historico[i].taxa);
    }
    printf("Saldo atual: %.2f\n", u->investimento.saldo);
}

void simularPassagemTempo(Usuario *u) {
    printf("\n=== Simulação de Rendimentos ===\n");
    for (int i = 0; i < u->investimento.numAtivos; i++) {
        for (int j = 0; j < NUM_ATIVOS; j++) {
            if (strcmp(u->investimento.carteira[i].ticker, ativosDisponiveis[j].ticker) == 0) {
                float rend = u->investimento.carteira[i].quantidade *
                             (ativosDisponiveis[j].dividendo / ativosDisponiveis[j].frequencia);
                u->investimento.saldo += rend;
                registrarTransacaoInvest(u, "Rendimento", rend, 0);
                printf("%s rendeu: %.2f\n", ativosDisponiveis[j].ticker, rend);
            }
        }
    }
}

// ===== Menus =====
void menuContaBanco(Usuario *u) {
    int opc;
    do {
        printf("\n=== Conta Banco ===\n");
        printf("1 - Depositar\n2 - Transferir p/ Investimento\n3 - Extrato\n0 - Voltar\n");
        scanf("%d", &opc);
        switch(opc) {
            case 1: depositarBanco(u); break;
            case 2: transferirParaInvestimento(u); break;
            case 3: verExtratoBanco(u); break;
        }
    } while(opc != 0);
}

void menuContaInvestimento(Usuario *u) {
    int opc;
    do {
        printf("\n=== Conta Invest ===\n");
        printf("1 - Ver extrato\n2 - Comprar Ativo\n3 - Vender Ativo\n4 - Carteira\n5 - Simular Tempo\n0 - Voltar\n");
        scanf("%d", &opc);
        switch(opc) {
            case 1: verExtratoInvest(u); break;
            case 2: comprarAtivoRV(u); break;
            case 3: venderAtivoRV(u); break;
            case 4: mostrarCarteira(u); break;
            case 5: simularPassagemTempo(u); break;
        }
    } while(opc != 0);
}

void menuPrincipal(Usuario *u) {
    int opc;
    do {
        printf("\n=== Menu Principal ===\n");
        printf("1 - Conta Banco\n2 - Conta Investimentos\n0 - Sair\n");
        scanf("%d", &opc);
        switch(opc) {
            case 1: menuContaBanco(u); break;
            case 2: menuContaInvestimento(u); break;
        }
    } while(opc != 0);
}

// ===== Main =====
int main() {
    setlocale(LC_ALL, "Portuguese");
    Usuario user;
    cadastrarUsuario(&user);
    if (validarLogin(&user)) {
        menuPrincipal(&user);
    }
    return 0;
}

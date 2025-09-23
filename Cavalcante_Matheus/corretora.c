#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <locale.h>
#include <time.h>

// ===== Estruturas =====
#define MAX_TRANSACOES 100

typedef struct {
    float saldo;
} ContaBanco;

typedef struct {
    float saldo;
} ContaInvestimento;

typedef struct {
    char descricao[100];
    float valor;
    char tipo[20]; // PIX, TED, Transferência, Resgate
    char data[30];
} Transacao;

typedef struct {
    char nome[50];
    char cpf[15];
    char senha[20];
    ContaBanco banco;
    ContaInvestimento investimento;
    Transacao historico[MAX_TRANSACOES];
    int qtdTransacoes;
} Usuario;

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
void registrarTransacao(Usuario *u, const char *descricao, float valor, const char *tipo);
void exibirExtrato(Usuario *u);

// ===== Funções =====
void clear_input(void){
    int c;
    while((c = getchar()) != '\n' && c != EOF);
}

// Cadastro de usuário
void cadastrarUsuario(Usuario *u) {
    printf("\n=== Cadastro de Usuário ===\n");
    printf("Nome: ");
    clear_input();
    scanf(" %[^\n]", u->nome);
    printf("CPF: ");
    scanf("%s", u->cpf);
    printf("Senha: ");
    scanf("%s", u->senha);

    // Inicializa contas
    u->banco.saldo = 0;
    u->investimento.saldo = 0;
    u->qtdTransacoes = 0;

    printf("Usuário %s cadastrado com sucesso!\n", u->nome);
}

// Login do usuário
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

// Registrar transação
void registrarTransacao(Usuario *u, const char *descricao, float valor, const char *tipo) {
    if (u->qtdTransacoes < MAX_TRANSACOES) {
        Transacao *t = &u->historico[u->qtdTransacoes++];
        strncpy(t->descricao, descricao, sizeof(t->descricao)-1);
        t->valor = valor;
        strncpy(t->tipo, tipo, sizeof(t->tipo)-1);
        time_t agora = time(NULL);
        struct tm *tm_info = localtime(&agora);
        strftime(t->data, sizeof(t->data), "%d/%m/%Y %H:%M", tm_info);
    }
}

// Exibir extrato
void exibirExtrato(Usuario *u) {
    printf("\n===== EXTRATO DE %s =====\n", u->nome);
    for(int i=0; i<u->qtdTransacoes; i++){
        printf("[%s] %-15s | %-30s | R$ %.2f\n",
               u->historico[i].data,
               u->historico[i].tipo,
               u->historico[i].descricao,
               u->historico[i].valor);
    }
    printf("Saldo Banco: R$ %.2f | Saldo Investimento: R$ %.2f\n", u->banco.saldo, u->investimento.saldo);
}

// Menu da Conta do Banco
void menuContaBanco(Usuario *u) {
    int opcao;
    do {
        printf("\n=== CONTA DO BANCO ===\n");
        printf("Olá %s, saldo atual: R$ %.2f\n", u->nome, u->banco.saldo);
        printf("1 - Depositar (PIX/TED)\n");
        printf("2 - Transferir para Investimentos\n");
        printf("3 - Transferir para outro Banco\n");
        printf("4 - Extrato\n");
        printf("5 - Voltar\n");
        printf("Escolha: ");
        scanf("%d", &opcao);

        switch (opcao)
        {
            case 1: depositarBanco(u); break;
            case 2: transferirParaInvestimento(u); break;
            case 3: transferirParaBancoExterno(u); break;
            case 4: exibirExtrato(u); break;
            case 5: printf("Voltando...\n"); break;
            default: printf("Opção inválida.\n");
        }

    } while(opcao != 5);
}

// Menu da Conta de Investimentos
void menuContaInvestimento(Usuario *u) {
    int opcao;
    do {
        printf("\n=== CONTA DE INVESTIMENTOS ===\n");
        printf("Olá %s, saldo atual: R$ %.2f\n", u->nome, u->investimento.saldo);
        printf("1 - Resgatar para Banco\n");
        printf("2 - Extrato\n");
        printf("3 - Voltar\n");
        printf("Escolha: ");
        scanf("%d", &opcao);

        switch (opcao)
        {
            case 1: transferirParaBanco(u); break;
            case 2: exibirExtrato(u); break;
            case 3: printf("Voltando...\n"); break;
            default: printf("Opção inválida.\n"); break;
        }

    } while(opcao != 3);
}

// Menu Principal
void menuPrincipal(Usuario *u) {
    int opcao;
    do {
        printf("\n=== MENU PRINCIPAL ===\n");
        printf("1 - Conta do Banco\n");
        printf("2 - Conta de Investimentos\n");
        printf("3 - Extrato Completo\n");
        printf("0 - Sair\n");
        printf("Escolha: ");
        scanf("%d", &opcao);

        switch(opcao) {
            case 1: menuContaBanco(u); break;
            case 2: menuContaInvestimento(u); break;
            case 3: exibirExtrato(u); break;
            case 0: printf("Saindo do sistema...\n"); break;
            default: printf("Opção inválida!\n");
        }
    } while(opcao != 0);
}

// Função de depósito (Banco)
void depositarBanco(Usuario *u) {
    int tipo;
    float valor, taxa = 0.0;

    printf("\n=== Depósito na Conta do Banco ===\n");
    printf("1 - PIX (sem taxa)\n");
    printf("2 - TED (c/ taxa de 1%%)\n");
    printf("Escolha: ");
    scanf("%d", &tipo);

    printf("Digite o valor do depósito: R$ ");
    scanf("%f", &valor);

    if (tipo == 2) {
        taxa = valor * 0.01; // 1% de taxa
    }

    u->banco.saldo += (valor - taxa);

    // Registrar no extrato
    if(tipo == 1)
        registrarTransacao(u, "Depósito PIX", valor, "PIX");
    else
        registrarTransacao(u, "Depósito TED", valor, "TED");

    printf("Depósito realizado com sucesso!\n");
    printf("Olá %s, saldo atual do banco: R$ %.2f\n", u->nome, u->banco.saldo);
}

// Transferências
void transferirParaInvestimento(Usuario *u) {
    float valor;
    printf("\n=== Transferência Banco -> Investimentos ===\n");
    printf("Saldo disponível no banco: R$ %.2f\n", u->banco.saldo);
    printf("Digite o valor para transferir: R$ ");
    scanf("%f", &valor);

    if (valor > 0 && valor <= u->banco.saldo) {
        u->banco.saldo -= valor;
        u->investimento.saldo += valor;
        registrarTransacao(u, "Transferência p/ Investimento", valor, "Transferência");
        printf("Transferência realizada com sucesso!\n");
        printf("Saldo banco: R$ %.2f | Saldo investimentos: R$ %.2f\n",
               u->banco.saldo, u->investimento.saldo);
    } else {
        printf("Saldo insuficiente ou valor inválido.\n");
    }
}

void transferirParaBanco(Usuario *u) {
    float valor;
    printf("\n=== Resgate Investimentos -> Banco ===\n");
    printf("Saldo disponível em investimentos: R$ %.2f\n", u->investimento.saldo);
    printf("Digite o valor para resgatar: R$ ");
    scanf("%f", &valor);

    if (valor > 0 && valor <= u->investimento.saldo) {
        u->investimento.saldo -= valor;
        u->banco.saldo += valor;
        registrarTransacao(u, "Resgate Investimento p/ Banco", valor, "Resgate");
        printf("Resgate realizado com sucesso!\n");
        printf("Saldo banco: R$ %.2f | Saldo investimentos: R$ %.2f\n",
               u->banco.saldo, u->investimento.saldo);
    } else {
        printf("Saldo insuficiente ou valor inválido.\n");
    }
}

void transferirParaBancoExterno(Usuario *u) {
    float valor;
    printf("\n=== Transferência Banco -> Externo ===\n");
    printf("Saldo disponível no banco: R$ %.2f\n", u->banco.saldo);
    printf("Digite o valor para transferir: R$ ");
    scanf("%f", &valor);

    if (valor > 0 && valor <= u->banco.saldo) {
        u->banco.saldo -= valor;
        registrarTransacao(u, "Transferência Banco Externo", valor, "TED");
        printf("Transferência para banco externo concluída!\n");
        printf("Saldo atual no banco: R$ %.2f\n", u->banco.saldo);
    } else {
        printf("Saldo insuficiente ou valor inválido.\n");
    }
}

// ===== Função principal =====
int main(void) {
    setlocale(LC_ALL, "Portuguese");
    Usuario usuario;
    int logado = 0;
    int opcao;

    while (!logado) {
        printf("\n=== Sistema Corretora ===\n");
        printf("1 - Cadastrar Usuário\n");
        printf("2 - Login\n");
        printf("0 - Sair\n");
        printf("Escolha: ");
        scanf("%d", &opcao);

        if (opcao == 1) {
            cadastrarUsuario(&usuario);
        } else if (opcao == 2) {
            logado = validarLogin(&usuario);
        } else if (opcao == 0){
            printf("Saindo do sistema...\n");
            return 0;
        } else{
            printf("Opção inválida.\n");
        }
    }

    menuPrincipal(&usuario);

    return 0;
}

#include <stdio.h>
#include <string.h>

#define MAX_EXTRATOS 100
#define MAX_ATIVOS 10

// Estrutura do Extrato
typedef struct {
    char descricao[100];
    float valor;
} Extrato;

// Estrutura do Ativo
typedef struct {
    char nome[50];
    float preco;
    int quantidade;
    int tipo; // 1 = Renda Variável, 2 = Renda Fixa
} Ativo;

// Variáveis globais
float saldo = 1000.0; // saldo inicial
Extrato extratos[MAX_EXTRATOS];
int qtdExtratos = 0;

Ativo ativos[MAX_ATIVOS] = {
    {"Sanepar (SAPR4)", 4.50, 0, 1},
    {"Banco do Brasil (BBAS3)", 30.00, 0, 1},
    {"Cemig (CMIG4)", 12.00, 0, 1},
    {"Itaú (ITUB4)", 25.00, 0, 1},
    {"HGLG11 (FII)", 150.00, 0, 1}
};
int qtdAtivos = 5;

// Função para registrar no extrato
void registrarExtrato(const char* descricao, float valor) {
    if (qtdExtratos < MAX_EXTRATOS) {
        strcpy(extratos[qtdExtratos].descricao, descricao);
        extratos[qtdExtratos].valor = valor;
        qtdExtratos++;
    }
}

// Função para mostrar saldo
void mostrarSaldo() {
    printf("\n=== Saldo Atual ===\n");
    printf("R$ %.2f\n", saldo);
}

// Função para mostrar extrato
void mostrarExtrato() {
    printf("\n=== Extrato ===\n");
    for (int i = 0; i < qtdExtratos; i++) {
        printf("%s | R$ %.2f\n", extratos[i].descricao, extratos[i].valor);
    }
}

// Função para comprar ativo
void comprarAtivo() {
    printf("\n=== Comprar Ativo ===\n");
    for (int i = 0; i < qtdAtivos; i++) {
        printf("%d - %s | Preço: R$ %.2f | Qtde: %d\n", i+1, ativos[i].nome, ativos[i].preco, ativos[i].quantidade);
    }

    int escolha, quantidade;
    printf("Escolha o ativo: ");
    scanf("%d", &escolha);
    escolha--;

    if (escolha >= 0 && escolha < qtdAtivos) {
        printf("Quantidade: ");
        scanf("%d", &quantidade);

        float custo = ativos[escolha].preco * quantidade;
        if (saldo >= custo) {
            saldo -= custo;
            ativos[escolha].quantidade += quantidade;

            char descricao[100];
            sprintf(descricao, "Compra %s (%d)", ativos[escolha].nome, quantidade);
            registrarExtrato(descricao, -custo);

            printf("Compra realizada!\n");
        } else {
            printf("Saldo insuficiente!\n");
        }
    } else {
        printf("Opção inválida!\n");
    }
}

// Função para vender ativo
void venderAtivo() {
    printf("\n=== Vender Ativo ===\n");
    for (int i = 0; i < qtdAtivos; i++) {
        printf("%d - %s | Preço: R$ %.2f | Qtde: %d\n", i+1, ativos[i].nome, ativos[i].preco, ativos[i].quantidade);
    }

    int escolha, quantidade;
    printf("Escolha o ativo: ");
    scanf("%d", &escolha);
    escolha--;

    if (escolha >= 0 && escolha < qtdAtivos) {
        printf("Quantidade: ");
        scanf("%d", &quantidade);

        if (quantidade <= ativos[escolha].quantidade) {
            float valorVenda = ativos[escolha].preco * quantidade;
            saldo += valorVenda;
            ativos[escolha].quantidade -= quantidade;

            char descricao[100];
            sprintf(descricao, "Venda %s (%d)", ativos[escolha].nome, quantidade);
            registrarExtrato(descricao, valorVenda);

            printf("Venda realizada!\n");
        } else {
            printf("Quantidade insuficiente!\n");
        }
    } else {
        printf("Opção inválida!\n");
    }
}

// Função para simular investimentos
void simularInvestimentos(int meses) {
    printf("\n=== Simulação de %d meses ===\n", meses);

    for (int m = 1; m <= meses; m++) {
        printf("\n--- Mês %d ---\n", m);

        for (int i = 0; i < qtdAtivos; i++) {
            if (ativos[i].quantidade > 0) {
                float rendimento = 0.0;

                if (strcmp(ativos[i].nome, "Sanepar (SAPR4)") == 0 && m % 12 == 0) {
                    rendimento = ativos[i].quantidade * 2.0;
                } else if (strcmp(ativos[i].nome, "Banco do Brasil (BBAS3)") == 0 && m % 3 == 0) {
                    rendimento = ativos[i].quantidade * 1.5;
                } else if (strcmp(ativos[i].nome, "Cemig (CMIG4)") == 0 && m % 6 == 0) {
                    rendimento = ativos[i].quantidade * 1.2;
                } else if (strcmp(ativos[i].nome, "Itaú (ITUB4)") == 0) {
                    rendimento = ativos[i].quantidade * 0.8;
                } else if (strcmp(ativos[i].nome, "HGLG11 (FII)") == 0) {
                    rendimento = ativos[i].quantidade * 1.5;
                }

                if (rendimento > 0) {
                    saldo += rendimento;

                    char descricao[100];
                    sprintf(descricao, "Dividendo %s - Mês %d", ativos[i].nome, m);
                    registrarExtrato(descricao, rendimento);

                    printf("Recebeu R$ %.2f de %s\n", rendimento, ativos[i].nome);
                }
            }
        }
    }
}

// Menu de investimentos
void menuInvestimentos() {
    int opcao;
    do {
        printf("\n=== Menu Investimentos ===\n");
        printf("1 - Comprar Ativo\n");
        printf("2 - Vender Ativo\n");
        printf("3 - Simular Investimentos\n");
        printf("0 - Voltar\n");
        printf("Escolha: ");
        scanf("%d", &opcao);

        if (opcao == 1) comprarAtivo();
        else if (opcao == 2) venderAtivo();
        else if (opcao == 3) {
            int meses;
            printf("Quantos meses deseja simular? ");
            scanf("%d", &meses);
            simularInvestimentos(meses);
        }
    } while (opcao != 0);
}

// Menu principal
int main() {
    int opcao;
    do {
        printf("\n=== Banco Stark 1.5 Build 700 ===\n");
        printf("1 - Mostrar Saldo\n");
        printf("2 - Extrato\n");
        printf("3 - Investimentos\n");
        printf("0 - Sair\n");
        printf("Escolha: ");
        scanf("%d", &opcao);

        if (opcao == 1) mostrarSaldo();
        else if (opcao == 2) mostrarExtrato();
        else if (opcao == 3) menuInvestimentos();
    } while (opcao != 0);

    return 0;
}

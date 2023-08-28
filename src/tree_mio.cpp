#include <ilcplex/ilocplex.h>
#include <vector>
#include <fstream>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <set>
#include <filesystem>
#include <time.h>

ILOSTLBEGIN

#define DEBUG 0

#define NOS_UTEIS 0
#define GALHOS 1
#define FOLHAS 2
#define ALTURA 3
#define ACERTOS 4
#define ERROS 5
#define N_TOTAIS ERROS+1

// variáveis de dados de execução
int totais[N_TOTAIS];

using namespace std;

int DISPLAY_OUTPUTS = 1;
string datasets = ""; //"/mnt/Dados/Home2019/Dropbox/Mestrado Computação UNIFEI/Disciplinas/Dissertação/Datasets/Iris/";
string fileName = datasets+"Iris.csv";
string fileNameTrainingData = "training-data-file.csv";
vector<string> problemClasses;


typedef IloArray<IloIntVarArray> IloIntVarArray2;
typedef IloArray<IloBoolVarArray> IloBoolVarArray2;

// Retorna o índice do filho esquerdo
int getLeftChild(int t) {
	return 2*t+1;
}

// Retorna o índice do filho direito
int getRightChild(int t) {
	return 2*t+2;
}

// Retorna o nó pai de um nó filho
int getParent(int t) {
	return ceil((double) t/2)-1;
}

// Encontrar uma string em um vector<string>
// Retorna o índice onde a string foi encontrada ou -1, caso contrário
int findStringInVector(string const _key, vector<string> list) {
	for(unsigned int i = 0; i < list.size(); i++) {
		if(_key.compare(list[i]) == 0) {
			return i;
		}
	}
	return -1;
}

/**
 * Imprime a árvore com indentações e
 * Calcula alguns somatórios como:
 * 	- Quantidade de nós válidos
 * 	- Altura da árvore
 */
void preOrder2(int nElements,
			int pAttributes,
			int kClasses,
			vector<vector<double>> x,
			int maxNos,
			int firstLeaf,
			IloBoolVarArray2 a,
			IloNumVarArray b,
			IloBoolVarArray2 c,
			IloBoolVarArray2 z,
			IloCplex cplex,
			fstream &treeSettingsStream,
			int currentNode = 0,
			int indent = 0,

			int indexNode = 0,
			int indexNodeParent = 0
			){

		// Inicialmente, presume-se que o nó corrente é um nó válido e, portanto o índice dos seus nós filhos é calculado como:
		int indexLeftChild = getLeftChild(indexNode); // 2 x indexNode + 1
		int indexRightChild = getRightChild(indexNode); // 2 x IndexNode + 2

		// Verifica se o nó t está dentro do conjunto de nós da árvore (0,...,n)
		if(currentNode >= maxNos) {
			return;
		}

		// Adicionando detalhes do nó (se galho ou folha)
		string kindNode = "galho";
		int classNode = -1;
		int param = -1;
		double bound = -1;
		string separator = " ";

		// Se for nó folha
		if(currentNode >= firstLeaf && currentNode < maxNos) {
			kindNode = "folha";

			// Obter a classe atribuída ao nó folha
			for(int k = 0; k < kClasses; k++) {
				if(cplex.getValue(c[k][currentNode] >= 0.5)) {
					classNode = k;
					break;
				}
			}
		}// Se for nó galho
		else if(currentNode >= 0 && currentNode < firstLeaf) {
			// Obtendo o parâmetro no qual o nó galho corrente irá se dividir
			for(int p = 0; p < pAttributes; p++) {
				if(cplex.getValue(a[p][currentNode])) {
					param = p;
				}
			}
			bound = cplex.getValue(b[currentNode]);
		}


		// Se for um nó válido, ou seja, nó galho ou nó classe
		if(param != -1 || classNode != -1) {
			if(DISPLAY_OUTPUTS) { cout << endl; }

			//
			for(int i = 0; i < indent; i++) {
				if(DISPLAY_OUTPUTS) { cout << "  "; }
			}

			// Contando total de nós úteis
			totais[NOS_UTEIS]++;

			// Imprime o nó
			if(DISPLAY_OUTPUTS) { cout << "(Nó: " << indexNode << "\tTipo: " << kindNode << "\t"; }
			treeSettingsStream << "(" << separator << indexNode << separator << kindNode << separator;

			if(kindNode.compare("galho") == 0) {
				if(DISPLAY_OUTPUTS) { cout << "param. " << param << "\tLimiar: " << setprecision(7) << bound; }
				treeSettingsStream << param << separator << setprecision(7) << bound << separator;
				totais[GALHOS]++;
			}
			else {
				if(DISPLAY_OUTPUTS) { cout << "Classe: " << setprecision(0) << classNode << "_" << problemClasses[classNode]; }
				treeSettingsStream << setprecision(0) << classNode << separator;
				totais[FOLHAS]++;
			}

			// Se é um nó válido, é um nó pai válido, portanto atualiza-se o indice do nó pai para os próximos que será o nó corrente.
			indexNodeParent = indexNode;
		}
		// o else abaixo é apenas para corrigir a indentação ao imprimir a árvore em notação de parênteses
		// ao terminar o treinamento
		else {
			// Se o nó corrente é inválido, não há porque adicionar um nível de indentação
			indent--;

			// Se o nó corrente é um nó inválido, significa que o próximo nó válido é uma folha.
			// Resta saber se o próximo nó válido será o filho esquerdo ou direito do nó pai.
			// É possível calcular isso baseado no valor do índice corrente.
			// Se o nó corrente é ímpar, o nó corrente é o filho esquerdo.
			// Contudo, o passado no passo recursivo será o mesmo, pois não importa se a folha está à esquerda ou à direita de
			// um nó pai inválido, importa apenas se a folha está a esquerda ou à direita do último nó pai válido.
			// Se esta condição já está sendo tratada, é porque a verficação já está no nível da árvore abaixo do último nó pai válido e,
			// neste caso, só haverá mais um nó válido do lado esquerdo e mais um nó válido do lado direito, mas ambos são enxergados
			// separadamente no processo recursivo.
			// Quando o nó corrente for o filho esquerdo, o passo recursivo para o filho direito não terá efeitos e vice-versa.
			indexLeftChild = indexRightChild = (indexNode % 2 == 0) ? getRightChild(indexNodeParent) : getLeftChild(indexNodeParent);
			//APAGAR: Jeito errado que estava anteriormente
			//APAGAR: indexLeftChild = getLeftChild(indexNodeParent);
			//APAGAR: indexRightChild = getRightChild(indexNodeParent);

			// Se o nó corrente é inválido, o nó pai continuará sendo o nó pai válido ocorrido no processo.
			indexNodeParent = getParent(indexNode);
		}

		// Registrando a maior altura alcançada da árvore
		if(indent > totais[ALTURA]) {
			totais[ALTURA] = indent;
		}

		// Vai para o filho esquerdo
		preOrder2(nElements, pAttributes, kClasses, x, maxNos, firstLeaf, a, b, c, z, cplex, treeSettingsStream, getLeftChild(currentNode), indent+1, indexLeftChild, indexNodeParent);
		// Vai para o filho direito
		preOrder2(nElements, pAttributes, kClasses, x, maxNos, firstLeaf, a, b, c, z, cplex, treeSettingsStream, getRightChild(currentNode), indent+1, indexRightChild, indexNodeParent);

		if(param == -1 && classNode == -1) {
			return;
		}

		if(kindNode.compare("galho") == 0) {
			if(DISPLAY_OUTPUTS) { cout << endl; }
			for(int i = 0; i < indent; i++) {
				if(DISPLAY_OUTPUTS) { cout << "  "; }
			}
		}

		if(DISPLAY_OUTPUTS) { cout << ")"; }
		treeSettingsStream << ")" << separator;

}



/**
 * A cada execução, este método é chamado para escrever os dados de execução em um arquivo de
 * histórico de execução para fins de análise e estatística.
 */
int saveTrainingDataStart(
		int printOutputs,
		string datasetName,
		int nElements,
		int nAttributes,
		int nClasses,
		string classesInLine,
		int	maxTreeHeigth,
		double alpha,
		int minLeafElements,
		int timeLimit) {

	// Abrindo o arquivo e definindo o separador CSV
	fstream dataTrainingFile;

	//dataTrainingFile.open(fileNameTrainingData, fstream::out);
	//dataTrainingFile << "texto";
	//dataTrainingFile.close();
	//return 1;

	//dataTrainingFile.close();
	dataTrainingFile.open(fileNameTrainingData, fstream::app);
	if(!dataTrainingFile.is_open()) {
		cerr << endl << "Erro ao abrir o arquivo \"" << fileNameTrainingData << "\"" << endl << endl;
		return 0;
	}
	if(DISPLAY_OUTPUTS) { cout << endl << "Arquivo " << fileNameTrainingData << " aberto com sucesso." << endl << endl;}
	string SEP = ",";
	string line = "";

	// Se o arquivo estiver vazio, criar a primeira linha com os cabeçalhos das colunas
	if(dataTrainingFile.tellg() == 0) {
		line = "printOutputs"+SEP+"datasetName"+SEP+"nElements"+SEP+"nAttributes"+SEP+
				"nClasses"+SEP+"classesInLine"+SEP+"maxTreeHeigth"+SEP+"alpha"+SEP+
				"minLeafElements"+SEP+"timeLimit"+SEP+"totalElementos"+SEP+"totalGalhos"+SEP+
				"totalFolhas"+SEP+"alturaReal"+SEP+"totalErros"+SEP+"totalAcertos"+SEP+"spentTime";

		if(DISPLAY_OUTPUTS) { cout << line << endl;}
		dataTrainingFile << line;
	}

	// Gravando os dados de início da execução.
	line = std::to_string(printOutputs)+SEP+datasetName+SEP+std::to_string(nElements)+SEP+
			std::to_string(nAttributes)+SEP+std::to_string(nClasses)+SEP+"\""+classesInLine+"\""+SEP+
			std::to_string(maxTreeHeigth)+SEP+std::to_string(alpha)+SEP+std::to_string(minLeafElements)+SEP+
			std::to_string(timeLimit);
	if(DISPLAY_OUTPUTS) { cout << line << endl << endl;}
	dataTrainingFile << endl << line;

	dataTrainingFile.close();
	return 1;
}

/**
 * Ao fim de cada execução, este método é chamado para escrever os dados de execução em um arquivo de
 * histórico de execução para fins de análise e estatística.
 */
int saveTrainingDataEnd(
		int totalElementos,
		int totalGalhos,
		int totalFolhas,
		int alturaReal,
		int totalErros,
		int totalAcertos,
		double spentTime) {

	// Abrindo o arquivo e definindo o separador CSV
	fstream dataTrainingFile;

	//dataTrainingFile.close();
	dataTrainingFile.open(fileNameTrainingData, fstream::app);
	if(!dataTrainingFile.is_open()) {
		cerr << endl << "Erro ao abrir o arquivo \"" << fileNameTrainingData << "\"" << endl << endl;
		return 0;
	}
	if(DISPLAY_OUTPUTS) { cout << endl << "Arquivo " << fileNameTrainingData << " aberto com sucesso." << endl << endl;}
	string SEP = ",";
	string line = "";

	// Gravando os dados de fim da execução.
	line = SEP+std::to_string(totalElementos)+SEP+std::to_string(totalGalhos)+SEP+
			std::to_string(totalFolhas)+SEP+std::to_string(alturaReal)+SEP+
			std::to_string(totalErros)+SEP+std::to_string(totalAcertos)+SEP+
			std::to_string(spentTime);

	if(DISPLAY_OUTPUTS) { cout << line << endl << endl;}
	dataTrainingFile << line;

	dataTrainingFile.close();
	return 1;
}


/**
 * Imprime o menu e obtem do usuário se pode-se continuar a execução ou não.
 */
int printMenu(int argc, char** argv) {
	// Recebe o nome do arquivo por parâmetro quando é passado
	if(argc > 1) {
		DISPLAY_OUTPUTS = atoi(argv[1]);
	}

	if(DISPLAY_OUTPUTS) {
		if(argc == 1) {
			cout << "Você também pode passar os seguintes parametros:" << endl;
			cout << "[1] Mostrar saídas do programa (1 - Sim, 0 - não) " << endl;
			cout << "[2] arquivo do dataset (padrão: Iris.csv)" << endl;
			cout << "[3] N. atributos (padrão: 4)" << endl;
			cout << "[4] N. classes (padrão: 3) " << endl;
			cout << "[5] Classes separadas por vírgula ('Iris-setosa,Iris-versicolor,Iris-virginica')" << endl;
			cout << "[6] altura (padrao: 2)" << endl;
			cout << "[7] alpha (padrao: 0.3)" << endl;
			cout << "[8] Min. elementos na folha (padrao: 2)" << endl;
			cout << "[9] Tempo max. exec.(padrao: 180 s)" << endl;
			cout << endl;
			cout << "Deseja continuar mesmo assim? [S ou n]: ";
			char continuar;
			cin >> continuar;
			if(continuar != 's' && continuar != 'S') {
				return 0;
			}
		}
	}
	return 1;
}


/**
 *
 */
int main(int argc, char** argv){

	// Inicializando totais[]
	for(int i = 0; i < N_TOTAIS; i++) {
		totais[i] = 0;
	}

	// Calculando o tempo gasto
	clock_t startTime = clock();
	clock_t endTime = startTime;


	int _continue = printMenu(argc, argv);
	if(!_continue) {
		return 0;
	}


	/**
	 * Criar as principais variáveis do problema e define-as conforme os
	 * parâmetros recebidos pela função main()
	 */
	int p;					// número de parâmetros do problema
	int K;					// número de classes do problema
	string classesInLine;	// todas as classes do problema separadas por vígula
	int altura;				// altura máxima da árvore
	double alpha;			// alpha: valor que equilibra acurácia x complexidade
	int nMin;				// número mínimo de elementos em um nó folha
	int maxTime;			// tempo máximo de execução

	if(DISPLAY_OUTPUTS) { cout << "Argumentos do programa:" << endl; }
	for(int i = 0; i < argc; i++) {
		if(DISPLAY_OUTPUTS) { cout << i << ": " << argv[i] << endl; }
	}
	if(DISPLAY_OUTPUTS) { cout << endl; }

	// Recebendo atributos do problema por parâmetro

	// Recebe o nome do arquivo por parâmetro quando é passado
	if(argc > 1) {
		if(DISPLAY_OUTPUTS) { cout << "1 - Mostrar saídas:\t" << ((DISPLAY_OUTPUTS) ? "Sim" : "Não" ) << endl; }
	}

	// Recebe o nome do arquivo por parâmetro quando é passado
	if(argc > 2) {
		fileName = string(argv[2]);
		if(DISPLAY_OUTPUTS) { cout << "2 - fileName:\t" << fileName << endl; }
	}

	// Recebe o número de atributos do dataset
	p  = (argc > 3) ? atoi(argv[3]) : 4;
	if(DISPLAY_OUTPUTS) { cout << "Num. de atributos do problema:\t" << p << endl; }

	// Recebe o número de clases do dataset
	K  = (argc > 4) ? atoi(argv[4]) : 3;
	if(DISPLAY_OUTPUTS) { cout << "Num. de classes do problema:\t" << K << endl; }

	// Recebe a lista de classes possíveis para o problema
	classesInLine = (argc > 5) ? string(argv[5]) : "Iris-setosa,Iris-versicolor,Iris-virginica";
	if(DISPLAY_OUTPUTS) { cout << "5 - Classes separadas por vírgula:\t" << classesInLine << endl; }

	// Recebe a altura da árvore por parâmetro quando é passada
	altura = (argc > 6) ? atoi(argv[6]) : 2;
	if(DISPLAY_OUTPUTS) { cout << "altura:\t" << altura << endl; }

	// Recebe o alpha por parâmetro quando é passado
	char* ptrdouble;
	alpha = (argc > 7) ? strtod(argv[7], &ptrdouble) : 0.3;
	if(DISPLAY_OUTPUTS) { cout << "alpha:\t" << alpha << endl; }

	// Recebe a quantidade mínima de cad folha quando é passada
	nMin  = (argc > 8) ? atoi(argv[8]) : 2;
	if(DISPLAY_OUTPUTS) { cout << "Min. elementos na folha:\t" << nMin << endl; }

	// Recebe o tempo máximo para execução do programa em segundos
	maxTime  = (argc > 9) ? atoi(argv[9]) : 2*60;
	if(DISPLAY_OUTPUTS) { cout << "Tempo max. exec.:\t" << maxTime << " sec." << endl; }



	// Gravando dados de execução do programa
	saveTrainingDataStart(DISPLAY_OUTPUTS,fileName,-1,p,K,classesInLine,altura,alpha,nMin,maxTime);


	// CARREGANDO DADOS
	vector < vector < double > > x;
	fstream arquivo;
	arquivo.open(fileName, fstream::in);
	/* Testando leitura de arquivo */
	string aux;
	string line;
	string word;
	const char SEPARATOR = ',';


	// Criando um dicionário de classes
	stringstream _lineClasses(classesInLine);
	string _class;

	while(getline(_lineClasses, _class, ',')) {
		problemClasses.push_back(_class);
		_class.clear();
	}

	if(DISPLAY_OUTPUTS) {
		for(unsigned int i = 0; i < problemClasses.size(); i++) {
			cout << problemClasses[i] << ",\t";
		}
		cout << endl;
	}



	// Descartando a primeira linha contendo as legendas
	arquivo >> aux;


	// Lendo os dados do arquivo
	string cl;
	int n; // = 150; // Total de elementos
	// int p = 4; // Quantidade de atributos
	// A quantidade de classes será definida durante a leitura do arquivo
	//int K; // = 3; // Quantidade de classes
	int	somaClasses[K]; // Quantidade de elementos associados a cada classe K
	double minValueAttribute = 0; // Valor mínimo de um atributo
	double maxValueAttribute = 0; // Valor máximo de um atributo


	// LENDO ARQUIVO CSV SEM ESPAÇAMENTO DEPOIS DA VÍRGULA
	if(!arquivo.is_open()) {
		if(DISPLAY_OUTPUTS) { cout << "O arquivo não pode ser aberto." << endl; }
		return 0;
	}

	// A contagem de classes será feita em uma estrutura de map (chave-valor)
	// já durante a leitura do arquivo
	set<string> setClasses;


	// Zerando a quantidade de elementos inseridos.
	n = 0;
	getline(arquivo, line); // descartando a primeira linha
	for(int i = 0; !arquivo.eof(); i++) {
		//x[i].resize(p+1);
		vector<double> attributes(p);

		// Lê uma linha do arquivo
		line.clear();
		getline(arquivo, line);

		// Se encontrar uma linha em branco, seja no meio ou no fim do arquivo, continua a na próxima linha
		// Até encontrar o fim do arquivo.
		if(!line.compare("")) {
			continue;
		}

		//transforma a string num fluxo de dados (stream)
		stringstream str(line);
		word.clear();
		int j = 0;
		while(getline(str, word, SEPARATOR)) {
			// Converte apenas os atributos numericos que estã nos índices 1,2,3,4;
			if(j > 0 && j <= p) {
				// Ao copiar para o novo vetor e descartar o ID, o dados vão de line[j] -> attributes[j-1].
				attributes[j-1] = stod(word);
			}
			// PAttributes+1 é o índice onde está a classe
			else if(j == p+1) {
				// DEPRECATED HARDCODE: Convertendo a classe de string para double
				//attributes.push_back(word.compare("Iris-setosa") == 0 ? 0 : (word.compare("Iris-versicolor") == 0 ? 1 : 2));


				// SOFTCODE:
				// Adiciona todos os valores de classes a um conjunto, desta forma
				// será possível tê-los associados à um índice
				//setClasses.insert(word);

				// Obtem o índice equivalente posição que a classe foi inserida na estrutura set
				//unsigned int index = std::distance(setClasses.begin(), setClasses.find(word));

				// Obtem o índice equivalente posição que a classe foi inserida na estrutura set
				unsigned int index = findStringInVector(word, problemClasses);

				// Se não encontra a classe no vetor, então ocorreu um erro
				unsigned int last = problemClasses.size()-1;
				if(index < 0 || index > last) {
					cerr << "Word: " << word << endl;
					cerr << "Indice da classe: " << index << endl;
					cerr << "Houve um erro ao tentar encontrar a classe no vetor: findStringInVector(word, problemClasses)" << endl;
					break;
				}

				// Associa o elemento à classe representada aqui equivalentemente pelo seu índice
				attributes.push_back(index);
			}

			word.clear();
			j++;
		}

		// Colocando os atributos no índice do elemento
		x.push_back(attributes);

		// Contando a quantidade de elementos inseridos
		n++;
	}

	// FIM DA LEITURA DO ARQUIVO CSV COM ESPAÇAMENTO DEPOIS DA VÍRGULA

	// Apresentando quantas classes foram encontradas para o problema
	if(DISPLAY_OUTPUTS) { cout << "Num. de classes encontradas:\t" << setClasses.size() << endl; }

	if(DISPLAY_OUTPUTS) { cout << "n:\t" << n << endl; }

	if(DISPLAY_OUTPUTS) { cout << "Matriz de dados original sem o ID" << endl; }
	for(unsigned int i = 0; i < x.size(); i++) {
		if(DISPLAY_OUTPUTS) { cout << i << "\t"; }
		for(unsigned int j = 0; j < x[i].size(); j++) {
			if(DISPLAY_OUTPUTS) { cout << x[i][j] << "\t"; }
		}
		if(DISPLAY_OUTPUTS) { cout << endl; }
	}
	// FIM DA LEITURA DO ARQUIVO CSV SEM ESPAÇAMENTO DEPOIS DA VÍRGULA


	//
	/*

	// ### Normalizando os dados ###
	double menorValorColuna = 0;
	double maiorValorColuna = 0;
	double amplitudeColuna = 0;
	double xNormalizado[n][p+1];

	// Para cada coluna:
	for(int j = 0; j < p; j++) {
		// Inicializando variáveis
		menorValorColuna = 0;
		maiorValorColuna = 0;

		// Para cada linha:
		// O primeiro valor é inicialmente o maior e o menor valor da coluna
		menorValorColuna = x[0][j];
		maiorValorColuna = x[0][j];

		for(int i = 0; i < n; i ++) {
			// Obter o menor valor
			if(x[i][j] < menorValorColuna) { menorValorColuna =  x[i][j]; }
			// Obter o maior valor
			if(x[i][j] > maiorValorColuna) { maiorValorColuna =  x[i][j]; }
		}

		// Calcular a amplitude
		amplitudeColuna = maiorValorColuna - menorValorColuna;
		// Normalizar a linha i calculando (xi - médiaDeX) / ampliturdeDeX
		for(int i = 0; i < n; i++){
			xNormalizado[i][j] = (x[i][j] - menorValorColuna) / amplitudeColuna;
		}
	}

	// Imprimindo os valores normalizados
	cout << "--- Valores Normalizados ---" << endl;
	std::cout << std::fixed;
	std::cout << std::setprecision(2);
	for(int i = 0; i < n; i ++) {
		for(int j = 0; j < p; j++) {
			x[i][j] = xNormalizado[i][j];
			cout << std::setprecision(7) << x[i][j] << "\t";
		}
		cout << std::setprecision(1) << x[i][p] << "\t";
		cout << endl;
	}

	// */


	IloEnv env;
	IloModel model(env);
	// Se for para ocultar saídas do programa, ocultar saídas do CPLEX também.
	if(!DISPLAY_OUTPUTS) { env.setOut(env.getNullStream()); }

	/*** VARIÁVEIS DO PROBLEMA ***/
	// Variaveis do problema
	//int altura = 2;
	// altura = 2;
	// máximo de nós da árvore
	int maxNos = pow(2,(altura+1))-1;
	// Primeira Folha
	int firstLeaf = floor(maxNos/2);
	// Total de nós branches
	int totalBranches = firstLeaf;
	// Total de nós folha
	// int totalLeaf = maxNos - totalBranches;
	// Mínimo de elementos que uma folha pode ter, caso ela não seja vazia.
	//int nMin = 2;



	if(DISPLAY_OUTPUTS) { cout << "Altura: " << altura << endl; }
	if(DISPLAY_OUTPUTS) { cout << "maxNos: " << maxNos << endl; }
	if(DISPLAY_OUTPUTS) { cout << "Primeira folha: " << firstLeaf << endl; }


	// d[i] = 1 se o nó i for branch; 0 se for nó folha
	IloBoolVarArray d(env, totalBranches);

	// a[j][t] = 1 se o parâmetro j é considerado no nó t, 0 se não.
	IloBoolVarArray2 a(env, p);

	// Criando a variável a
	for(int j = 0; j < p; j++){
		a[j] = IloBoolVarArray(env, totalBranches);
	}

	// Valores de separação dos nós galho
	// 1. Encontrar o valor mínimo e máximo de um atributo
	// Para isso inicía-se a busca pelo valor do primeiro atributo do primeiro objeto
	// x[,[attr_0]..,[attr_n], [_class]] // Não, não tem index nessa matriz ainda.
	if(x.size() > 0 && x[0].size() > 0) {
		minValueAttribute = maxValueAttribute = x[0][0];
	}

	// Para todos os elementos do dataset
	for(unsigned int i = 0; i < x.size(); i++) {
		// Para todos os atributos do dataset, ou seja, exceto a coluna 1 (index) e a n (classe)
		for(unsigned int j = 0; j < x[0].size()-1; j++) {
			// Se o valor corrente for menor que o menor já encontrado, atualiza-se o menor
			if(x[i][j] < minValueAttribute) {
				minValueAttribute = x[i][j];
			}
			// Se o valor corrente for maior que o maior já encontrado, atualiza-se o maior
			else if (x[i][j] > maxValueAttribute) {
				maxValueAttribute = x[i][j];
			}
		}
	}


	// Calculando a amplitude dos valores dos atributos
	double M = maxValueAttribute - minValueAttribute;

	// Imprimindo valores mínimo e máximo dos atributos
	if(DISPLAY_OUTPUTS) {
		cout << "Valor mínimo:\t" << minValueAttribute << endl;
		cout << "Valor máximo:\t" << maxValueAttribute << endl;
		cout << endl;
	}

	// 2. Definir uma variável para armazenar o valor de separação limitado pelo
	// valor mínimo e máximo de um atributo.
	IloNumVarArray b(env, totalBranches);
	for(int i = 0; i < totalBranches; i++) {
		// b[i] = IloNumVar(env, 0, 1);

		// Quando um nó não faz divisão, ele precisa mandar todos elementos para a direita, para isso
		// o valor do limiar (b[i]) tem que ser menor que o mínimo para permitir que todos os valores
		// sejam maior que o limiar.
		//b[i] = IloNumVar(env, minValueAttribute, maxValueAttribute);
		b[i] = IloNumVar(env, minValueAttribute, maxValueAttribute);
	}

	// z[i][t] = 1, se o elemento i está no nó folha t, 0 se não.
	IloBoolVarArray2 z(env, n); // A gente só vai usar o z das folhas.

	// Criando a variável z
	for(int i = 0; i < n; i++){
		z[i] = IloBoolVarArray(env, maxNos);
	}

	// l[t] = 1, se a folha t tem elementos, 0 caso contrário
	IloBoolVarArray l(env, maxNos); // A gente só vai usar o z das folhas.

	// N[k][t] = é o número de elementos da classe k no nó folha t
	IloIntVarArray2 N(env, K);
	for(int k = 0; k < K; k++) {
		N[k] = IloIntVarArray(env, maxNos, 0, n);
	}

	// c[k][t] = 1, se a folha t é classificada como classe k, 0 caso contrário
	IloBoolVarArray2 c(env, K);
	// Para toda classe k
	for(int k = 0; k < K; k++) {
		c[k] = IloBoolVarArray(env, maxNos);
	}

	// L[t] = Quarda quantos erros houveram na folha 't'
	IloIntVarArray L(env, maxNos, 0, n);



	/*** RESTRIÇÕES DO PROBLEMA ***/

	// Restrição 2: Garante que se um nó faz alguma divisão, então ele divide apenas um atributo
	// Para todos os nós t
	for(int t = 0; t < totalBranches; t++) {
		IloExpr somaAtributosDivididosNesteNo(env);
		// Para todos os atributos
		for(int j = 0; j < p; j++) {
			somaAtributosDivididosNesteNo += a[j][t];
		}
		model.add(somaAtributosDivididosNesteNo == d[t]);
	}


	// Restrição 3: Garante que se um nó não faz separação,
	// consequentemente o limiar dele também é /*zero*/ o menor possível,
	// fazendo todos os filhos irem para a direita.
	// Para todos os nós galho
	for(int t = 0; t < totalBranches; t++) {
		// Antes (Normalizado): model.add(b[t] <= d[t]);
		// Não normalizado:

		model.add(b[t] <= minValueAttribute + (d[t]*(M)));
	}


	// Se um nó é branch, o pai dele também é branch
	// _Todo d(t) tem que ser <= que d(getParent(t))

	// Restrição para o nó raiz como nó galho, ou seja, d[0] == 1
	model.add(d[0] == 1);
	// Restrição 5: para os demais nós.
	for(int t = 1; t < totalBranches;t++) {
		if(DISPLAY_OUTPUTS) {
			cout << "t: " << t << endl;
			cout << "getparent(t): " << getParent(t) << endl;
		}
		model.add(d[t] <= d[getParent(t)]); // Trecho corrigido do artigo
	}

	// Restrição 6: de que os elementos só estão em nós folhas
	// Percorrendo os elementos
	for(int i = 0; i < n; i++) {
		// Percorrendo os nós
		for(int t = firstLeaf; t < maxNos; t++) {
			model.add(z[i][t] <= l[t]);
		}
	}

	// Restrição 7: garante que_todo nó folha tem pelo menos nMin elementos ou então é vazio.
	for(int t = firstLeaf; t < maxNos; t++) {
		IloExpr somaElementosDaFolha(env);
		// Para todos os elementos
		for(int i = 0; i < n; i++) {
			somaElementosDaFolha += z[i][t];
		}
		model.add(somaElementosDaFolha >= nMin*l[t]);
	}


	// Restrição 8: de que _todo elemento obrigatoriamente tem que estar em uma folha
	// Para _todo elemento
	for(int i = 0; i < n; i++) {
		IloExpr somaElementoDasFolhas(env);
		for(int t = firstLeaf; t < maxNos; t++) {
			somaElementoDasFolhas += z[i][t];
		}
		model.add(somaElementoDasFolhas == 1);
	}

	// Encontrando o epsilon, que é a menor diferença entre dois atributos que não for zero
	// Para_todo elemento
	//double epsilon = 999999999999; // valor suficientemente grande para forçar haver uma diferença entre 2 atributos que seja menor que ele.
	vector<double> epsilon(p);
	for(int j = 0; j < p; j++) {
		epsilon[j] = M; // espsilon começa com M, pois M é a maior diferença entre todos os valores de atributos
	}

	//for(int i = 0; i < n; i++) {
	for(int i = 0; i < n-1; i++) {
		// Para_todos os pares de elementos
		for(int h = i+1; h < n; h++) {
			// Para _todo atributos
			for(int j = 0; j < p; j++) {
				double difference = abs(x[i][j]-x[h][j]);
				// Não atualizar o valor se a diferença entre dois atributos for zero.
				if(difference >= 0.00000000001) {
					epsilon[j] = (epsilon[j] > difference) ? difference : epsilon[j];
				}
			}
		}
	}


	// Imprimindo cada epsilon encontrado para cada atributo
	if(DISPLAY_OUTPUTS) {
		cout.precision(10);
		for(int j = 0; j < p; j++) {
			cout << "Epsilon[" << j << "]: " << epsilon[j] << endl;
		}
	}


	// Restrição 11: de que se um elemento pertence à um nó t, ele não pode ter sido
	// desviado (na árvore) por um ancestral de t.
	// Em outras palavras, o elemento precisa ter passado por todos os ancestrais de t
	// Para _todo  nó interno (de 0 até totalBranches)
	for(int t = firstLeaf; t < maxNos; t++) {
		int pai = getParent(t);

		int noAtual = t;
		while(pai >= 0) {
			// Se o noAtual é par, o noAtual é o filho direito do pai dele.
			if((noAtual%2) == 0) {
				//Aplica a restrição 10: a^T_m(x_i) ≥ b_t - M (1 − z_{it}), i = 1,…, n
				// Para cada elemento
				for(int i = 0; i < n; i++) {
					// Para cada parâmetro
					IloExpr produtoEscalar(env);
					produtoEscalar += minValueAttribute;
					for(int j = 0; j < p; j++) {
						//produtoEscalar += a[j][pai] * x[i][j]; Com erro
						produtoEscalar += a[j][pai] * (x[i][j] - minValueAttribute);
					}

					//cerr << produtoEscalar <<  " >= " << "b["<<pai<<"] " << " -2*" << "(1-z[" << i << "][" << t << "])" << endl;
					//cerr << produtoEscalar <<  " >= " << b[pai] << " -2*" << (1-z[i][t]) << endl;
					// (M+1), onde M é a amplitude e, portanto, (M+1) é um valor grande o suficiente para evitar os extremos

					// TODO Verificar se a inequação abaixo funciona em todos os casos
					model.add(produtoEscalar >= b[pai]-((M+1)*(1-z[i][t]))); // M = amplitude absoluta
					//cerr << "Adicionado com sucesso." << endl;

					// Planilhoa com testes:
					// https://docs.google.com/spreadsheets/d/16nKGRQPiKy8YZpNOorGnaPYsXC4cGxLxQgnHBbiKp3Q/edit#gid=0
				}
			}
			else { // Se o nó atual é ímpar
				// Aplicar Restrição 11: que
				for(int i = 0; i < n; i++) {
					// Guardará qual o atributo está sendo dividido no nó corrente
					IloExpr epsilonCurr(env);

					// Para cada parâmetro
					IloExpr produtoEscalar(env);
					produtoEscalar += minValueAttribute;
					for(int j = 0; j < p; j++) {
						produtoEscalar += a[j][pai] * (x[i][j] - minValueAttribute);

						// Obtendo o epsilon de qual atributo está sendo usado para fazer a divisão do nó
						epsilonCurr += a[j][pai] * epsilon[j];
					}
					// (M+1), onde M é a amplitude e, portanto, (M+1) é um valor grande o suficiente para evitar os extremos
					//model.add(produtoEscalar+epsilon <= b[pai]+(M+1)*(1-z[i][t])); // M = amplitude absoluta

					// TODO Verificar se funciona em todos os casos
					// TODO trazer os epsilons
					// Computer on the beach
					model.add(produtoEscalar+0.001 <= b[pai]+(M+1)*(1-z[i][t])); // M = amplitude absoluta
				}
			}


			//cerr << pai << ", ";
			noAtual = pai;
			pai = getParent(pai);
			//cerr << "pai: " << pai << endl;
		}
		//cerr << "]" << endl;
	}

	// Restrição 15: Garante que N[k][f] tenha o número de elementos da classe k na folha f
	// Para todos os elementos folha
	for(int f = firstLeaf; f < maxNos; f++) {
		// Olhar todas as classes
		for(int k = 0; k < K; k++) {
			IloExpr totalNosDaClasseKEmCadaFolha(env);
			// Para todos os elementos
			for(int e = 0; e < n; e++) {
				// Verificar se o elemento 'e' pertence a classe 'k'
				if((int) x[e][p] == k) {
					totalNosDaClasseKEmCadaFolha += z[e][f];
				}
			}
			model.add(N[k][f] == totalNosDaClasseKEmCadaFolha);
		}
	}

	// Restrição 16: Gerante que a variavel totalNosEmCadaFolha armazene o número total de nós de cada folha (totalElementosDeUmaFolha)
	// Para todos os elementos
	IloIntVarArray totalElementosEmCadaFolha(env, maxNos, 0, n);
	for(int f = firstLeaf; f < maxNos; f++) {
		IloExpr totalElementosDeUmaFolha(env);
		for(int e = 0; e < n; e++) {
			totalElementosDeUmaFolha += z[e][f];
		}
		model.add(totalElementosEmCadaFolha[f] == totalElementosDeUmaFolha);
	}

	// Restrição 18: Garante que cada folha, se for utilizada, tenha uma classe
	for(int f = firstLeaf; f < maxNos; f++) {
		IloExpr somaDasClassesDeUmaFolha(env);
		for(int k = 0; k < K; k++) {
			somaDasClassesDeUmaFolha += c[k][f];
		}
		model.add(somaDasClassesDeUmaFolha == l[f]);
	}


	// Restrição 20: Garante que, dada uma classe k como rótulo da folha,
	// soma-se os erros de atribuição de elementos nesta classe.
	// Para toda folha
	for(int f = firstLeaf; f < maxNos; f++) {
		// para toda classe
		for(int k = 0; k < K; k++) {
			model.add(L[f] >= (totalElementosEmCadaFolha[f]-N[k][f])-n*(1-c[k][f]));
			model.add(L[f] <= (totalElementosEmCadaFolha[f]-N[k][f])+n*(c[k][f]));
		}
	}

	// Inicializando o vetor de contagem de elementos de cada classe
	for(int k = 0; k < K; k++) {
		somaClasses[k] = 0;
	}
	// Obtendo o número de elementos de cada classe
	for(unsigned int i = 0; i < x.size(); i++) {
		somaClasses[(int) x[i][p]]++;
	}
	// Obtendo a quantidade de elementos da classe de maior proporção
	unsigned int quantidadeElementosMaiorClasse = 0;
	for(int k = 0; k < K; k++) {
		if(somaClasses[k] > quantidadeElementosMaiorClasse) {
			quantidadeElementosMaiorClasse = somaClasses[k];
		}
	}

	if(DISPLAY_OUTPUTS) { cout << "quantidadeElementosMaiorClasse: " << quantidadeElementosMaiorClasse << endl; }

	// FUNÇÃO OBJETIVO
	IloExpr somaDosErros(env);
	// Para todas as folhas
	for(int f = firstLeaf; f < maxNos; f++) {
		somaDosErros += L[f];
	}

	IloExpr funcaoObjetivo(env);
	//double alpha = 0.3;
	// Definindo a ponderação da função objetivo
	funcaoObjetivo = (1.0/quantidadeElementosMaiorClasse) * somaDosErros + (alpha * IloSum(d));


	model.add(IloMinimize(env, funcaoObjetivo));

	IloCplex cplex(env);
	cplex.setParam(IloCplex::TiLim, maxTime);
	cplex.extract(model);

	cplex.solve();

	if(DISPLAY_OUTPUTS) { cplex.out() << "Solution status: " << cplex.getStatus() << endl; }
	if(DISPLAY_OUTPUTS) { cplex.out() << "Optimal value: " << cplex.getObjValue() << endl; }

	if(DISPLAY_OUTPUTS) { cout << endl << " ==================================================" << endl << endl; }

	//ofstream treeFile;
	//treeFile.open(datasets+"Iris/Trees/Iris70.csv");
	//vector<vector<double>> treeFileContent();


	// Mostrando em quais parâmetros cada nós se separou
	// Para todos os elementos
	if(DISPLAY_OUTPUTS) {
		for(int i = 0; i < firstLeaf; i++) {
			// Para todos os parâmetros dos elementos
			for(int j = 0; j < p; j++) {
				if(cplex.getValue(a[j][i]) >= 0.5) {
					cout << "O nó " << i << " separou pelo parâmetro " << j << endl;
					cout << "Valor de b[" << i << "]: " << cplex.getValue(b[i]) << endl;
				}
			}
		}
	}

	if(DISPLAY_OUTPUTS) {
		// Para todas as folhas
		for(int f = firstLeaf; f < maxNos; f++) {
			// Para todas as classes
			for(int k = 0; k < K; k++) {
				if(cplex.getValue(c[k][f]) >= 0.5) {
					cout << "A folha " << f << " foi classificada com a classe " << k << endl;
				}
			}
		}
	}


	if(DISPLAY_OUTPUTS) {
		// Para todas as folhas
		for(int f = firstLeaf; f < maxNos; f++) {
			cout << "Folha " << f << endl;
			// Para todas as classes
			for(int k = 0; k < K; k++) {
				cout << "classe: " << k << "\t" << (int) cplex.getValue(N[k][f]) << endl;
			}
			cout << endl;
		}
	}


	// Mostrando cada elemento classificado dentro de cada nó folha
	for(int f = firstLeaf; f < maxNos; f++) {
		// totalElementosEmCadaFolha[f]
		unsigned int quantidadeErros = 0;
		float	 	 taxaErros = 0.0;


		if(DISPLAY_OUTPUTS) { cout << "Folha: " << f << "\t"; }
		// Mostrar a classe atribuída ao nó folha
		int k = 0;
		for(; k < K; k++) {
			if(cplex.getValue(c[k][f] >= 0.5)) {
				if(DISPLAY_OUTPUTS) { cout << "Classe: " << k << endl; }
				break;
			}
		}

		// Mostrar todos os elementos contidos nele e suas respectivas classes reais.
		unsigned int counter = 0;
		for(int e = 0; e < n; e++) {
			counter++;
			int aux = (int) cplex.getValue(z[e][f]);
			if(aux >= 0.5) {
				// Mostra qual a classe do elemento
				if(DISPLAY_OUTPUTS) { cout << counter << "\tElemento: " << e << "\tclasse: " << (int) x[e][p] << "\t"; }

				if(DISPLAY_OUTPUTS) { cout << "Classificou: "; }
				if(k == x[e][p]) {
					if(DISPLAY_OUTPUTS) { cout << "Certo"; }
				}
				else {
					if(DISPLAY_OUTPUTS) { cout << "Errado"; }
					quantidadeErros++;
				}
				if(DISPLAY_OUTPUTS) { cout << endl; }
			}
		}

		// Se houver elementos neste nó f
		if((int) cplex.getValue(totalElementosEmCadaFolha[f]) > 0) {
			// Porcentagem de acertos e erros
			if(DISPLAY_OUTPUTS) { cout << setprecision(2); }
			taxaErros = quantidadeErros * 100 / (int) cplex.getValue(totalElementosEmCadaFolha[f]);
			if(DISPLAY_OUTPUTS) { cout << "Acertos:\t" << setprecision(7) << (int) cplex.getValue(totalElementosEmCadaFolha[f]) - quantidadeErros << "\t" << 100 - taxaErros << "%" << endl; }
			if(DISPLAY_OUTPUTS) { cout << "Erros:\t\t"	 << quantidadeErros << "\t" << taxaErros << "%" << endl; }
		}

		if(DISPLAY_OUTPUTS) { cout << endl << endl; }

		// Contando a quantidade total de erros
		totais[ERROS] += quantidadeErros;
		totais[ACERTOS] = x.size() - totais[ERROS];
	}


	//string treeSettingsFile = fileName+".MIO-Settings.txt";
	string treeSettingsFile = "tree-settings.txt";
	fstream treeSettingsStream;
	treeSettingsStream.open(treeSettingsFile, fstream::out);
	preOrder2(n, p, K, x, maxNos, firstLeaf, a, b, c, z, cplex, treeSettingsStream, 0, 0, 0);
	treeSettingsStream.close();



	// Imprimir os nós da árvore
	// /*

	cout << endl << endl;
	cout << "Árvore:" << endl;

	// Imprimindo os galhos
	for(int branch = 0; branch < totalBranches; branch++) {

		double splitValue = cplex.getValue(b[branch]);

		// Só imprimir nós galhos válidos
		for(int attr = 0; attr < p; attr++){
			int usedAttr = cplex.getValue(a[attr][branch]);
			if(usedAttr >= 0.5) {
				cout << "Branch " << branch << ",\tParam. " << attr << ",\tValue " << std::setprecision(6) << splitValue << endl;
			}
			else {
				//cout << "Branch " << "["<<branch<<"]"<<"["<<attr<<"] = --" << std::setprecision(6) << splitValue << endl << endl;
			}
		}
	}

	// Imprimindo as folhas
	for(int leaf = firstLeaf; leaf < maxNos; leaf++) {
		// Só imprimir folhas que contem classes
		for(int k_class = 0; k_class < K; k_class++) {

			int labeledLeaf = cplex.getValue(c[k_class][leaf] >= 0.5);

			if(labeledLeaf) {
				cout << "Leaf " << leaf << "\tclasse: " << k_class << "\tN Elements: " << cplex.getValue(N[k_class][leaf]) << endl;
			}
			else {
				//cout << "Leaf " << leaf << "\tclasse not: " << k_class << "\tN Elements: " << cplex.getValue(N[k_class][leaf]) << endl << endl;
			}
		}
	}

	cout << endl;

	// */
	// Fim de imprimir os nós da árvore



	if(DISPLAY_OUTPUTS) { cout << endl << endl; }
	//treeFile.close();
	env.end();




	// DADOS DA EXECUÇÃO COMPLETA
	endTime = clock();
	double spentTime = double(endTime - startTime) / double(CLOCKS_PER_SEC);
	double doubleValue = 0.0;
	if(DISPLAY_OUTPUTS) {
		// Quantos nós totais (úteis)
		cout << "Total elementos:\t" << x.size() << endl;
		// Quantos nós galho
		cout << "Total galhos:\t\t" << totais[GALHOS] << endl;
		// Quantos nós folha
		cout << "Total folhas:\t\t" << totais[FOLHAS] << endl;
		// Altura real da árvore
		cout << "Altura real:\t\t" << totais[ALTURA] << endl;
		// Total erros
		doubleValue = (double) (totais[ERROS] * 100) / x.size();
		cout << "Total erros:\t\t" << totais[ERROS] << "\t(" << setprecision(3) << doubleValue << "%)" << endl;
		// Total acertos
		doubleValue = (double) (totais[ACERTOS] * 100) / x.size();
		cout << "Total acertos:\t\t" << totais[ACERTOS] << "\t(" << setprecision(3) << doubleValue << "%)" << endl;
		// Porcentagem de erros
		//cout << "Porcentagem erros:\t" << setprecision(3) << totais[ERROS] * 100 / x.size() << "%" << endl;
		// Porcentagem de acertos
		//cout << "Porcentagem acertos:\t" << setprecision(3) << totais[ACERTOS] * 100 / x.size() << "%" << endl;
		// Tempo final
		cout << "Tempo gasto:\t\t" << setprecision(5) << spentTime << "s" << endl;
	}
	saveTrainingDataEnd(x.size(), totais[GALHOS], totais[FOLHAS], totais[ALTURA], totais[ERROS], totais[ACERTOS], spentTime);

	return 1;
}

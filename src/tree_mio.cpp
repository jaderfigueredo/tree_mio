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
#define BRANCH "galho"
#define LEAF "folha"

// Os valores são os índices do array que guardará os dados estatísticos
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
namespace fs = std::filesystem;

int DISPLAY_OUTPUTS = 1;

string datasets = "csv/";
string fileName = datasets+"iris.norm.csv";
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

// Retorna o último nó mais à direita de um nó t
int getLastRightChild(int t, int maxNos) {
	int child = getRightChild(t);

	// Caso base (parada)
	if(child > maxNos) {
		return t;
	}

	// Passo recursivo
	return getLastRightChild(child, maxNos);
}

// Retorna o nó pai de um nó filho
int getParent(int t) {
	return ceil((float) t/2)-1;
}

// Retorna o nível do nó calculado através do seu índice (0,1,...,n) na árvore
int getLevelFromIndex(int index) {
	// Caso base
	if(index == 0) {
		return 0;
	}

	// Passo recursivo
	return 1+getLevelFromIndex(getParent(index));
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




bool createDirectory(const string& directoryPath) {
    if (!fs::exists(directoryPath)) {
        if (fs::create_directory(directoryPath)) {
            if(DEBUG) { std::cout << "Directory created: " << directoryPath << std::endl; }
            return true;
        } else {
            std::cerr << "Failed to create directory: " << directoryPath << std::endl;
            return false;
        }
    }
    return true;  // Directory already exists
}

bool createFile(const string& filePath, const string& fileContent) {
    std::ofstream outputFile(filePath, std::ios::app);
    if (outputFile.is_open()) {
        outputFile << fileContent;
        outputFile.close();
        if(DEBUG) { std::cout << "File created: " << filePath << std::endl; }
        return true;
    } else {
        std::cerr << "Failed to create file: " << filePath << std::endl;
        return false;
    }
}




/**
 * Imprime a árvore com indentações e
 * Calcula alguns somatórios como:
 * 	- Quantidade de nós válidos
 * 	- Altura da árvore
 */
void imprimirArvore(int nElements,
			int pAttributes,
			int kClasses,
			vector<vector<float>> x,
			int maxNos,
			int firstLeaf,
			IloBoolVarArray2 a,
			IloNumVarArray b,
			IloBoolVarArray2 c,
			IloBoolVarArray2 z,
			IloCplex cplex,
			fstream &treeSettingsStream,
			IloIntVarArray2 N,
			ostringstream &results,
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
		string kindNode = BRANCH;
		int classNode = -1;
		int param = -1;
		float bound = -1;
		string separator = " ";
		bool isValidNode = false;

		// Se for nó folha
		if(currentNode >= firstLeaf && currentNode < maxNos) {
			kindNode = LEAF;

			// Obter a classe atribuída ao nó folha
			for(int k = 0; k < kClasses; k++) {
				if(cplex.getValue(c[k][currentNode] >= 0.5)) {
					classNode = k;
					isValidNode = true;
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
			isValidNode = true;
		}


		// Se for um nó válido, ou seja, nó galho ou nó folha
		if(param != -1 || classNode != -1) {
			results << endl;

			//
			for(int i = 0; i < indent; i++) {
				results << "  ";
			}

			// Contando total de nós úteis
			totais[NOS_UTEIS]++;

			// Imprime o nó
			results << "(" << kindNode << " "  << indexNode;
			treeSettingsStream << "(" << separator << indexNode << separator << kindNode << separator;

			if(kindNode.compare(BRANCH) == 0 && isValidNode) {
				results << " | feat: " << param << " | Threshold: " << setprecision(16) << bound << " | ";
				treeSettingsStream << param << separator << setprecision(32) << bound << separator;
				totais[GALHOS]++;
			}
			else if(isValidNode) {
				results << " | class: " << problemClasses[classNode];
				treeSettingsStream << int(classNode) << separator;
				totais[FOLHAS]++;

				vector<int> elementsOfClasses(kClasses);
				int totalElementos = 0;
				int totalAcertos = 0;
				int totalErros = 0;

				for(int k_class = 0; k_class < kClasses; k_class++) {
					elementsOfClasses[k_class] = round(cplex.getValue(N[k_class][currentNode]));
					totalElementos += round(cplex.getValue(N[k_class][currentNode]));
					if(classNode == k_class) {
						totalAcertos = cplex.getValue(N[k_class][currentNode]);
					}
				}

				totalErros = totalElementos - totalAcertos;

				results << " | " << totalElementos << " elementos";
				results << std::fixed;
				results << " | " << "Acertos: " << std::setprecision(2) << float(totalAcertos)/totalElementos*100 << "%";
				results << " | " << "Erros: " << std::setprecision(2) << float(totalErros)/totalElementos*100 << "%)";

				// Imprimir as quantidades de elementos da ŕvore
				for(int k_class = 0; k_class < kClasses; k_class++) {
					results << endl;
					for(int i = 0; i < indent; i++) {
						results << "  ";
					}

					results << " Classe ["<<k_class<<"] = "
							<< elementsOfClasses[k_class] << " elementos\t("
							<< std::setprecision(2)
							<< float(elementsOfClasses[k_class])/totalElementos*100 << "%)";
				}
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

			// Se o nó corrente é inválido, o nó pai continuará sendo o nó pai válido ocorrido no processo.
			indexNodeParent = getParent(indexNode);
		}

		// Registrando a maior altura alcançada da árvore
		if(indent > totais[ALTURA]) {
			totais[ALTURA] = indent;
		}

		// Vai para o filho esquerdo
		imprimirArvore(nElements, pAttributes, kClasses, x, maxNos, firstLeaf, a, b, c, z, cplex, treeSettingsStream, N, results, getLeftChild(currentNode), indent+1, indexLeftChild, indexNodeParent);
		// Vai para o filho direito
		imprimirArvore(nElements, pAttributes, kClasses, x, maxNos, firstLeaf, a, b, c, z, cplex, treeSettingsStream, N, results, getRightChild(currentNode), indent+1, indexRightChild, indexNodeParent);

		if(param == -1 && classNode == -1) {
			return;
		}

		if(kindNode.compare(BRANCH) == 0) {
			results << endl;
			for(int i = 0; i < indent; i++) {
				results << "  ";
			}
			results << ")";
		}

		treeSettingsStream << ")" << separator;
}




/**
 * A cada execução, este método é chamado para escrever os dados de execução em um arquivo de
 * histórico de execução para fins de análise e estatística.
 */
int saveTrainingDataStart(
		string idDT,
		int printOutputs,
		string datasetName,
		int nElements,
		int nAttributes,
		int nClasses,
		string classesInLine,
		int	maxTreeHeigth,
		float alpha,
		int minLeafElements,
		int timeLimit) {

	// Abrindo o arquivo e definindo o separador CSV
	fstream dataTrainingFile;


	//dataTrainingFile.close();
	dataTrainingFile.open(fileNameTrainingData, fstream::app);
	if(!dataTrainingFile.is_open()) {
		cerr << endl << "Erro ao abrir o arquivo \"" << fileNameTrainingData << "\"" << endl << endl;
		return 0;
	}
	if(DISPLAY_OUTPUTS && DEBUG) { cout << endl << "Arquivo " << fileNameTrainingData << " aberto com sucesso." << endl << endl;}
	string SEP = ",";
	string line = "";

	// Se o arquivo estiver vazio, criar a primeira linha com os cabeçalhos das colunas
	if(dataTrainingFile.tellg() == 0) {
		line = "IDbegin"+SEP+"printOutputs"+SEP+"datasetName"+SEP+"nElements"+SEP+"nAttributes"+SEP+
				"nClasses"+SEP+"classesInLine"+SEP+"maxTreeHeigth"+SEP+"alpha"+SEP+
				"minLeafElements"+SEP+"timeLimit"+SEP+"totalElementos"+SEP+"totalGalhos"+SEP+
				"totalFolhas"+SEP+"alturaReal"+SEP+"totalErros"+SEP+"totalAcertos"+SEP+"spentTime"+SEP+
				"spentTimeStr"+SEP+"nVariaveisModelo"+SEP+"Status"+SEP+"objValue"+SEP+"Gap"+SEP+"IDend";

		if(DISPLAY_OUTPUTS && DEBUG) { cout << line << endl;}
		dataTrainingFile << line;
	}

	// Gravando os dados de início da execução.
	line = idDT+SEP+std::to_string(printOutputs)+SEP+datasetName+SEP+std::to_string(nElements)+SEP+
			std::to_string(nAttributes)+SEP+std::to_string(nClasses)+SEP+"\""+classesInLine+"\""+SEP+
			std::to_string(maxTreeHeigth)+SEP+std::to_string(alpha)+SEP+std::to_string(minLeafElements)+SEP+
			std::to_string(timeLimit);
	if(DISPLAY_OUTPUTS && DEBUG) { cout << line << endl << endl;}
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
		unsigned long int spentTime,
		string spentTimeStr,
		long int nCols,
		string status,
		float objValue,
		float gap,
		string idDT) {

	// Abrindo o arquivo e definindo o separador CSV
	fstream dataTrainingFile;

	//dataTrainingFile.close();
	dataTrainingFile.open(fileNameTrainingData, fstream::app);
	if(!dataTrainingFile.is_open()) {
		cerr << endl << "Erro ao abrir o arquivo \"" << fileNameTrainingData << "\"" << endl << endl;
		return 0;
	}
	if(DISPLAY_OUTPUTS && DEBUG) { cout << endl << "Arquivo " << fileNameTrainingData << " aberto com sucesso." << endl << endl;}
	string SEP = ",";
	string line = "";

	// Gravando os dados de fim da execução.
	line = SEP+std::to_string(totalElementos)+SEP+std::to_string(totalGalhos)+SEP+
			std::to_string(totalFolhas)+SEP+std::to_string(alturaReal)+SEP+
			std::to_string(totalErros)+SEP+std::to_string(totalAcertos)+SEP+
			std::to_string(spentTime)+SEP+spentTimeStr+SEP+std::to_string(nCols)+SEP+
			status+SEP+std::to_string(objValue)+SEP+std::to_string(gap)+SEP+idDT;

	if(DISPLAY_OUTPUTS && DEBUG) { cout << line << endl << endl;}
	dataTrainingFile << line;

	dataTrainingFile.close();
	return 1;
}


// Function to convert IloCplex::CplexStatus to string
std::string getStatusString(IloAlgorithm::Status status) {
    switch (status) {
		case IloAlgorithm::Optimal:
			return "Optimal";
		case IloAlgorithm::Feasible:
			return "Feasible";
		case IloAlgorithm::Infeasible:
			return "Infeasible";
		case IloAlgorithm::InfeasibleOrUnbounded:
			return "Infeasible or Unbounded";
		case IloAlgorithm::Unbounded:
			return "Unbounded";
		case IloAlgorithm::Error:
			return "Error";
		case IloAlgorithm::Unknown:
			return "Unknown";
		default:
			return "Other Else";
    }
}



int main(int argc, char** argv){
	ostringstream results;

	// Inicializando totais[]
	for(int i = 0; i < N_TOTAIS; i++) {
		totais[i] = 0;
	}

	// Calculando o tempo gasto
	auto startTime = std::chrono::high_resolution_clock::now();
	//auto endTime = startTime;



	// Recebe o nome do arquivo por parâmetro quando é passado
	if(argc > 1) {
		DISPLAY_OUTPUTS = atoi(argv[1]);
	}

	if(DISPLAY_OUTPUTS) {
		if(argc == 1) {
			cout << "Você também pode passar os seguintes parametros:" << endl;
			cout << "[1] Mostrar saídas do programa (1 - Sim, 0 - não) " << endl;
			cout << "[2] arquivo do dataset (padrão: csv/iris.norm.csv)" << endl;
			cout << "[3] N. atributos (padrão: 4)" << endl;
			cout << "[4] N. classes (padrão: 3) " << endl;
			cout << "[5] Classes separadas por vírgula ('0,1,2')" << endl;
			cout << "[6] altura (padrao: 2)" << endl;
			cout << "[7] alpha (padrao: 0.3)" << endl;
			cout << "[8] Min. elementos na folha (padrao: 1)" << endl;
			cout << "[9] Tempo max. exec.(padrao: 0h2m)" << endl;
			cout << "[10] ID da execução (int)" << endl;
			cout << endl;
			cout << "Deseja continuar mesmo assim? [S ou n]: ";
			char continuar;
			cin >> continuar;
			if(continuar != 's' && continuar != 'S') {
				return 0;
			}
		}
	}


	if(DISPLAY_OUTPUTS) { cout << "Argumentos do programa:" << endl; }
	for(int i = 0; i < argc; i++) {
		if(DISPLAY_OUTPUTS && DEBUG) { cout << i << ": " << argv[i] << endl; }
	}
	if(DISPLAY_OUTPUTS) { cout << endl; }

	// Recebendo atributos por parâmetro

	// Recebe o nome do arquivo por parâmetro quando é passado
	if(argc > 1) {
		if(DISPLAY_OUTPUTS) { cout << "1 - Mostrar saídas:\t" << ((DISPLAY_OUTPUTS) ? "Sim" : "Não" ) << endl; }
	}

	// Recebe o nome do arquivo por parâmetro quando é passado
	if(argc > 2) {
		fileName = string(argv[2]);
		if(DISPLAY_OUTPUTS) { cout << "2 - Arquivo CSV:\t" << fileName << endl; }
	}

	// Recebe o número de atributos do dataset
	int p  = (argc > 3) ? atoi(argv[3]) : 4;
	if(DISPLAY_OUTPUTS) { cout << "3 - N. atributos:\t" << p << endl; }

	// Recebe o número de clases do dataset
	int K  = (argc > 4) ? atoi(argv[4]) : 3;
	if(DISPLAY_OUTPUTS) { cout << "4 - N. classes:\t\t" << K << endl; }

	// Recebe a lista de classes possíveis para o problema
	string classesInLine = (argc > 5) ? string(argv[5]) : "0,1,2";
	if(DISPLAY_OUTPUTS) { cout << "5 - Classes:\t\t" << classesInLine << endl; }

	// Recebe a altura da árvore por parâmetro quando é passada
	int altura = (argc > 6) ? atoi(argv[6]) : 2;
	if(DISPLAY_OUTPUTS) { cout << "6 - Altura:\t\t" << altura << endl; }

	// Recebe o alpha por parâmetro quando é passado
	char* ptrfloat;
	float alpha = (argc > 7) ? strtod(argv[7], &ptrfloat) : 0.3;
	if(DISPLAY_OUTPUTS) { cout << "7 - Alpha:\t\t" << alpha << endl; }

	// Recebe a quantidade mínima de cad folha quando é passada
	int nMin  = (argc > 8) ? atoi(argv[8]) : 1;
	if(DISPLAY_OUTPUTS) { cout << "Min. elementos/folha:\t" << nMin << endl; }

	// Recebe o tempo máximo para execução do programa em segundos
	string maxTimeStr  = (argc > 9) ? argv[9] : "0h2m";
	if(DISPLAY_OUTPUTS) { cout << "Tempo limite:\t\t"<< maxTimeStr << endl; }

	// Receber o ID de execução, pois este programa é parte de um pipeline de execução
	unsigned int idExec = (argc > 10) ? atoi(argv[10]) : 0;
	if(DISPLAY_OUTPUTS) { cout << "ID:\t\t\t"<< idExec << endl; }



	unsigned long int horas, minutos, segundos, milisegundos;
	string spentTimeStr;
	char charH, charM;
	std::istringstream iss(maxTimeStr);
	iss >> horas >> charH >> minutos >> charM;

	if(iss.fail()) {
		cerr << "Formato inválido de tempo inálido. Use '#h#m'" << endl;
		return 1;
	}

	int maxTime = (60*60)*horas + 60*minutos;

	//if(DISPLAY_OUTPUTS) { cout << "Tempo max. exec.:\t" << maxTime << " sec." << "\t(" << horas << " horas e " << minutos << "minutos)" << endl; }


	if(pow(2,altura) < K) {
		if(DISPLAY_OUTPUTS) {
			cerr << "Uma árvore de altura " << altura << " possui a quantidade de folhas ("
					<< pow(2,altura) << ") menor que a quantidade de classes ("
					<< K << "). Isso pode gerar uma classificação insuficiente." << endl;
		}
	}


	// Get the current timepoint
	// auto now = std::chrono::system_clock::now();
	auto clock = std::chrono::high_resolution_clock::now();
	auto timeNow = std::chrono::duration_cast<std::chrono::milliseconds>(clock.time_since_epoch());
	unsigned long int ID = static_cast<int>(timeNow.count());


	// Get the current timepoint
	auto now = std::chrono::system_clock::now();
	// Convert the timepoint to a time_t object
	std::time_t time = std::chrono::system_clock::to_time_t(now);
	// Convert the time_t to a tm struct in local time
	std::tm localTime = *std::localtime(&time);
	// Format and print the current date and time

	std::ostringstream formattedTime;
	formattedTime << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S:");
	// Get the string representation
	std::string idDT = formattedTime.str();


	// Gravando dados de execução do programa
	saveTrainingDataStart(to_string(ID), DISPLAY_OUTPUTS,fileName,-1,p,K,classesInLine,altura,alpha,nMin,maxTime);

	// CARREGANDO DADOS
	vector < vector < float > > x;
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

	if(DISPLAY_OUTPUTS && DEBUG) {
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
	int	somaClasses[K]; // Quantidade de elementos associados a cada classe K
	float minValueAttribute = 0; // Valor mínimo de um atributo
	float maxValueAttribute = 0; // Valor máximo de um atributo


	// LENDO ARQUIVO CSV SEM ESPAÇAMENTO DEPOIS DA VÍRGULA
	if(!arquivo.is_open()) {
		cerr << "O arquivo não pode ser aberto." << endl;
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
		vector<float> attributes(p);

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
				int index = findStringInVector(word, problemClasses);

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
				setClasses.insert(to_string(index));
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
	if(DISPLAY_OUTPUTS) { cout << "N. classes encontr.:\t" << setClasses.size() << endl; }

	if(DISPLAY_OUTPUTS) { cout << "N. elementos encontr.:\t" << n << endl; }

	if(DISPLAY_OUTPUTS) { cout << endl << endl << endl; }

	if(DISPLAY_OUTPUTS && DEBUG) { cout << "Dataset:" << endl; }

	unsigned int endOfBegin = (x.size() > 5) ? 5 : x.size();
	unsigned int startOfEnd = endOfBegin+1;
	startOfEnd = (x.size()-5 > endOfBegin) ? x.size()-5 : startOfEnd;

	for(unsigned int i = 0; i < endOfBegin; i++) {
		if(DISPLAY_OUTPUTS && DEBUG) { cout << i << "\t"; }
		for(unsigned int j = 0; j < x[i].size(); j++) {
			if(DISPLAY_OUTPUTS && DEBUG) { cout << x[i][j] << "\t"; }
		}
		if(DISPLAY_OUTPUTS && DEBUG) { cout << endl; }
	}

	if (DISPLAY_OUTPUTS && DEBUG and (startOfEnd > endOfBegin)) { cout << "\t..." << endl;}

	for(unsigned int i = startOfEnd; i < x.size(); i++) {
		if(DISPLAY_OUTPUTS && DEBUG) { cout << i << "\t"; }
		for(unsigned int j = 0; j < x[i].size(); j++) {
			if(DISPLAY_OUTPUTS && DEBUG) { cout << x[i][j] << "\t"; }
		}
		if(DISPLAY_OUTPUTS && DEBUG) { cout << endl; }
	}
	// FIM DA LEITURA DO ARQUIVO CSV SEM ESPAÇAMENTO DEPOIS DA VÍRGULA

	// Definindo variáveis de métricas
	string status = "--";
	float objValue = -1.0;
	long int nCols = -1;
	float gap = -1.0;

	IloEnv env;
	IloModel model(env);
	// Se for para ocultar saídas do programa, ocultar saídas do CPLEX também.
	if(!DISPLAY_OUTPUTS) { env.setOut(env.getNullStream()); }

	/*** VARIÁVEIS DO PROBLEMA ***/
	int maxNos = pow(2,(altura+1))-1;
	// Primeira Folha
	int firstLeaf = floor(maxNos/2);
	// Total de nós branches
	int totalBranches = firstLeaf;



	if(DISPLAY_OUTPUTS && DEBUG) {
		cout << "Altura: " << altura << endl;
		cout << "maxNos: " << maxNos << endl;
		cout << "Primeira folha: " << firstLeaf << endl;
	}


	// d[i] = 1 se o nó i for branch; 0 se for nó folha
	IloBoolVarArray d(env, totalBranches);

	// a[j][t] = 1 se o parâmetro j é considerado no nó t, 0 se não.
	IloBoolVarArray2 a(env, p);

	// Criando a variável a
	for(int j = 0; j < p; j++){
		a[j] = IloBoolVarArray(env, totalBranches);
	}


	// Se os dados já estarão normalizados, o maior valor é 1 e o menor valor é 0.
	// A amplitude M é 1, mas convencionou-se em M = 2
	maxValueAttribute = 1;
	minValueAttribute = 0;
	float M = 2; // Professor acha que M deve ser maior que 1

	// Imprimindo valores mínimo e máximo dos atributos
	if(DISPLAY_OUTPUTS & DEBUG) {
		cout << "Valor mínimo:\t" << minValueAttribute << endl;
		cout << "Valor máximo:\t" << maxValueAttribute << endl;
		cout << endl;
	}

	// 2. Definir uma variável para armazenar o valor de separação limitado pelo
	// valor mínimo e máximo de um atributo.
	IloNumVarArray b(env, totalBranches);
	for(int i = 0; i < totalBranches; i++) {
		// Quando um nó não faz divisão, ele precisa mandar todos elementos para a direita, para isso
		// o valor do limiar (b[i]) tem que ser menor que o mínimo para permitir que todos os valores
		// sejam maior que o limiar.
		b[i] = IloNumVar(env, 0, 1); // Prof.: b tem que poder ser 0
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
		N[k] = IloIntVarArray(env, maxNos, 0, n);  // Só vamos usar os índices das folhas
	}

	// c[k][t] = 1, se a folha t é classificada como classe k, 0 caso contrário
	IloBoolVarArray2 c(env, K);
	// Para toda classe k
	for(int k = 0; k < K; k++) {
		c[k] = IloBoolVarArray(env, maxNos);  // Só vai usar para os indices das folhas
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
		model.add(somaAtributosDivididosNesteNo == d[t]); // Está igual ao artigo. Fórmula (1)
	}


	// Restrição 3: Garante que se um nó não faz separação,
	// consequentemente o limiar dele também é zero, fazendo todos os filhos irem para a direita.
	// Para todos os nós folha
	for(int t = 0; t < totalBranches; t++) {
		model.add(b[t] <= d[t]); // p/ dados normalizados
	}


	// Se um nó é branch, o pai dele também é branch
	// _Todo d(t) tem que ser <= que d(getParent(t))

	// Restrição para o nó raiz
	model.add(d[0] == 1);
	// Restrição 5: para os demais nós.
	for(int t = 1; t < totalBranches;t++) {
		if(DISPLAY_OUTPUTS && DEBUG) {
			cout << "t: " << t << endl;
			cout << "getparent(t): " << getParent(t) << endl;
		}
		model.add(d[t] <= d[getParent(t)]); // Trecho corrigido do artigo
	}

	// Restrição 6: de que os elementos só estão em nós folhas
	// Percorrendo os elementos
	for(int i = 0; i < n; i++) {
		// Percorrendo os nós folhas
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
		model.add(somaElementoDasFolhas == 1); // Define que cada elemento termine finalmente em uma única folha
	}

	// Encontrando o epsilon, que é a menor diferença entre dois atributos que não for zero
	// Para_todo elemento
	double epsilon = M; // valor suficientemente grande para forçar haver uma diferença entre 2 atributos que seja menor que ele.
	vector<float> epsilons(p);
	for(int j = 0; j < p; j++) {
		epsilons[j] = M; // espsilon começa com M, pois M é a maior diferença entre todos os valores de atributos
	}

	//for(int i = 0; i < n; i++) {
	for(int i = 0; i < n-1; i++) {
		// Para_todos os pares de elementos
		for(int h = i+1; h < n; h++) {
			// Para _todo atributos
			for(int j = 0; j < p; j++) {
				double difference = abs(x[i][j]-x[h][j]);
				// Não atualizar o valor se a diferença entre dois atributos for zero.
				if(difference >= 0.00000001) {
					epsilon = epsilon > difference ? difference : epsilon;
					epsilons[j] = (epsilons[j] > difference) ? difference : epsilons[j];
				}
			}
		}
	}

	if(DISPLAY_OUTPUTS) {
		if(DEBUG) { cout << endl << "epsilon:\t" << std::fixed << std::setprecision(10) << epsilon << endl << endl; }
		if(epsilon < 0 || epsilon > 1) {
			string _continue;
			cout << "Continuar? (S ou N):\t";
			cin >> _continue;
			cout << endl << endl;
		}

		// Imprimindo cada epsilon encontrado para cada atributo
		cout.precision(10);
		//cout << "Epsilon: " << epsilon << endl;
		for(int j = 0; j < p; j++) {
			if(DEBUG){ cout << "Epsilon[" << j << "]: " << epsilons[j] << endl; }
		}
	}


	// Restrição 11: de que se um elemento pertence à um nó t, ele não pode ter sido
	// desviado (na árvore) por um ancestral de t.
	// Em outras palavras, o elemento precisa ter passado por todos os ancestrais de t.
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
					for(int j = 0; j < p; j++) {
						produtoEscalar += a[j][pai] * x[i][j];
					}

					model.add(produtoEscalar >= b[pai]-(1-z[i][t]));

				}
			}
			else { // Se o nó atual é ímpar
				// Aplicar Restrição 11: que
				for(int i = 0; i < n; i++) {
					// Guardará qual o atributo está sendo dividido no nó corrente
					IloExpr epsilonCurr(env);

					// Para cada parâmetro
					IloExpr produtoEscalar(env);
					for(int j = 0; j < p; j++) {
						produtoEscalar += a[j][pai] * x[i][j];

						// Obtendo o epsilon de qual atributo está sendo usado para fazer a divisão do nó
						epsilonCurr += a[j][pai] * epsilons[j];
					}

					model.add(produtoEscalar+epsilon <= b[pai]+((M)*(1-z[i][t]))); // M = amplitude absoluta
					//model.add(produtoEscalar+epsilonCurr <= b[pai]+((M)*(1-z[i][t]))); // M = amplitude absoluta

					/**
					 * epsilon: 0.1
					 * limiar: 0.5
					 * 0.4+0.1 <= 0.5
					 * 0.5 <= 0.5
					 *
					 */
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
		model.add(totalElementosEmCadaFolha[f] == totalElementosDeUmaFolha); //N_t
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
			model.add(L[f] >= (totalElementosEmCadaFolha[f]-N[k][f])-(n*(1-c[k][f])));
			model.add(L[f] <= (totalElementosEmCadaFolha[f]-N[k][f])+(n*(c[k][f])));
		}
	}



	/**
	 * Esta regra não está no artigo e está sendo imposta pelos desenvolvedores.
	 * A regra diz que, se um um nó faz divisão, deve haver ao menos uma folha na
	 * subárvore esquerda e uma folha na subárvore direita.
	 * Obs: Se existe apenas uma folha na subárvore descendente em qualquer um dos lados,
	 * esta folha é o nó mais à direita da subárvore.
	 * @date 23/10/2023
	 */
	for(int t = 0; t < totalBranches; t++) {
		IloInt nohMaisAaDireitaSubarvoreEsquerda;
		IloInt nohMaisAaDireitaSubarvoreDireita;

		nohMaisAaDireitaSubarvoreEsquerda = getLastRightChild(getLeftChild(t), maxNos);
		nohMaisAaDireitaSubarvoreDireita = getLastRightChild(getRightChild(t), maxNos);

		model.add(d[t] <= l[nohMaisAaDireitaSubarvoreEsquerda]);
		model.add(d[t] <= l[nohMaisAaDireitaSubarvoreDireita]);
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

	if(DISPLAY_OUTPUTS && DEBUG) { cout << "quantidadeElementosMaiorClasse: " << quantidadeElementosMaiorClasse << endl; }

	// FUNÇÃO OBJETIVO
	IloExpr somaDosErros(env);
	// Para todas as folhas
	for(int f = firstLeaf; f < maxNos; f++) {
		somaDosErros += L[f];
	}

	IloExpr funcaoObjetivo(env);
	//float alpha = 0.3;
	// Definindo a ponderação da função objetivo
	funcaoObjetivo = (1.0/quantidadeElementosMaiorClasse) * somaDosErros + (alpha * IloSum(d));


	model.add(IloMinimize(env, funcaoObjetivo));

	IloCplex cplex(env);
	cplex.setParam(IloCplex::TiLim, maxTime);
	cplex.extract(model);


	/**
	 * Apagar se não resolver
	// Criando variáveis de contagem de números do modelo
	cplex.setParam(IloCplex::CutCliques, 2);
	cplex.setParam(IloCplex::CutCovers, 2);
	cplex.setParam(IloCplex::CutImplBd, 2);
	cplex.setParam(IloCplex::CutFlowPaths, 2);
	cplex.setParam(IloCplex::MIRCuts, 2);
	*/
	// Obtendo os dados do resolvedor

	bool isModelSolved = cplex.solve();

	IloAlgorithm::Status statusIloAlg = cplex.getStatus();
	status = isModelSolved ? getStatusString(statusIloAlg) : "--";

	IloNum objValueIloNum = cplex.getObjValue();
	objValue = isModelSolved ? static_cast<float>(objValueIloNum) : -1.0;

	IloInt nColsIloNum = cplex.getNcols();
	nCols = isModelSolved ? static_cast<int>(nColsIloNum) : -1;

	IloNum gapIloNum = cplex.getMIPRelativeGap();
	gap = isModelSolved ? static_cast<float>(gapIloNum) : -1.0;




	// Mostrando em quais parâmetros cada nós se separou
	// Para todos os elementos
	if(DISPLAY_OUTPUTS && DEBUG) {
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

	if(DISPLAY_OUTPUTS && DEBUG) {
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


	if(DISPLAY_OUTPUTS && DEBUG) {
		// Para todas as folhas
		for(int f = firstLeaf; f < maxNos; f++) {
			cout << "Folha " << f << endl;
			// Para todas as classes
			for(int k = 0; k < K; k++) {
				cout << "classe: " << k << "\t" << cplex.getValue(N[k][f]) << endl;
			}
			cout << endl;
		}
	}


	// Mostrando cada elemento classificado dentro de cada nó folha
	for(int f = firstLeaf; f < maxNos; f++) {
		// totalElementosEmCadaFolha[f]
		unsigned int quantidadeErros = 0;
		float	 	 taxaErros = 0.0;


		if(DISPLAY_OUTPUTS && DEBUG) { cout << "Folha: " << f << "\t"; }
		// Mostrar a classe atribuída ao nó folha
		int k = 0;
		for(; k < K; k++) {
			if(cplex.getValue(c[k][f] >= 0.5)) {
				if(DISPLAY_OUTPUTS && DEBUG) { cout << "Classe: " << k << endl; }
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
				if(DISPLAY_OUTPUTS && DEBUG) { cout << counter << "\tElemento: " << e << "\tclasse: " << (int) x[e][p] << "\t"; }

				if(DISPLAY_OUTPUTS && DEBUG) { cout << "Classificou: "; }
				if(k == x[e][p]) {
					if(DISPLAY_OUTPUTS && DEBUG) { cout << "Certo"; }
				}
				else {
					if(DISPLAY_OUTPUTS && DEBUG) { cout << "Errado"; }
					quantidadeErros++;
				}
				if(DISPLAY_OUTPUTS && DEBUG) { cout << endl; }
			}
		}

		// Se houver elementos neste nó f
		if((int) cplex.getValue(totalElementosEmCadaFolha[f]) > 0) {
			// Porcentagem de acertos e erros
			if(DISPLAY_OUTPUTS && DEBUG) { cout << setprecision(2); }
			taxaErros = quantidadeErros * 100 / (int) cplex.getValue(totalElementosEmCadaFolha[f]);
			if(DISPLAY_OUTPUTS && DEBUG) { cout << "Acertos:\t" << setprecision(7) << (int) cplex.getValue(totalElementosEmCadaFolha[f]) - quantidadeErros << "\t" << 100 - taxaErros << "%" << endl; }
			if(DISPLAY_OUTPUTS && DEBUG) { cout << "Erros:\t\t"	 << quantidadeErros << "\t" << taxaErros << "%" << endl; }
		}

		if(DISPLAY_OUTPUTS && DEBUG) { cout << endl << endl; }

		// Contando a quantidade total de erros
		totais[ERROS] += quantidadeErros;
		totais[ACERTOS] = x.size() - totais[ERROS];
	}


	//string treeSettingsFile = fileName+".MIO-Settings.txt";
	string treeSettingsFile = "tree-settings.txt";
	fstream treeSettingsStream;
	treeSettingsStream.open(treeSettingsFile, fstream::out);



	if(DISPLAY_OUTPUTS) { cout << endl << endl; }
	imprimirArvore(n, p, K, x, maxNos, firstLeaf, a, b, c, z, cplex, treeSettingsStream, N, results, 0, 0, 0);
	if(DISPLAY_OUTPUTS) { cout << endl << endl; }


	treeSettingsStream.close();



	if(DISPLAY_OUTPUTS && DEBUG) {
		// Imprimir os nós da árvore
		cout << endl << endl;
		cout << "Árvore:" << endl;

		// Imprimindo os galhos
		for(int branch = 0; branch < totalBranches; branch++) {

			float splitValue = cplex.getValue(b[branch]);

			// Só imprimir nós galhos válidos
			for(int attr = 0; attr < p; attr++){
				int usedAttr = cplex.getValue(a[attr][branch]);
				if(usedAttr >= 0.5) {
					cout << "Branch " << branch << ",\tParam. " << attr << ",\tValue " << std::setprecision(6) << splitValue << endl;
				}
				else {
					// cout << "Branch " << "["<<branch<<"]"<<"["<<attr<<"] = --" << std::setprecision(6) << splitValue << "\t";
				}
			}
		}


		cout << endl;

		// Imprimindo as folhas
		for(int leaf = firstLeaf; leaf < maxNos; leaf++) {
			// Só imprimir folhas que contem classes
			for(int k_class = 0; k_class < K; k_class++) {

				int labeledLeaf = cplex.getValue(c[k_class][leaf] >= 0.5);

				if(labeledLeaf) {
					cout << "Leaf " << leaf << "\tclasse: " << k_class
							<< "\tN Elements: " << cplex.getValue(N[k_class][leaf]) << endl;
				}
				else {
					// cout << "Leaf " << leaf << "\tclasse not: " << k_class << "\tN Elements: " << cplex.getValue(N[k_class][leaf]) << "\t";
				}
			}
		}

		cout << endl;

		// */
		// Fim de imprimir os nós da árvore
	} // END IF DISPLAY_OUTPUTS



	if(DISPLAY_OUTPUTS) { cout << endl << endl; }
	//treeFile.close();

	env.end();

	// DADOS DA EXECUÇÃO COMPLETA
	auto endTime = std::chrono::high_resolution_clock::now();

	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
	unsigned long int spentTime = duration.count();
	float floatValue = 0.0;


	// Extract hours, minutes, seconds, and remaining milliseconds
	auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
	duration -= hours;

	auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
	duration -= minutes;

	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
	duration -= seconds;

	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

	horas = int(hours.count());
	minutos = int(minutes.count());
	segundos = int(seconds.count());
	milisegundos = int(duration.count());
	spentTimeStr = to_string(horas)+"h"+to_string(minutos)+"min"+to_string(segundos)+"s"+to_string(milisegundos)+"ms";




	results << endl << "=== FIT ==========================================" << endl << endl;

	// Quantos nós totais (úteis)
	results << "N. elementos trein.:\t" << x.size() << endl;
	// Quantos nós galho
	results << "N. galhos:\t\t" << totais[GALHOS] << endl;
	// Quantos nós folha
	results << "N. folhas:\t\t" << totais[FOLHAS] << endl;
	// Altura real da árvore
	results << "Altura real/max:\t" << totais[ALTURA] << " / " << altura << endl;
	// Total acertos
	floatValue = (float) (totais[ACERTOS] * 100) / x.size();
	results << "Total acertos:\t\t" << totais[ACERTOS] << "\t(" << setprecision(3) << floatValue << "%)" << endl;
	// Total erros
	floatValue = (float) (totais[ERROS] * 100) / x.size();
	results << "Total erros:\t\t" << totais[ERROS] << "\t(" << setprecision(3) << floatValue << "%)" << endl;

	results << "Tempo gasto:\t\t" << spentTimeStr << endl;


	results << endl << "-----------------------------" << endl << endl;

	results << "Solução:\t\t" << status << endl;
	results << "Valor objetivo:\t\t" << objValue << endl;
	results << "N. Variáveis:\t\t" << nCols << endl;
	results << "GAP:\t\t\t" << std::setprecision(2) << gap*100 << "%" <<endl;

	results << endl << "-----------------------------" << endl << endl;

	results << "Parametros de entrada:" << endl;
	results << "CSV:\t\t\t" << fileName << endl;
	results << "N. Elementos:\t\t" << x.size() << endl;
	results << "Atributos:\t\t" << p << endl;
	results << "Classes:\t\t" << K << " ("<<classesInLine<<")" << endl;
	results << "Altura max.:\t\t" << altura << endl;
	results << "Alpha:\t\t\t" << alpha << endl;
	results << "Min. elem. folha:\t" << nMin << endl;
	results << "Tempo Limite:\t\t" << maxTimeStr << endl;

	results << endl << "==================================================" << endl << endl;


	// Salvando saídas no arquivo
	string directoryPath = "outputs";
	ostringstream alpha3f;
	alpha3f << std::fixed << std::setprecision(3) << alpha;
	string outputsFile = to_string(ID)
			+"_n"+to_string(x.size())
			+"_p"+to_string(p)
			+"_K"+to_string(K)
			+"_h"+to_string(altura)
			+"_a"+alpha3f.str()
			+"_nMin"+to_string(nMin)
			+"_t"+maxTimeStr
			+"_"+status
			+".txt";

	// Check and create the directory
	if (createDirectory(directoryPath)) {
		string filePath = directoryPath + "_" + outputsFile;
		string fileContent = results.str();
		// Create the file
		createFile(filePath, fileContent);
	}



	if(DISPLAY_OUTPUTS) {
		cout << results.str();

		cout << endl;

		/*

		// Clique cuts applied (interget)
		int numCliqueCuts = cplex.getNcuts(IloCplex::CutCliques);

		// Number of solutions
		int numSolutions = cplex.getSolnPoolNumSolns();
		// Tem erro nessa maneira de obter a quantidade de soluções de um modelo

		// Cover cuts aplied:
		int numCoverCuts = cplex.getNcuts(IloCplex::CutCovers);

		// Implied bounds cuts applied:
		int numImpliedBoundCuts = cplex.getNcuts(IloCplex::CutImplBd);

		// Flow cuts applied:
		int numFlowCuts = cplex.getNcuts(IloCplex::CutFlowPaths);

		// Mixed integer rounding cuts applied
		int numMIRcuts = cplex.getNcuts(IloCplex::CutMIR);
		*/

	}

	// Gravando dados de execução do programa
	saveTrainingDataStart(
			to_string(ID),
			DISPLAY_OUTPUTS,
			fileName,
			-1,
			p,
			K,
			classesInLine,
			altura,
			alpha,
			nMin,
			maxTime
		);

	saveTrainingDataEnd(
			x.size(),
			totais[GALHOS],
			totais[FOLHAS],
			totais[ALTURA],
			totais[ERROS],
			totais[ACERTOS],
			spentTime,
			spentTimeStr,
			nCols,
			status,
			objValue,
			gap,
			to_string(ID)
		);


	return 1;


}

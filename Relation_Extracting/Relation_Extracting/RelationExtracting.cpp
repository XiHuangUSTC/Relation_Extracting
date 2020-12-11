#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>

#define FILEPATH_TRAIN "..\\train.txt"
#define FILEPATH_TEST "..\\test.txt"
#define DOUBLE_CONSONANT_ING_LIST "..\\DoubleConsonant_ing_List.txt"
#define DELETE_E_ING_LIST "..\\Delete_e_ing_List.txt"
#define SPECIAL_WORD_LIST "..\\Special_Word_List.txt"
#define TRAIN_NUMBER 6400        //训练集语句数
#define TEST_NUMBER 1600          //测试集语句数
#define FEATURE_NUMBER 30       //特征词个数

using namespace std;

map<string, string> map_DoubleConsonant_ing, map_Delete_e_ing, map_Special_Word;

bool Cmp_By_Value(pair<string, int> leftPair, pair<string, int> rightPair) {
	return leftPair.second > rightPair.second;
}

//标准化，返回值：0--正常返回，1--冠词
int Standardize(string& word) {
	int wordLength = 0;
	string::iterator ite_string;
	map<string, string>::iterator ite_map;

	//大写转小写
	for (ite_string = word.begin(); ite_string != word.end(); ite_string++)
		if (isalpha(*ite_string) == 1)
			*ite_string = *ite_string + 32;
	//冠词
	if (word == "the" || word == "a" || word == "an")
		return 1;
	//特殊词
	ite_map = map_Special_Word.find(word);
	if (ite_map != map_Special_Word.end()) {
		word = (*ite_map).second;
		return 0;
	}

	wordLength = word.length();
	//ing
	if (wordLength >= 4 && word[wordLength - 3] == 'i' && word[wordLength - 2] == 'n' && word[wordLength - 1] == 'g') {
		//双写辅音加ing
		ite_map = map_DoubleConsonant_ing.find(word);
		if (ite_map != map_DoubleConsonant_ing.end()) {
			word.erase(wordLength - 4);
			return 0;
		}
		//去e加ing
		ite_map = map_Delete_e_ing.find(word);
		if (ite_map != map_Delete_e_ing.end()) {
			word[wordLength - 3] = 'e';
			word.erase(wordLength - 2);
			return 0;
		}
		//规则ing
		word.erase(wordLength - 3);
	}
	//ed
	else if (wordLength >= 3 && word[wordLength - 2] == 'e' && word[wordLength - 1] == 'd') {
		//去y加ied
		if (word[wordLength - 3] == 'i') {
			word[wordLength - 3] = 'y';
			word.erase(wordLength - 2);
		}
		//双写辅音加ed
		else if (wordLength >= 4 && word[wordLength - 4] == word[wordLength - 3])
			word.erase(wordLength - 3);
		//规则ed
		else
			word.erase(wordLength - 2);
	}
	//s
	else if (wordLength >= 2 && word[wordLength - 1] == 's') {
		//以s,o,x,sh,ch结尾加es
		if (wordLength >= 4 && word[wordLength - 2] == 'e' && (word[wordLength - 3] == 's'
			|| word[wordLength - 3] == 'o' || word[wordLength - 3] == 'x'
			|| (word[wordLength - 4] == 's' && word[wordLength - 3] == 'h')
			|| (word[wordLength - 4] == 'c' && word[wordLength - 3] == 'h')))
			word.erase(wordLength - 2);
		//去y加ies
		else if (wordLength >= 3 && word[wordLength - 3] == 'i' && word[wordLength - 2] == 'e') {
			word[wordLength - 3] = 'y';
			word.erase(wordLength - 2);
		}
		//去f，加ves
		else if (word == "selves" || word == "shelves" || word == "wolves" || word == "thieves"
			|| word == "calves") {
			word[wordLength - 3] = 'f';
			word.erase(wordLength - 2);
		}
		//去fe，加ves
		else if (word == "wives" || word == "knives" || word == "lives") {
			word[wordLength - 3] = 'f';
			word.erase(wordLength - 1);
		}
		//规则s
		else
			word.erase(wordLength - 1);
	}
	return 0;
}

int main() {
	ifstream f_Train(FILEPATH_TRAIN);
	ifstream f_Test(FILEPATH_TEST);
	ifstream f_DoubleConsonant_ing_List(DOUBLE_CONSONANT_ING_LIST);
	ifstream f_Delete_e_ing_List(DELETE_E_ING_LIST);
	ifstream f_Special_Word_List(SPECIAL_WORD_LIST);
	int i, j, k, feature_Count[10][FEATURE_NUMBER] = {};
	double rel_Probability[10], prob = 0, max_Prob;
	char ch;
	string str, str1, word, line, relation, max_Relation;
	string relations[10] = { "Component-Whole", "Other", "Instrument-Agency", "Member-Collection",
		"Cause-Effect", "Entity-Destination", "Content-Container", "Message-Topic", "Product-Producer",
		"Entity-Origin" };
	map<string, int> map_Words, map_Feature, map_Relation;
	map<string, int>::iterator ite_map;
	vector<pair<string, int>> vec_Words;
	vector<int> vec_Appear;
	
	for (i = 0; i < 12; i++) {
		str.clear();
		f_DoubleConsonant_ing_List >> str;
		map_DoubleConsonant_ing.insert(pair<string, string>(str, str1));
	}
	for (i = 0; i < 23; i++) {
		str.clear();
		f_Delete_e_ing_List >> str;
		map_Delete_e_ing.insert(pair<string, string>(str, str1));
	}
	for (i = 0; i < 54; i++) {
		str.clear();
		str1.clear();
		f_Special_Word_List >> str;
		f_Special_Word_List >> str1;
		map_Special_Word.insert(pair<string, string>(str, str1));
	}
	f_Delete_e_ing_List.close();
	f_DoubleConsonant_ing_List.close();
	f_Special_Word_List.close();

	for (i = 0; i < TRAIN_NUMBER; i++) {
		//第一行
		while ((ch = f_Train.get()) != '\n') {
			word.clear();
			//取一个词
			if (isalpha(ch) != 0) {
				do {
					word.push_back(ch);
					ch = f_Train.get();
				} while (isalpha(ch) != 0);
				//跳过冠词
				if (Standardize(word) == 1)
					continue;
				//插入map
				pair<map<string, int>::iterator, bool> insert_Pair;
				insert_Pair = map_Words.insert(pair<string, int>(word, 1));
				if (insert_Pair.second == false)
					map_Words[word]++;
			}
		}
		//第二行
		getline(f_Train, str);
	}
	cout << "词频统计完成" << endl;

	//将map导入vector，进行排序
	for (ite_map = map_Words.begin(); ite_map != map_Words.end(); ite_map++)
		vec_Words.push_back(*ite_map);
	nth_element(vec_Words.begin(), vec_Words.begin() + FEATURE_NUMBER - 1, vec_Words.end(), Cmp_By_Value);
	cout << "排序完成" << endl;

	//取词频前30的词当作特征
	for (i = 0; i < FEATURE_NUMBER; i++)
		map_Feature.insert(pair<string, int>(vec_Words[i].first, i));

	//统计各特征出现在不同关系中的次数和各关系出现的次数
	for (i = 0; i < 10; i++)
		map_Relation.insert(pair<string, int>(relations[i], 0));
	for (i = 0; i < FEATURE_NUMBER; i++)
		vec_Appear.push_back(0);
	f_Train.seekg(ios::beg);
	for (i = 0; i < TRAIN_NUMBER; i++) {
		getline(f_Train, line);
		//读取关系，计入关系出现的次数
		relation.clear();
		while ((ch = f_Train.get()) != '(') {
			relation.push_back(ch);
		}
		ite_map = map_Relation.find(relation);
		ite_map->second++;
		//读取特征词，计入特征出现次数
		for (j = 0; j < FEATURE_NUMBER; j++)
			if (vec_Appear[j] == 1)
				vec_Appear[j] = 0;
		for (j = 0; j < line.length(); j++) {
			word.clear();
			while (j < line.length() && isalpha(line[j]) != 0) {
				word.push_back(line[j]);
				j++;
			}
			//标准化
			if (word.empty() || Standardize(word) == 1)
				continue;
			//确认特征词
			ite_map = map_Feature.find(word);
			if (ite_map != map_Feature.end() && vec_Appear[(*ite_map).second] == 0)
				vec_Appear[(*ite_map).second] = 1;
		}
		//确认关系
		for (j = 0; j < 10; j++)
			if (relation == relations[j])
				break;
		for (k = 0; k < FEATURE_NUMBER; k++)
			feature_Count[j][k] = feature_Count[j][k] + vec_Appear[k];
		getline(f_Train, str);
	}
	cout << "统计特征出现次数完成" << endl;

	//计算概率
	for (i = 0; i < TEST_NUMBER; i++) {
		for (j = 0; j < FEATURE_NUMBER; j++)
			if (vec_Appear[j] == 1)
				vec_Appear[j] = 0;
		for (j = 0; j < 10; j++)
			rel_Probability[j] = 1;
		//一行
		while ((ch = f_Test.get()) != '\n') {
			word.clear();
			//取一个词
			if (isalpha(ch) != 0) {
				do {
					word.push_back(ch);
					ch = f_Test.get();
				} while (isalpha(ch) != 0);
				//标准化
				if (Standardize(word) == 1)
					continue;
				//确认特征词
				ite_map = map_Feature.find(word);
				if (ite_map != map_Feature.end() && vec_Appear[(*ite_map).second] == 0)
					vec_Appear[(*ite_map).second] = 1;
			}
		}
		//计算每种关系的概率
		for (j = 0; j < 10; j++) {
			ite_map = map_Relation.find(relations[j]);
			rel_Probability[j] *= ((double)(*ite_map).second) / TRAIN_NUMBER;
			for (k = 0; k < FEATURE_NUMBER; k++) {
				prob = vec_Appear[k] ? ((double)feature_Count[j][k]/(*ite_map).second) : (1 - (double)feature_Count[j][k] / (*ite_map).second);
				rel_Probability[j] *= prob;
			}
		}
		//输出概率最大的关系
		max_Prob = 0;
		for (j = 0; j < 10; j++)
			if (rel_Probability[j] > max_Prob) {
				max_Prob = rel_Probability[j];
				max_Relation = relations[j];
			}
		cout << "Detected Relation:" << max_Relation << endl;
	}

	f_Train.close();
	f_Test.close();
	std::system("pause");
	return 0;
}
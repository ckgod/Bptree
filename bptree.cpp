#pragma warning (disable:4996)
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <cstdlib>
#include <fstream>
#include <stdio.h>
using namespace std;

#define re_c reinterpret_cast<char*>
#define re_c_cst reinterpret_cast<const char*>

class leafEntry {
public:
	int key;
	int value;
	leafEntry(int key, int value) {
		this->key = key;
		this->value = value;
	}
};

class non_leafEntry {
public:
	int key;
	int NextLevelBID;
	non_leafEntry(int key, int NextLevelBID) {
		this->key = key;
		this->NextLevelBID = NextLevelBID;
	}
};

class non_leafNode {
public:
	non_leafNode* parent;
	int bID;
	int NextLevelBID;
	int depth;
	vector<non_leafEntry> non_leafList;
};

class leafNode {
public:
	non_leafNode* parent;
	int bID;
	int NextBID;
	int depth;
	vector<leafEntry> keyList;
};

bool leafEntryOper(leafEntry l1, leafEntry l2) {
	return l1.key < l2.key;
}
bool non_leafEntryOper(non_leafEntry l1, non_leafEntry l2) {
	return l1.key < l2.key;
}

class BTree {
public:
	int headerSize = 12; // blockSize + root + depth
	int root;
	int blockSize;
	int depth;
	int blockCnt, nodeCapacity;
	char* binFileName;
	ifstream finput;
	ofstream foutput;
	fstream f_file;

	BTree() {};

	BTree(char* binFileName, int blockSize) {
		this->binFileName = binFileName;
		this->blockSize = blockSize;
	}

	void create(char* binFileName, int blockSize) {
		foutput.open(binFileName, ios::binary);
		if (foutput.is_open()) {
			foutput.write(re_c(&blockSize), sizeof(int));
			foutput.write(re_c(&root), sizeof(int));
			foutput.write(re_c(&depth), sizeof(int));
		}
		foutput.close();
	}

	void openBin(char* binFileName) {
		this->binFileName = binFileName;
		finput.open(binFileName, ios::in | ios::out | ios::binary);
		finput.read(re_c(&blockSize), sizeof(int));
		finput.read(re_c(&root), sizeof(int));
		finput.read(re_c(&depth), sizeof(int));
		finput.seekg(0, std::ios::end);
		int fileSize = finput.tellg();
		this->nodeCapacity = ((blockSize - 4) / 8);
		//cout << nodeCapacity << "\n";
		//cout << blockSize << "\n";
		//cout << fileSize << "\n";
		this->blockCnt = (fileSize - 12) / blockSize;
		//cout << "blockCnt : " << blockCnt << "\n";
		//cout << "depth : " << depth << "\n";
		finput.close();
	}

	void insert(int key, int value) {
		//cout << binFileName << " : binFileName\n";
		openBin(binFileName);
		//cout << root << " : root\n";
		if (root == 0) {
			int tmp = 0;
			//cout << "insert\n";
			this->root = 1; // root를 1로 설정
			foutput.open(binFileName, ios::in | ios::out | ios::binary);
			foutput.seekp(headerSize, ios::beg);
			foutput.write(re_c(&key), sizeof(int));
			foutput.write(re_c(&value), sizeof(int));
			for (int i = 0; i < nodeCapacity - 1; i++) {
				foutput.write(re_c(&tmp), sizeof(int));
				foutput.write(re_c(&tmp), sizeof(int));
			}
			foutput.write(re_c(&tmp), sizeof(int));
			foutput.seekp(0, ios::beg);
			foutput.write(re_c(&blockSize), sizeof(int));
			foutput.write(re_c(&root), sizeof(int));
			foutput.write(re_c(&depth), sizeof(int));
			foutput.close();
		}
		else {
			int curDepth = 0;
			int curBID = root;
			non_leafNode* curParent = NULL;
			leafNode leaf;
			while (this->depth != curDepth) { // 내려가는과정
				non_leafNode* curNode = new non_leafNode();
				curNode->parent = curParent;
				curNode->bID = curBID;
				curNode->depth = curDepth;
				finput.open(binFileName, ios::binary);
				int curPos = headerSize + ((curBID - 1) * blockSize);
				finput.seekg(curPos);
				int nextid;
				finput.read(re_c(&nextid), sizeof(int));
				curNode->NextLevelBID = nextid;
				for (int i = 0; i < nodeCapacity; i++) {
					int k = 0; int b = 0;
					finput.read(re_c(&k), sizeof(int));
					finput.read(re_c(&b), sizeof(int));
					if (k == 0) continue;
					non_leafEntry tmp(k, b);
					curNode->non_leafList.push_back(tmp);
				}
				finput.close();
				bool flag = false;
				curBID = curNode->NextLevelBID;
				for (int i = 0; i < curNode->non_leafList.size(); i++) {
					non_leafEntry t = curNode->non_leafList[i];
					if (key < t.key) {
						curDepth++;
						curParent = curNode;
						flag = true;
					}
					if (flag) break;
					else curBID = t.NextLevelBID;
					if (i == curNode->non_leafList.size() - 1) {
						curDepth++;
						curParent = curNode;
					}
				}
			}
			//cout << "여기는 들어옴?\n";
			if (curDepth == this->depth) { // 위치를 찾았음 -> 리프노드까지 내려옴
				cout << "curBID : " << curBID << "\n";
				int curBlockPos = headerSize + ((curBID - 1) * blockSize);
				finput.open(binFileName, ios::in | ios::out | ios::binary);
				finput.seekg(curBlockPos);
				//cout << curBlockPos << "\n";
				for (int i = 0; i < nodeCapacity; i++) {
					int k = 0; int v = 0;
					finput.read(re_c(&k), sizeof(int));
					finput.read(re_c(&v), sizeof(int));
					if (k == 0) continue;
					cout << "k : " << k << ", v : " << v << "\n";
					leafEntry tmp(k, v);
					leaf.keyList.push_back(tmp);
				}
				int nbid;
				finput.read(re_c(&nbid), sizeof(int));
				leaf.NextBID = nbid;
				leaf.bID = curBID;
				leaf.parent = curParent;
				leaf.depth = this->depth;
				leafEntry tmp(key, value);
				leaf.keyList.push_back(tmp);
				sort(leaf.keyList.begin(), leaf.keyList.end(), leafEntryOper);
				finput.close();
				//cout << "현재 리프노드에 들어가있는 데이터 개수 : " << leaf.keyList.size() << " \n";
				if (leaf.keyList.size() > nodeCapacity) {
					// 이때 스플릿
					leafNode splitNode;
					int splitIdx;
					if (nodeCapacity % 2 == 0) {
						splitIdx = nodeCapacity / 2;
					}
					else {
						splitIdx = nodeCapacity / 2 + 1;
					}
					for (int i = 0; i < splitIdx; i++) {
						splitNode.keyList.push_back(leaf.keyList.back());
						leaf.keyList.pop_back();
					}
					sort(splitNode.keyList.begin(), splitNode.keyList.end(), leafEntryOper);
					this->blockCnt++;
					splitNode.bID = blockCnt;
					splitNode.NextBID = leaf.NextBID;
					leaf.NextBID = blockCnt;
					int leafPos = headerSize + ((leaf.bID - 1) * blockSize);
					int splitPos = headerSize + ((splitNode.bID - 1) * blockSize);
					//cout << "leaf.bid : " << leaf.bID << ", split.bid : " << splitNode.bID << "\n";
					foutput.open(binFileName, ios::in | ios::out | ios::binary);
					// ----------------- 원래있던애 write -------------------
					foutput.seekp(leafPos);
					//cout << "leafNode size : " << leaf.keyList.size() << "\n";
					for (int i = 0; i < leaf.keyList.size(); i++) {
						int k = leaf.keyList[i].key;
						int v = leaf.keyList[i].value;
						foutput.write(re_c(&k), sizeof(int));
						foutput.write(re_c(&v), sizeof(int));
					}
					int emptySize = nodeCapacity - leaf.keyList.size();
					int empty = 0;
					for (int i = 0; i < emptySize; i++) {
						foutput.write(re_c(&empty), sizeof(int));
						foutput.write(re_c(&empty), sizeof(int));
					}
					foutput.write(re_c(&leaf.NextBID), sizeof(int));
					//cout << "리프 스플릿 됐을때";
					//cout << " leaf.NextBID : " << leaf.NextBID << "\n";
					// ----------------- 스플릿 된애 write -------------------
					foutput.seekp(splitPos, ios::beg);
					//cout << "splitNode size : " << splitNode.keyList.size() << "\n";
					for (int i = 0; i < splitNode.keyList.size(); i++) {
						int k = splitNode.keyList[i].key;
						int v = splitNode.keyList[i].value;
						foutput.write(re_c(&k), sizeof(int));
						foutput.write(re_c(&v), sizeof(int));
					}
					emptySize = nodeCapacity - splitNode.keyList.size();
					for (int i = 0; i < emptySize; i++) {
						foutput.write(re_c(&empty), sizeof(int));
						foutput.write(re_c(&empty), sizeof(int));
					}
					foutput.write(re_c(&splitNode.NextBID), sizeof(int));
					//cout << "split.nextbid : " << splitNode.NextBID << "\n";

					if (leaf.parent == NULL) { // -> 리프노드들만 있을때
						this->blockCnt++;
						int newNon_leafPos = headerSize + ((blockCnt - 1) * blockSize);
						foutput.seekp(newNon_leafPos);
						foutput.write(re_c(&leaf.bID), sizeof(int));
						foutput.write(re_c(&splitNode.keyList[0].key), sizeof(int));
						foutput.write(re_c(&splitNode.bID), sizeof(int));
						for (int i = 0; i < nodeCapacity - 1; i++) {
							foutput.write(re_c(&empty), sizeof(int));
							foutput.write(re_c(&empty), sizeof(int));
						}
						this->root = blockCnt;
						this->depth++;
					}
					else { // 부모가 있다면 업데이트
						non_leafNode* parentNode = leaf.parent;
						non_leafEntry tmp(splitNode.keyList[0].key, splitNode.bID);
						parentNode->non_leafList.push_back(tmp);
						sort(parentNode->non_leafList.begin(), parentNode->non_leafList.end(), non_leafEntryOper);
						if (parentNode->non_leafList.size() > nodeCapacity) {
							// 부모노드 스플릿해야될 때
							// 연속으로 스플릿 될수도 있다.
							foutput.close();
							splitParentNode(parentNode);

							//cout << "990check root : " << root << ", blockCnt : " << blockCnt << "\n";
							foutput.open(binFileName, ios::in | ios::out | ios::binary);
						}
						else if (parentNode->non_leafList.size() <= nodeCapacity) {
							// 부모노드 안 꽉찼을 때
							int parentPos = headerSize + ((parentNode->bID - 1) * blockSize);
							foutput.seekp(parentPos, ios::beg);
							foutput.write(re_c(&parentNode->NextLevelBID), sizeof(int));
							for (int i = 0; i < parentNode->non_leafList.size(); i++) {
								foutput.write(re_c(&parentNode->non_leafList[i].key), sizeof(int));
								foutput.write(re_c(&parentNode->non_leafList[i].NextLevelBID), sizeof(int));
							}
							int lastSpace = nodeCapacity - parentNode->non_leafList.size();
							for (int i = 0; i < lastSpace; i++) {
								foutput.write(re_c(&empty), sizeof(int));
								foutput.write(re_c(&empty), sizeof(int));
							}
						}
					}

					//// 헤더최신화 꼭 해주기
					foutput.seekp(0, ios::beg);
					foutput.write(re_c(&blockSize), sizeof(int));
					foutput.write(re_c(&root), sizeof(int));
					foutput.write(re_c(&depth), sizeof(int));
					foutput.close();
				}
				else if (leaf.keyList.size() <= nodeCapacity) {
					// 스플릿 안할때
					int writePos = headerSize + ((leaf.bID - 1) * blockSize);
					foutput.open(binFileName, ios::in | ios::out | ios::binary); // ios 인아웃 안해주면 다 새로 써버림
					foutput.seekp(writePos);
					//cout << "leafNode size : " << leaf.keyList.size() << "\n";
					for (int i = 0; i < leaf.keyList.size(); i++) {
						int k = leaf.keyList[i].key;
						int v = leaf.keyList[i].value;
						foutput.write(re_c(&k), sizeof(int));
						foutput.write(re_c(&v), sizeof(int));
					}
					int emptySize = nodeCapacity - leaf.keyList.size();
					int empty = 0;
					for (int i = 0; i < emptySize; i++) {
						foutput.write(re_c(&empty), sizeof(int));
						foutput.write(re_c(&empty), sizeof(int));
					}
					//cout << "leaf split 안할때 ";
					//cout << "leaf.NextBID : " << leaf.NextBID << "\n";
					foutput.write(re_c(&leaf.NextBID), sizeof(int));
					// 헤더최신화 꼭 해주기
					foutput.seekp(0, ios::beg);
					foutput.write(re_c(&blockSize), sizeof(int));
					foutput.write(re_c(&root), sizeof(int));
					foutput.write(re_c(&depth), sizeof(int));
					foutput.close();
				}
			}
		}
	}

	void splitParentNode(non_leafNode* parentNode) {
		int empty = 0;
		non_leafNode* splitNode = new non_leafNode();
		int splitIdx;
		if (nodeCapacity % 2 == 0) {
			splitIdx = nodeCapacity / 2;
		}
		else {
			splitIdx = nodeCapacity / 2 + 1;
		}
		for (int i = 0; i < splitIdx; i++) {
			splitNode->non_leafList.push_back(parentNode->non_leafList.back());
			parentNode->non_leafList.pop_back();
		}
		sort(splitNode->non_leafList.begin(), splitNode->non_leafList.end(), non_leafEntryOper);
		this->blockCnt++;
		splitNode->bID = blockCnt;
		foutput.open(binFileName, ios::in | ios::out | ios::binary);
		int originPos = headerSize + ((parentNode->bID - 1) * blockSize);
		foutput.seekp(originPos, ios::beg);
		foutput.write(re_c(&parentNode->NextLevelBID), sizeof(int));
		for (int i = 0; i < parentNode->non_leafList.size(); i++) {
			foutput.write(re_c(&parentNode->non_leafList[i].key), sizeof(int));
			foutput.write(re_c(&parentNode->non_leafList[i].NextLevelBID), sizeof(int));
		}
		int emptySize = nodeCapacity - parentNode->non_leafList.size();
		for (int i = 0; i < emptySize; i++) {
			foutput.write(re_c(&empty), sizeof(int));
			foutput.write(re_c(&empty), sizeof(int));
		}
		if (parentNode->parent == NULL) { // 맨위에 애가 스플릿되면 새로 루트만들어 줘야함
			non_leafNode* tmpNode = new non_leafNode();
			this->blockCnt++;
			int newNodePos = headerSize + ((blockCnt - 1) * blockSize);
			foutput.seekp(newNodePos, ios::beg);
			foutput.write(re_c(&parentNode->bID), sizeof(int));
			foutput.write(re_c(&splitNode->non_leafList[0].key), sizeof(int));
			foutput.write(re_c(&splitNode->bID), sizeof(int));
			for (int i = 0; i < nodeCapacity - 1; i++) {
				foutput.write(re_c(&empty), sizeof(int));
				foutput.write(re_c(&empty), sizeof(int));
			}
			this->root = blockCnt;
			this->depth++;
		}
		else {
			//cout << "990 check\n";
			//cout << "blockCnt : " << this->blockCnt << " , root : " << root << " \n";
			non_leafNode* curSplitNode = splitNode;
			non_leafNode* grandParentNode = parentNode->parent;
			non_leafEntry tmp(curSplitNode->non_leafList[0].key, curSplitNode->bID);
			grandParentNode->non_leafList.push_back(tmp);
			sort(grandParentNode->non_leafList.begin(), grandParentNode->non_leafList.end(), non_leafEntryOper);
			if (grandParentNode->non_leafList.size() > nodeCapacity) {
				foutput.close();
				splitParentNode(grandParentNode);
				foutput.open(binFileName, ios::in | ios::out | ios::binary);
			}
			else if (grandParentNode->non_leafList.size() <= nodeCapacity) {
				int grandParentPos = headerSize + ((grandParentNode->bID - 1) * blockSize);
				foutput.seekp(grandParentPos, ios::beg);
				foutput.write(re_c(&grandParentNode->NextLevelBID), sizeof(int));
				for (int i = 0; i < grandParentNode->non_leafList.size(); i++) {
					foutput.write(re_c(&grandParentNode->non_leafList[i].key), sizeof(int));
					foutput.write(re_c(&grandParentNode->non_leafList[i].NextLevelBID), sizeof(int));
				}
				int g_emptySize = nodeCapacity - grandParentNode->non_leafList.size();
				for (int i = 0; i < g_emptySize; i++) {
					foutput.write(re_c(&empty), sizeof(int));
					foutput.write(re_c(&empty), sizeof(int));
				}
			}
		}
		int tmpBID = splitNode->non_leafList[0].NextLevelBID;
		splitNode->non_leafList.erase(splitNode->non_leafList.begin());
		splitNode->NextLevelBID = tmpBID;
		int splitPos = headerSize + ((splitNode->bID - 1) * blockSize);
		foutput.seekp(splitPos, ios::beg);
		foutput.write(re_c(&splitNode->NextLevelBID), sizeof(int));
		for (int i = 0; i < splitNode->non_leafList.size(); i++) {
			foutput.write(re_c(&splitNode->non_leafList[i].key), sizeof(int));
			foutput.write(re_c(&splitNode->non_leafList[i].NextLevelBID), sizeof(int));
		}
		int s_emptySize = nodeCapacity - splitNode->non_leafList.size();
		for (int i = 0; i < s_emptySize; i++) {
			foutput.write(re_c(&empty), sizeof(int));
			foutput.write(re_c(&empty), sizeof(int));
		}
		foutput.close();
	}

	void search(int key, char* outputFileName) {
		openBin(binFileName);
		int curDepth = 0;
		int curBID = root;
		non_leafNode* curParent = NULL;
		leafNode leaf;
		while (this->depth != curDepth) {
			non_leafNode* curNode = new non_leafNode();
			curNode->parent = curParent;
			curNode->bID = curBID;
			curNode->depth = curDepth;
			finput.open(binFileName, ios::binary);
			int curPos = headerSize + ((curBID - 1) * blockSize);
			finput.seekg(curPos);
			int nextid;
			finput.read(re_c(&nextid), sizeof(int));
			curNode->NextLevelBID = nextid;
			for (int i = 0; i < nodeCapacity; i++) {
				int k = 0; int b = 0;
				finput.read(re_c(&k), sizeof(int));
				finput.read(re_c(&b), sizeof(int));
				if (k == 0) continue;
				non_leafEntry tmp(k, b);
				curNode->non_leafList.push_back(tmp);
			}
			finput.close();
			bool flag = false;
			curBID = curNode->NextLevelBID;
			for (int i = 0; i < curNode->non_leafList.size(); i++) {
				non_leafEntry t = curNode->non_leafList[i];
				if (key < t.key) {
					curDepth++;
					curParent = curNode;
					flag = true;
				}
				if (flag) break;
				else curBID = t.NextLevelBID;
				if (i == curNode->non_leafList.size() - 1) {
					curDepth++;
					curParent = curNode;
				}
			}
		}

		int ansBPos = headerSize + ((curBID - 1) * blockSize);
		finput.open(binFileName, ios::in | ios::out | ios::binary);
		finput.seekg(ansBPos, ios::beg);
		for (int i = 0; i < nodeCapacity; i++) {
			int ansKey = 0;
			int ansValue = 0;
			finput.read(re_c(&ansKey), sizeof(int));
			finput.read(re_c(&ansValue), sizeof(int));
			if (ansKey == key) {
				cout << "ansValue : " << ansValue << "\n";
				foutput.open(outputFileName, ios::out | ios::app);
				foutput << ansKey << ", " << ansValue << "\n";
				foutput.close();
				break;
			}
		}
		finput.close();
	}

	void rangeSearch(int start, int end, char* outputFileName) {
		openBin(binFileName);
		int curDepth = 0;
		int curBID = root;
		non_leafNode* curParent = NULL;
		leafNode leaf;
		while (this->depth != curDepth) {
			non_leafNode* curNode = new non_leafNode();
			curNode->parent = curParent;
			curNode->bID = curBID;
			curNode->depth = curDepth;
			finput.open(binFileName, ios::binary);
			int curPos = headerSize + ((curBID - 1) * blockSize);
			finput.seekg(curPos);
			int nextid;
			finput.read(re_c(&nextid), sizeof(int));
			curNode->NextLevelBID = nextid;
			for (int i = 0; i < nodeCapacity; i++) {
				int k = 0; int b = 0;
				finput.read(re_c(&k), sizeof(int));
				finput.read(re_c(&b), sizeof(int));
				if (k == 0) continue;
				non_leafEntry tmp(k, b);
				curNode->non_leafList.push_back(tmp);
			}
			finput.close();
			bool flag = false;
			curBID = curNode->NextLevelBID;
			for (int i = 0; i < curNode->non_leafList.size(); i++) {
				non_leafEntry t = curNode->non_leafList[i];
				if (start < t.key) {
					curDepth++;
					curParent = curNode;
					flag = true;
				}
				if (flag) break;
				else curBID = t.NextLevelBID;
				if (i == curNode->non_leafList.size() - 1) {
					curDepth++;
					curParent = curNode;
				}
			}
		}

		int curBPos = headerSize + ((curBID - 1) * blockSize);
		int cNid;
		vector<pair<int, int>> answer;
		finput.open(binFileName, ios::in | ios::out | ios::binary);
		bool flag = false;
		while (!flag) {
			finput.seekg(curBPos, ios::beg);
			for (int i = 0; i < nodeCapacity; i++) {
				int k = 0; int v = 0;
				finput.read(re_c(&k), sizeof(int));
				finput.read(re_c(&v), sizeof(int));
				if (k >= end) {
					flag = true;
					break;
				}
				if (k == 0) continue;
				if (k >= start) {
					answer.push_back({ k,v });
				}
			}
			finput.read(re_c(&cNid), sizeof(int));
			curBPos = headerSize + ((cNid - 1) * blockSize);
		}
		finput.close();
		foutput.open(outputFileName, ios::out | ios::app);
		for (int i = 0; i < answer.size(); i++) {
			foutput << answer[i].first << "," << answer[i].second << "\t";
		}
		foutput << "\n";
		foutput.close();
	}

	void print(char* printTxtFileName) {
		openBin(binFileName);
		vector<int> zeroLevel;
		vector<int> oneLevel;
		if (this->depth == 0) { // 트리에 리프밖에 없을때
			finput.open(binFileName, ios::in | ios::out | ios::binary);
			int rootPos = headerSize + ((root - 1) * blockSize);
			finput.seekg(rootPos, ios::beg);
			for (int i = 0; i < nodeCapacity; i++) {
				int k, v;
				finput.read(re_c(&k), sizeof(int));
				finput.read(re_c(&v), sizeof(int));
				if (k == 0) continue;
				zeroLevel.push_back(k);
			}
			finput.close();
		}
		else {
			finput.open(binFileName, ios::in | ios::out | ios::binary);
			int rootPos = headerSize + ((root - 1) * blockSize);
			finput.seekg(rootPos, ios::beg);
			vector<int> bidList;
			int tmp;
			finput.read(re_c(&tmp), sizeof(int));
			bidList.push_back(tmp);
			for (int i = 0; i < nodeCapacity; i++) {
				int k = 0; int b = 0;
				finput.read(re_c(&k), sizeof(int));
				finput.read(re_c(&b), sizeof(int));
				if (k == 0) continue;
				zeroLevel.push_back(k);
				bidList.push_back(b);
			}
			for (int i = 0; i < bidList.size(); i++) {
				int curBPos = headerSize + ((bidList[i] - 1) * blockSize);
				finput.seekg(curBPos, ios::beg);
				if (this->depth == 1) { // 트리 레벨이 1이면 얘네들은 리프
					for (int j = 0; j < nodeCapacity; j++) {
						int k = 0; int v = 0;
						finput.read(re_c(&k), sizeof(int));
						finput.read(re_c(&v), sizeof(int));
						if (k == 0) continue;
						oneLevel.push_back(k);
					}
				}
				else { // 아니면 논리프
					int waste;
					finput.read(re_c(&waste), sizeof(int));
					for (int j = 0; j < nodeCapacity; j++) {
						int k = 0; int b = 0;
						finput.read(re_c(&k), sizeof(int));
						finput.read(re_c(&b), sizeof(int));
						if (k == 0) continue;
						oneLevel.push_back(k);
					}
				}
			}
			finput.close();
		}
		foutput.open(printTxtFileName);
		foutput << "<0>\n";
		for (int i = 0; i < zeroLevel.size(); i++) {
			if (i == zeroLevel.size() - 1) {
				foutput << zeroLevel[i] << "\n";
			}
			else {
				foutput << zeroLevel[i] << ", ";
			}
		}
		foutput << "<1>\n";
		for (int i = 0; i < oneLevel.size(); i++) {
			if (i == oneLevel.size() - 1) {
				foutput << oneLevel[i] << "\n";
			}
			else {
				foutput << oneLevel[i] << ", ";
			}
		}
		foutput.close();
	}

};


int main(int argc, char* argv[]) {
	char command = argv[1][0];
	ifstream finput;
	ofstream foutput;
	BTree btree;
	char c;
	switch (command)
	{
	case 'c':
		btree.blockSize = stoi(argv[3]);
		btree.binFileName = argv[2];
		btree.create(argv[2], stoi(argv[3]));
		break;
	case 'i':
		finput.open(argv[3]); // input텍스트 오픈 
		btree.binFileName = argv[2];
		if (finput.is_open()) {
			char* input = new char[24];
			while (finput >> input) {
				int key, value;
				key = atoi(strtok(input, ","));
				value = atoi(strtok(NULL, " "));
				cout << key << " " << value << "\n";
				btree.insert(key, value);
			}
		}
		finput.close();
		break;
	case 's':
		finput.open(argv[3]);
		btree.binFileName = argv[2];
		if (finput.is_open()) {
			char* input = new char[24];
			while (finput >> input) {
				int key;
				key = atoi(input);
				btree.search(key, argv[4]);
			}
		}
		finput.close();
		break;
	case 'r':
		finput.open(argv[3]);
		btree.binFileName = argv[2];
		if (finput.is_open()) {
			char* input = new char[24];
			while (finput >> input) {
				int StartRange, EndRange;
				StartRange = atoi(strtok(input, ","));
				EndRange = atoi(strtok(NULL, " "));
				btree.rangeSearch(StartRange, EndRange, argv[4]);
			}
		}
		finput.close();
		break;
	case 'p':
		btree.binFileName = argv[2];
		btree.print(argv[3]);
		break;
	}

	return 0;
}
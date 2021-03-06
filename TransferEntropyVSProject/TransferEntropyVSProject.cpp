// TransferEntropyVSProject.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <math.h>
#include <tuple>
#include <unordered_map>
#include <map>
#include <iterator>

using namespace std;

template <class XCLASS, class YCLASS>
double TransferEntropy(XCLASS&, YCLASS&);

void split(const string& str, vector<string>& cont, char delim = ',')
{
	std::size_t current, previous = 0;
	current = str.find(delim);
	while (current != std::string::npos) {
		cont.push_back(str.substr(previous, current - previous));
		previous = current + 1;
		current = str.find(delim, previous);
	}
	cont.push_back(str.substr(previous, current - previous));
}

int symbol_length = 3;

void getSamplingFrames(vector<string>& frames, const string& movementDataPath, double samplingRate, vector<double>& rotationalVelocity) {
	ifstream ifs(movementDataPath);

	int samplePer = (int) (90.0 * samplingRate); //Framerate ~90 FPS
	int aChange = 1;
	int frameCount = samplePer; //Sample first frame
	string entry;
	string prevRotation[] = { "","","" }; //items[5], 6, then 7
	double prevTime;
	if (ifs) {
		while (getline(ifs, entry)) {
			if (entry.find("Date and clock time") == -1) {
				vector<string> items;
				split(entry, items);

				if (frameCount == samplePer) {
					frames.push_back(items[1]);

					double rVelocity;
					if (prevRotation[0] == "") {
						rVelocity = 0;
					}
					else { //Velocity can be calculated from last frame
						double deltaThetaSquared
							= (stod(items[5]) - stod(prevRotation[0])) * (stod(items[5]) - stod(prevRotation[0]))
							+ (stod(items[6]) - stod(prevRotation[1])) * (stod(items[6]) - stod(prevRotation[1]))
							+ (stod(items[7]) - stod(prevRotation[2])) * (stod(items[7]) - stod(prevRotation[2]));
						double deltaTheta = sqrt(deltaThetaSquared);
						rVelocity = deltaTheta / (stod(items[1]) - prevTime);
					}
					rotationalVelocity.push_back(rVelocity);
					frameCount = 0;
				}
				prevRotation[0] = items[5];
				prevRotation[1] = items[6];
				prevRotation[2] = items[7];
				prevTime = stod(items[1]);
				frameCount++;
			}
		}
	}

	ifs.close();
}

void getScoreDifferenceData(vector<int>& difference, const string& userScorePath, const string& peerScorePath, const vector<string>& sampleFrames) {
	ifstream userScoreStream(userScorePath);
	ifstream peerScoreStream(peerScorePath);

	map<double, int> diffMap;

	string entry;
	size_t frameIndex = 0;

	if (userScoreStream && peerScoreStream) {
		while (getline(userScoreStream, entry)) { //Read user score data first
			if (entry.find("Date and clock time") == -1) { //Meaning we're on a row that isn't the column titles:
				vector<string> items;
				split(entry, items);
				int scoreDiffVal = stoi(items[2]) - stoi(items[3]); //User score minus peer score
				diffMap[stod(items[1])] = scoreDiffVal;
			}
		}
		while (getline(peerScoreStream, entry)) {
			if (entry.find("Date and clock time") == -1) {
				vector<string> items;
				split(entry, items);
				int scoreDiffVal = stoi(items[2]) - stoi(items[3]); //User score minus peer score
				diffMap[stod(items[1])] = scoreDiffVal;
			}
		}
		//Now, fill diffMap and difference vec with intermediate values:
		for (const string& s : sampleFrames) {
			double frame = stod(s);
			if (diffMap.find(frame) == diffMap.end()) {
				if (frameIndex != 0) {
					diffMap[frame] = 1;
					diffMap[frame] = prev(diffMap.find(frame))->second;
				}
				else {
					frameIndex++;
					diffMap[frame] = 0;
				}
			}
			difference.push_back(diffMap[frame]);
		}

	}

	userScoreStream.close();
	peerScoreStream.close();
}

int main()
{
	vector<int> scoreDifferenceData;
	vector<float> velocity;

	/*
	vector<float> X, Y;
	ifstream ifs("Book1.csv");
	string word;
	if (ifs) {
		while (ifs >> word) {
			if (word.find('X') == -1) {
				X.push_back(stof(word.substr(0, word.find(','))));
				Y.push_back(stof(word.substr(word.find(',') + 1)));
			}
		}
	}
	ifs.close();
	*/

	

	vector<string> frames;
	vector<double> rotationalVelocityData;

	getSamplingFrames(frames, "MovementData.csv", 0.2, rotationalVelocityData);
	getScoreDifferenceData(scoreDifferenceData, "UserScoreData.csv", "PeerScoreData.csv", frames);

	//double teScoreToMovement = TransferEntropy(scoreDifferenceData, rotationalVelocityData);

	cout << "Score to movement: "
		<< TransferEntropy(scoreDifferenceData, rotationalVelocityData) << endl <<
		"Movement to score: " << TransferEntropy(rotationalVelocityData, scoreDifferenceData) << endl;

	system("pause");

    return 0;
}


string getSymbol(double x, double y, double z) {
	string sym = "";
	int first, second, third;
	if (x < y && x < z)
		first = 0;
	else if (x < y || x < z)
		first = 1;
	else
		first = 2;
	if (y < z) {
		if (first == 0) {
			second = 1;
			third = 2;
		}
		else {
			second = 0;
			if (z > x)
				third = 2;
			else
				third = 1;
		}
	}
	else {
		if (first == 0) {
			second = 2;
			third = 1;
		}
		else {
			second = 1;
			if (z > x)
				third = 1;
			else
				third = 0;
		}
	}
	sym += to_string(first);
	sym += to_string(second);
	sym += to_string(third);
	return sym;
}


void getSymbols(vector<string>& sym, vector<double>& vec) {
	for (size_t i = 0; i < vec.size() - symbol_length + 1; i++) { //Symbol is length 3
		sym.push_back(getSymbol(vec[i], vec[i + 1], vec[i + 2]));
	}
}

void getSymbols(vector<string>& sym, vector<int>& vec) { //Getting the score data symbols is a little different. 
	vector<int> scoreChange;
	for (size_t i = 0; i < vec.size(); i++) {
		if (i == 0 || vec[i] == vec[i - 1]) {
			scoreChange.push_back(0);
		}
		else if (vec[i] > vec[i - 1]) {
			scoreChange.push_back(1);
		}
		else {
			scoreChange.push_back(-1);
		}
	}

	for (size_t i = 0; i < vec.size() - symbol_length + 1; i++) {
		sym.push_back(to_string(scoreChange[i]) + to_string(scoreChange[i + 1]) + to_string(scoreChange[i + 2]));
	}
}

double ShannonEntropy(const vector<vector<string> >& symbolsVec) {
	unordered_map<string, int> mp;

	for (size_t i = 0; i < symbolsVec[0].size(); i++) {
		string symbol = "";
		for (int listCount = 0; listCount < symbolsVec.size(); listCount++) {
			symbol += symbolsVec[listCount][i];
		}
		mp[symbol]++;
	}
	size_t count = symbolsVec[0].size();
	double entropy = 0;

	for (pair<string, int> keyValPair : mp) {
		entropy += (-keyValPair.second / (double)count) * log(keyValPair.second / (double)count);
	}

	return entropy;
}

/*

double ShannonEntropyTriple(vector<string>& a, vector<string>& b, vector<string>& c) {
	unordered_map<string, int> mp;

	for (size_t i = 0; i < a.size(); i++) {
		mp[a[i] + b[i] + c[i]]++;
	}

	unordered_map<string, int>::iterator it;
	size_t count = a.size();
	double entropy = 0;

	for (it = mp.begin(); it != mp.end(); it++) {
		entropy += (-it->second / (double)count) * log(it->second / (double)count);
	}

	return entropy;
}

double ShannonEntropyPair(vector<string>& a, vector<string>& b) {
	unordered_map<string, int> mp;

	for (size_t i = 0; i < a.size(); i++) {
		mp[a[i] + b[i]]++;
	}

	unordered_map<string, int>::iterator it;
	size_t count = a.size();
	double entropy = 0;

	for (it = mp.begin(); it != mp.end(); it++)
	{
		entropy += (-it->second / (double)count) * log(it->second / (double)count);
	}

	return entropy;
}

double ShannonEntropy(vector<string>& vec) {
	unordered_map<string, int> mp{};

	for (size_t i = 0; i < vec.size(); i++) {
		mp[vec[i]]++;
	}

	//Calculate entropy (sum of -p(x)*ln(p(x)) )
	size_t count = vec.size();

	unordered_map<string, int>::iterator it;
	double entropy = 0;

	for (it = mp.begin(); it != mp.end(); it++)
	{
		entropy += (-it->second / (double)count) * log(it->second / (double)count);
	}
	return entropy;
}

*/

template <class XCLASS, class YCLASS>
double TransferEntropy(XCLASS& X, YCLASS& Y) {
	vector<string> symbolsX_t;
	getSymbols(symbolsX_t, X);
	symbolsX_t.erase(symbolsX_t.begin());

	vector<string> symbolsX_t_prev;
	getSymbols(symbolsX_t_prev, X);
	symbolsX_t_prev.pop_back();

	vector<string> symbolsY_t;
	getSymbols(symbolsY_t, Y);
	symbolsY_t.erase(symbolsY_t.begin());

	vector<string> symbolsY_t_prev;
	getSymbols(symbolsY_t_prev, Y);
	symbolsY_t_prev.pop_back();

	double transfer_entropy = 0; //Transfer entropy from data in column X to column Y:
	transfer_entropy += ShannonEntropy({ symbolsY_t, symbolsY_t_prev }) - ShannonEntropy({ symbolsY_t_prev });
	transfer_entropy += -ShannonEntropy({ symbolsY_t, symbolsY_t_prev, symbolsX_t_prev } );
	transfer_entropy += ShannonEntropy({ symbolsY_t_prev, symbolsX_t_prev });

	return transfer_entropy;
}



#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <random>
#include <cmath>
#include <cstdlib>
#include <filesystem> //C++17 and later
#include <sstream>


using namespace std;
namespace fs = filesystem;

long double pi = 3.14159265359;

void createEmptyFolder(const string& folderPath) {
    if (fs::exists(folderPath)) {
        for (const auto& entry : fs::directory_iterator(folderPath)) {
            fs::remove_all(entry.path());
        }
    }
    fs::create_directory(folderPath);
}

bool areBodiesTooClose(const vector<long double>& body1, const vector<long double>& body2, long double minDistance) {
    long double distX = body2[0] - body1[0];
    long double distY = body2[1] - body1[1];
    long double distance = sqrt(distX * distX + distY * distY);
    return distance < minDistance;
}

vector<long double> stringToVectorDouble(const string& inputString) {
    std::stringstream ss(inputString);
    std::vector<long double> resultVector;
    long double tempValue;

    while (ss >> tempValue) {
        resultVector.push_back(tempValue);
        if (ss.peek() == '/') {
            ss.ignore();
        }
    }

    return resultVector;
}

vector<string> tokenizeString(const string& input, char delimiter) {
    std::vector<std::string> tokens;
    std::istringstream tokenStream(input);
    std::string token;

    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

vector<vector<long double>> getBodies(const string& filename) {
    vector<vector<long double>> result;

    ifstream inputFile("objects/" + filename);
    if (!inputFile.is_open()) {
        cerr << "Error opening file: " << filename << endl;
    }

    string line;
    while (getline(inputFile, line)) {
        result.push_back(stringToVectorDouble(line));
        //cout << line << endl;
    }

    inputFile.close();
    return result;

}

void saveBodies(vector<vector<long double>> data, const string& filename) {
    ofstream outputFile("objects/" + filename);

    for (const auto& row : data) {
        outputFile << row[0];
        outputFile << "/";
        outputFile << row[1];
        outputFile << "/";
        outputFile << row[2];
        outputFile << "/";
        outputFile << row[3];
        outputFile << "/";
        outputFile << row[4];
        outputFile << endl;
    }

    outputFile.close();
}

int main(int argc, char* argv[]) {
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> dis(0.0, 1.0);
    vector<vector<long double>> bodies;
    double G, d, video_speed, friction, temperature_coefficient;
    int frames, chunks, framesInChunk, scrx, scry;
    int currentChunk = 1;

    if (argc > 1) {
        createEmptyFolder("chunks");
        int numberOfBodies = stoi(argv[1]);
        G = stod(argv[2]);
        chunks = stoi(argv[3]);
        framesInChunk = stoi(argv[4]);
        scrx = stoi(argv[5]);
        scry = stoi(argv[6]);
        d = 2.0 * stod(argv[7]);
        video_speed = stod(argv[8]);
        friction = stod(argv[9]);
        frames = stoi(argv[10]);
        temperature_coefficient = stod(argv[11]);
        string sceneData = argv[12];
        string saveAs = argv[12];


        if (sceneData.find("/") != string::npos) {

            vector<string> scenesTEMP = tokenizeString(sceneData, '#');        
            
            for (int i = 0; i < scenesTEMP.size(); ++i) {
                vector<string> currentScene = tokenizeString(scenesTEMP[i], '/');
                vector<vector<long double>> bodiesVector = getBodies(currentScene[0]);

                for (int j = 0; j < bodiesVector.size(); ++j) {
                    bodiesVector[j][0] += stod(currentScene[1]);
                    bodiesVector[j][1] += stod(currentScene[2]);
                    bodiesVector[j][2] += stod(currentScene[3]);
                    bodiesVector[j][3] += stod(currentScene[4]);
                }
                bodies.insert(bodies.end(), bodiesVector.begin(), bodiesVector.end());
            }
        }
        else {

            for (int i = 0; i < numberOfBodies; ++i) {
                vector<long double> row;
                bool tooClose;

                do {//              X                 Y      speedX  speedY  kelvins
                    row = { dis(gen) * scrx, dis(gen) * scry, 0.0,    0.0,     0.0 };

                    tooClose = false;
                    for (const auto& existingBody : bodies) {
                        if (areBodiesTooClose(row, existingBody, d)) {
                            tooClose = true;
                            break;
                        }
                    }
                } while (tooClose);

                bodies.push_back(row);
            }
        }

        int goalPercentage = 0;
        ofstream outputFile("chunks/chunk_"+to_string(currentChunk)+".txt");

        int writeCounter = 0;
        int effectiveFramesInChunk = static_cast<int>((double)framesInChunk * video_speed);
        for (int k = 0; k < frames; ++k) {
            for (int i = 0; i < bodies.size(); ++i) {

                for (int j = 0; j < bodies.size(); ++j) {
                    if (j != i) {
                        long double distX;
                        long double distY;

                        if (i < j) {
                            distX = bodies[j][0] - bodies[i][0];
                            distY = bodies[j][1] - bodies[i][1];
                        }
                        else {
                            distX = (bodies[j][0]) - bodies[i][0] - bodies[i][2];
                            distY = (bodies[j][1]) - bodies[i][1] - bodies[i][3];
                        }

                        long double distSquared = distX * distX + distY * distY;
                        long double Fx, Fy;

                        if (distSquared <= d * d) {// Collision
                            long double overlap = d - sqrt(distSquared);
                            long double angle = atan2(distY, distX);
                            long double pushX = 0.5 * overlap * cos(angle);
                            long double pushY = 0.5 * overlap * sin(angle);

                            long double relativeSpeedX = bodies[i][2] - bodies[j][2];
                            long double relativeSpeedY = bodies[i][3] - bodies[j][3];

                            long double collisionEnergy = temperature_coefficient * sqrt(relativeSpeedX * relativeSpeedX + relativeSpeedY * relativeSpeedY);

                            long double dampingForceX = -friction * relativeSpeedX;
                            long double dampingForceY = -friction * relativeSpeedY;

                            bodies[i][4] += collisionEnergy;
                            bodies[j][4] += collisionEnergy;

                            bodies[i][2] -= pushX;
                            bodies[i][3] -= pushY;
                            bodies[j][2] += pushX;
                            bodies[j][3] += pushY;

                            bodies[i][2] += dampingForceX;
                            bodies[i][3] += dampingForceY;
                            bodies[j][2] -= dampingForceX;
                            bodies[j][3] -= dampingForceY;

                        }
                        else {
                            Fx = G * (1.0 / distSquared) * (distX / sqrt(distSquared));
                            Fy = G * (1.0 / distSquared) * (distY / sqrt(distSquared));
                        }

                        bodies[i][2] += Fx;
                        bodies[i][3] += Fy;
                    }
                }

                bodies[i][0] += bodies[i][2];
                bodies[i][1] += bodies[i][3];
                

            }


            if (k == effectiveFramesInChunk * currentChunk) {
                currentChunk += 1;
                outputFile.close();
                outputFile.open("chunks/chunk_" + to_string(currentChunk) + ".txt");
            }
            if (writeCounter == 0) {
                for (const auto& row : bodies) {
                    outputFile << (int)row[0];
                    outputFile << "/";
                    outputFile << (int)row[1];
                    outputFile << "/";
                    outputFile << (int)row[4];
                    outputFile << endl;
                }
                outputFile << "#";
            }
        

            int percentage = ((float)k / (float)frames) * 100;
            if (percentage > goalPercentage) {
                goalPercentage++;
                cout << percentage << endl;

            }
            writeCounter = (writeCounter + 1) % static_cast<int>(video_speed);
        }

        if (saveAs.find('.') != string::npos) saveBodies(bodies, saveAs);
        cout << "C++ program has finished successfully!" << endl;

    }
    else {
        cerr << "Error: Couldn't get enough argv[]." << endl;
    }

    
    return 0;
}

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <cstring>
#include <iterator>

using namespace std;

int main(int argc, char* argv[]) {
    // Check if the SNP file is provided as an argument
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <path_to_SNP_file>\n";
        return 1;
    }

    // Use fast IO
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    unordered_set<string> snp_positions;  // To store SNPs for fast lookups

    ifstream snpFile(argv[1]);
    if (!snpFile.is_open()) {
        cerr << "Error opening SNP file.\n";
        return 1;
    }

    string line;
    while (getline(snpFile, line)) {
        istringstream iss(line);
        vector<string> cols((istream_iterator<string>(iss)), istream_iterator<string>());

        string chrom = cols[0];
        int pos = stoi(cols[1]);
        string snp = (cols[2] == "C" && cols[3] == "T") || (cols[2] == "T" && cols[3] == "C") ? "CT" : 
                     (cols[2] == "G" && cols[3] == "A") || (cols[2] == "A" && cols[3] == "G") ? "GA" : "";

        if (!snp.empty()) {
            snp_positions.insert(chrom + "_" + to_string(pos) + "_" + snp);
        }
    }

    snpFile.close();

    cerr << "SNP list loaded\n";

    vector<string> cols;
    string qual, newQual;
    while (getline(cin, line)) {
        if (line[0] == '@') {
            cout << line << '\n';
            continue;
        }

        istringstream iss(line);
        cols.assign((istream_iterator<string>(iss)), istream_iterator<string>());

        string chrom = cols[2];
        int position = stoi(cols[3]);
        bool reverse = stoi(cols[1]) & 16;
        string read = cols[9];
        qual = cols[10];
        newQual = qual;

        bool overlap_found = false;

        for (int i = 0; i < read.length(); i++) {
            int currentPos = position + i;
            string snpType = (reverse) ? "GA" : "CT";
            if (snp_positions.find(chrom + "_" + to_string(currentPos) + "_" + snpType) != snp_positions.end()) {
                overlap_found = true;
                newQual[i] = '!';
            }
        }

        if (overlap_found) {
            cols[10] = newQual;
        }

        for (const auto& col : cols) {
            cout << col << '\t';
        }
        cout << '\n';
    }

    return 0;
}

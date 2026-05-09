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
    // Check if the SNP file and the two end-distances are provided
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " <path_to_SNP_file> <bases_from_start> <bases_from_end>\n";
        cerr << "Example: " << argv[0] << " known_snps.txt 5 2\n";
        cerr << "  (Recalibrates the first 5 bases and the last 2 bases of the read string)\n";
        return 1;
    }

    int bases_from_start = stoi(argv[2]);
    int bases_from_end = stoi(argv[3]);

    // Use fast IO
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    unordered_set<string> snp_positions;

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
        // Target C/T transitions for forward strands, G/A for reverse strands
        string snp = (cols[2] == "C" && cols[3] == "T") || (cols[2] == "T" && cols[3] == "C") ? "CT" : 
                     (cols[2] == "G" && cols[3] == "A") || (cols[2] == "A" && cols[3] == "G") ? "GA" : "";

        if (!snp.empty()) {
            snp_positions.insert(chrom + "_" + to_string(pos) + "_" + snp);
        }
    }

    snpFile.close();
    cerr << "SNP list loaded. Restricting recalibration to the first " << bases_from_start 
         << " bases and the last " << bases_from_end << " bases of the read.\n";

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
            // ONLY recalibrate if within the specified distance from the start or end of the BAM read string
            if (i < bases_from_start || i >= (read.length() - bases_from_end)) {
                int currentPos = position + i; // Assumes perfect match without indels/clipping
                string snpType = (reverse) ? "GA" : "CT";
                
                if (snp_positions.find(chrom + "_" + to_string(currentPos) + "_" + snpType) != snp_positions.end()) {
                    overlap_found = true;
                    newQual[i] = '!'; // Downgrade to Phred 0
                }
            }
        }

        if (overlap_found) {
            cols[10] = newQual;
        }

        for (int i = 0; i < cols.size(); i++) {
            cout << cols[i] << (i == cols.size() - 1 ? "" : "\t");
        }
        cout << '\n';
    }

    return 0;
}
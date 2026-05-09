# baSSdrop
Tool to recalibrate (downgrade) base quality scores in a strand-aware manner for single-stranded ancient DNA library-derived data.

## Purpose
In single-stranded ancient DNA libraries, post-mortem deamination (C &rarr; T) is strand-specific. 
`baSSdrop` targets these specific transition types at a user-defined set of SNP coordinates. It identifies bases that match the expected strand-specific damage profile, and downgrades their base quality score to **0** (`!`). This keeps a substantial amount of data that is lost in comparison to trimming bases from each end of a fragment to remove damage.

## Amended Version from Original by pontussk
This version allows users to define how many bases from the start and end of the BAM read string should C>T transitions be targeted for recalibration, instead of removing them across the whole fragment which is overly conservative.

## Installation
First, clone the repository to your local machine:
```bash
git clone https://github.com/malmarri/baSSdrop.git
cd baSSdrop
```

## Compilation
Compile the tool using any standard C++ compiler:
```bash
g++ -O3 baSSdrop.cpp -o baSSdrop
```

## Usage
The tool reads SAM text from `stdin` and writes SAM text to `stdout`. It is typically used by piping `samtools view` into it:

```bash
samtools view -h input.bam | ./baSSdrop <path_to_SNP_file> <bases_from_start> <bases_from_end> | samtools view -bS - > output_recalibrated.bam
```

### Parameters
1. **`<path_to_SNP_file>`**: A tab-separated file (no header) containing known SNPs: `[Chr] [Pos] [Ref] [Alt]`.
2. **`<bases_from_start>`**: Number of bases from the 5' end of the **BAM sequence string** to recalibrate.
3. **`<bases_from_end>`**: Number of bases from the 3' end of the **BAM sequence string** to recalibrate.

## Pipeline Example
To recalibrate damage-prone transitions only within the terminal **5 bp** and **5 bp** of each read:

```bash
samtools view -h input.bam | \
  ./baSSdrop known_snps.txt 5 5 | \
  samtools view -bS - > output_recalibrated.bam
```

## Testing
To verify a successful installation and check functionality using the provided test data:

1. **Clone and Enter Directory:**
   ```bash
   git clone https://github.com/malmarri/baSSdrop.git
   cd baSSdrop
   ```

2. **Compile:**
   ```bash
   g++ -O3 baSSdrop.cpp -o baSSdrop
   ```

3. **Run Test:**
   ```bash
   ./baSSdrop test_data/known_snps.txt 5 2 < test_data/test.sam
   ```

4. **Verify Output:**
   - In `read1_forward`, the quality at position 101 (2nd base) should be `!` (downscaled).
   - In `read2_reverse`, the quality at position 118 (19th base) should be `!` (downscaled).
   - All other bases should retain their original quality (`1`).

## Input SNP File Format
The file should be tab- or space-separated and contain at least 4 columns. All known SNPs (e.g. 1240K) can be included, the program will only focus on the appropriate type of variant to downgrade base quality.
```text
chr1    101    C    T
chr1    108    G    A
chr2    505    T    C
```

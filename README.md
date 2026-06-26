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
2. **`<bases_from_start>`**: Number of bases from the **5' end of the original molecule** to recalibrate. For forward reads this corresponds to the start of the BAM sequence string; for reverse reads it corresponds to the end (since BAM stores the reverse complement). Soft-clipped and inserted bases are excluded from the window count.
3. **`<bases_from_end>`**: Number of bases from the **3' end of the original molecule** to recalibrate. Symmetric with `<bases_from_start>` but applied to the opposite end.

## Input SNP File Format
The file should be tab- or space-separated and contain at least 4 columns: `[Chr] [Pos] [Ref] [Alt]`. All known SNPs (e.g. 1240K, 1KG, HGDP etc ) can be included; the program will only focus on the appropriate transitions to downgrade base quality.
```text
chr1    101    C    T
chr1    108    G    A
chr2    505    T    C
```

> [!NOTE]
> A ready-to-use, pre-formatted SNP file based on the AADR v66 2 million SNP dataset is provided directly in this repository as [v66.ADDR.2M.snp](v66.ADDR.2M.snp).
> 
> In addition, a custom Y-chromosome specific SNP list is provided as [Y_chrom_yleaf.snp](Y_chrom_yleaf.snp). This contains **909,439** unique Y-chromosome SNPs integrated from the base set and all Yleaf databases (FTDNA, ISOGG, YFull v14 and v10). It is optimized to protect diagnostic SNPs required for Y-haplogroup classification during base quality recalibration.


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
   - In `read1_forward`, the quality string should be `1!11!1111111111111!1` (recalibrating positions 101, 104, and 118; leaving 105 and 115 untouched).
   - In `read2_reverse`, the quality string should be `1!1111111111111!11!1` (recalibrating positions 101, 115, and 118; leaving 102 and 114 untouched).
   - All other bases should retain their original quality (`1`).

## Testing: CIGAR-aware recalibration (soft-clips and indels)

A second test file (`test_data/test_indels.sam`) verifies that position arithmetic is correct for reads containing soft-clips, insertions, and deletions. Run with the same SNP file and trim values:

```bash
./baSSdrop test_data/known_snps.txt 5 2 < test_data/test_indels.sam
```

All three reads start at reference position 100, and the SNP file contains C→T transitions at positions **101, 104, 105, 115, and 118**.

### read3_fwd_softclip (`3S17M`, pos=103)

The first 3 bases are **soft-clipped** (unmapped). The POS field points to the first *aligned* base (ref=103), so the 5-base damage window covers **refs 103–107**, and the 2-base 3′ window covers **refs 118–119**.

| Output | Quality string |
|---|---|
| **Correct (new code)** | `!!!1!!111111111111!1` — recalibrates refs 104 and 105 (in 5′ window) and ref 118 (in 3′ window); soft-clip qualities unchanged |
| ~~Buggy (old code)~~ | `!!!11111111111111111` — window is offset by 3 soft-clipped bases; misses all three SNPs in the mapped damage zone |

### read4_fwd_insertion (`2M1I17M`, pos=100)

A **1-base insertion** is present at read index 2 (between refs 101 and 102). The inserted base has no reference coordinate and is skipped. All subsequent mapped bases shift by +1 in read-string space relative to reference space.

| Output | Quality string |
|---|---|
| **Correct (new code)** | `1!11111111111111111!` — recalibrates ref 101 (index 1, in 5′ window) and ref 118 (index 19, in 3′ window) |
| ~~Buggy (old code)~~ | `1!11!1111111111111!1` — falsely recalibrates index 4 (thinking it is at ref 104, actual ref 103) and index 18 (thinking ref 118, actual ref 117); misses the real ref 118 at index 19 |

### read5_fwd_deletion (`2M1D17M`, pos=100)

A **1-base deletion** at reference position 102 is skipped in the read string. All subsequent mapped bases shift by −1 in read-string space relative to reference space.

| Output | Quality string |
|---|---|
| **Correct (new code)** | `1!1!!111111111111!1` — recalibrates ref 101 (index 1), ref 104 (index 3), ref 105 (index 4), and ref 118 (index 17) |
| ~~Buggy (old code)~~ | `1!11!1111111111111!` — misses ref 104 at index 3 (computes ref 103); falsely recalibrates index 18 (computes ref 118, actual ref 119); misses ref 118 at index 17 |

# baSSdrop
Tool to recalibrate (downgrade) base quality scores in a strand-aware manner for single-stranded ancient DNA library-derived data.

## Purpose
In single-stranded ancient DNA libraries, post-mortem deamination (C &rarr; T) is strand-specific. When these reads are mapped to a reference:
- Reads from the **forward strand** show biochemical damage as **C &rarr; T** transitions.
- Reads from the **reverse strand** (reverse-complemented by BWA) show damage as **G &rarr; A** transitions.

`baSSdrop` targets these specific transition types at a user-defined set of SNP coordinates. It identifies bases that *could* be biological variants but match the expected strand-specific damage profile, and downgrades their base quality score to **0** (`!`). This prevents false-positive variant calling while preserving transversions and transitions in the middle of reads where damage is less frequent.

## Amended Version from Original by pontussk
This version allows users to define  how many bases from the start and end of the BAM read string should be targeted for recalibration. 

## Compilation
Compile the tool using any standard C++ compiler:
```bash
g++ -O3 baSSdrop.cpp -o baSSdrop
```

## Usage
The tool reads SAM text from `stdin` and writes SAM text to `stdout`.

```bash
./baSSdrop <path_to_SNP_file> <bases_from_start> <bases_from_end>
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

## Input SNP File Format
The file should be tab- or space-separated and contain at least 4 columns:
```text
chr1    101    C    T
chr1    108    G    A
chr2    505    T    C
```
- For forward reads, the tool recalibrates positions matching `C/T`.
- For reverse reads, the tool recalibrates positions matching `G/A`.
- Transversions are ignored.
